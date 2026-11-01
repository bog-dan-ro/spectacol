/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "zxgamesmodel.h"

#include <QAbstractItemModelTester>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUrlQuery>

#include <functional>
#include <memory>

using namespace Qt::StringLiterals;

namespace {
const QByteArray tape = QByteArray::fromHex("0200ffff");

struct Response {
    QByteArray bytes;
    int status = 200;
    QByteArray type = "application/json";
    int delay = 0;
};

class Server : public QTcpServer {
public:
    std::function<Response(const QUrl &)> handler;
    QList<QUrl> requests;

    Server()
    {
        if (!listen(QHostAddress::LocalHost))
            qFatal("Cannot listen on localhost");
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                auto buffer = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
                    *buffer += socket->readAll();
                    if (!buffer->contains("\r\n\r\n"))
                        return;
                    socket->disconnect(this);
                    const auto url = QUrl::fromEncoded(buffer->split(' ').value(1));
                    requests.append(url);
                    const auto response = handler ? handler(url) : Response{{}, 404};
                    const QPointer<QTcpSocket> target(socket);
                    QTimer::singleShot(response.delay, this, [target, response] {
                        if (!target || target->state() != QAbstractSocket::ConnectedState)
                            return;
                        target->write("HTTP/1.1 " + QByteArray::number(response.status)
                                      + " Test\r\nConnection: close\r\nContent-Type: " + response.type
                                      + "\r\nContent-Length: " + QByteArray::number(response.bytes.size())
                                      + "\r\n\r\n" + response.bytes);
                        target->disconnectFromHost();
                    });
                });
            }
        });
    }

    zxonline::Endpoints endpoints() const
    {
        zxonline::Endpoints endpoints;
        const QUrl base(u"http://127.0.0.1:%1/"_s.arg(serverPort()));
        endpoints.archive = base.resolved(QUrl(u"archive/"_s));
        endpoints.zxinfo = base.resolved(QUrl(u"zxinfo/"_s));
        endpoints.zxinfoMedia = base.resolved(QUrl(u"media/"_s));
        endpoints.spectrumComputing = base.resolved(QUrl(u"computing/"_s));
        return endpoints;
    }
};

QByteArray page(int source, bool second = false)
{
    if (source == 1) {
        const auto hits = second
            ? R"([{"_id":"3","_source":{"title":"Third"}}])"
            : R"([{"_id":"1","_source":{"title":"Test title","machineType":"ZX-Spectrum 48K","originalYearOfRelease":2026,"screens":[{"url":"/zxscreens/1/test.gif"}]}}, {"_id":"2","_source":{"title":"Other"}}])";
        return QByteArray(R"({"hits":{"total":{"value":3},"hits":)") + hits + "}}";
    }
    const auto docs = second ? R"([{"identifier":"3","title":"Third"}])"
        : R"([{"identifier":"1","title":"Test title"},{"identifier":"2","title":"Other"}])";
    return QByteArray(R"({"response":{"numFound":3,"docs":)") + docs + "}}";
}

QByteArray entry(int source, bool media = true)
{
    if (source == 1) {
        QJsonObject object{{u"_source"_s, QJsonObject{
            {u"title"_s, u"Test title"_s}, {u"originalYearOfRelease"_s, 2026},
            {u"availability"_s, u"Available"_s},
            {u"screens"_s, QJsonArray{QJsonObject{{u"url"_s, u"/zxscreens/1/test.gif"_s}}}},
            {u"releases"_s, QJsonArray{QJsonObject{{u"files"_s, QJsonArray{
                QJsonObject{{u"path"_s, media ? u"/pub/sinclair/games/t/Test:1.tap"_s : u"/pub/sinclair/manual.pdf"_s},
                            {u"size"_s, tape.size()}, {u"format"_s, u"Tape (TAP)"_s}},
                QJsonObject{{u"path"_s, u"/zxdb/sinclair/entries/1/Second.tzx.zip"_s}},
                QJsonObject{{u"path"_s, u"file:///tmp/unsupported.tap"_s}}
            }}}}}
        }}};
        if (!media) {
            auto details = object[u"_source"_s].toObject();
            details[u"releases"_s] = QJsonArray{};
            object[u"_source"_s] = details;
        }
        return QJsonDocument(object).toJson();
    }
    QJsonObject object{{u"metadata"_s, QJsonObject{{u"title"_s, u"Test title"_s}, {u"year"_s, u"2026"_s}}},
        {u"files"_s, QJsonArray{
            QJsonObject{{u"name"_s, media ? u"folder/Test:1.tap"_s : u"manual.pdf"_s},
                        {u"size"_s, QString::number(tape.size())},
                        {u"md5"_s, QString::fromLatin1(QCryptographicHash::hash(tape, QCryptographicHash::Md5).toHex())}},
            QJsonObject{{u"name"_s, u"private.tap"_s}, {u"private"_s, true}}
        }}};
    return QJsonDocument(object).toJson();
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}
} // namespace

class OnlineTest : public QObject {
    Q_OBJECT
private slots:
    void urls()
    {
        const zxonline::Endpoints endpoints;
        const auto url = zxonline::searchUrl(endpoints, 1, u"A/B + ? é"_s, 2);
        QCOMPARE(url.host(), u"api.zxinfo.dk"_s);
        QVERIFY(url.toEncoded().contains("A%2FB%20%2B%20%3F%20%C3%A9"));
        QCOMPARE(QUrlQuery(url).queryItemValue(u"offset"_s), u"2"_s);
        QCOMPARE(QUrlQuery(url).queryItemValue(u"size"_s), u"25"_s);
        QCOMPARE(QUrlQuery(url).queryItemValue(u"machinetype"_s), u"ZXSPECTRUM"_s);
        const auto letter = zxonline::searchUrl(endpoints, 1, u"#"_s, 0);
        QVERIFY(letter.toEncoded().contains("byletter/%23?"));
        QVERIFY(letter.fragment().isEmpty());
        const auto archive = zxonline::searchUrl(endpoints, 0, u"Rock & Roll"_s, 1);
        QCOMPARE(QUrlQuery(archive).queryItemValue(u"page"_s), u"2"_s);
        QVERIFY(QUrlQuery(archive).queryItemValue(u"q"_s).contains(u"Rock & Roll"_s));
    }

    void pagingAndDownload_data()
    {
        QTest::addColumn<int>("source");
        QTest::newRow("Internet Archive") << 0;
        QTest::newRow("ZXInfo") << 1;
    }

    void catalogueFileNames()
    {
        auto document = QJsonDocument::fromJson(entry(1)).object();
        auto source = document[u"_source"_s].toObject();
        source[u"releases"_s] = QJsonArray{QJsonObject{{u"files"_s, QJsonArray{QJsonObject{
            {u"path"_s, u"/pub/sinclair/games/t/Question #1?.tap.zip"_s}
        }}}}};
        document[u"_source"_s] = source;
        const auto details = zxonline::parseDetails(document, {}, 1, u"1"_s);
        QCOMPARE(details.files.size(), 1);
        QCOMPARE(details.files[0].name, u"Question #1?.tap.zip"_s);
        const auto url = details.files[0].url;
        QVERIFY(url.query().isEmpty());
        QVERIFY(url.fragment().isEmpty());
        QVERIFY(url.path().endsWith(u"/Question #1?.tap.zip"_s));
        const auto name = zxonline::downloadName(1, u"1"_s, details.files[0]);
        QVERIFY(!name.contains(u'?'));
        QVERIFY(!name.contains(u'/'));
        QVERIFY(name.endsWith(u".tap.zip"_s));
    }

    void pagingAndDownload()
    {
        QFETCH(int, source);
        Server server;
        server.handler = [source](const QUrl &url) -> Response {
            if (url.path().contains(u"entries/"_s) || url.path().contains(u"metadata/"_s))
                return {entry(source)};
            if (url.path().contains(u"search"_s)) {
                const QUrlQuery query(url);
                const int offset = query.queryItemValue(source ? u"offset"_s : u"page"_s).toInt();
                return {page(source, offset == (source ? 1 : 2))};
            }
            return {tape, 200, "application/octet-stream"};
        };
        ZXGamesModel model(server.endpoints());
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        tester.setUseFetchMore(false);
        model.setSource(source);
        model.search(u"Test title"_s);
        QTRY_VERIFY(!model.searching());
        QCOMPARE(model.errorString(), QString{});
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.total(), 3);
        QVERIFY(model.hasMore());
        QCOMPARE(model.data(model.index(0), ZXGamesModel::Title).toString(), u"Test title"_s);
        const auto thumbnail = model.data(model.index(0), ZXGamesModel::Screenshot).toUrl();
        QCOMPARE(thumbnail.host(), u"127.0.0.1"_s);
        QCOMPARE(thumbnail.path(), source ? u"/media/zxscreens/1/test.gif"_s : u"/archive/services/img/1"_s);
        // Reading result roles must not start metadata or media downloads.
        QCOMPARE(server.requests.size(), 1);
        model.loadMore();
        model.loadMore(); // Ignore a duplicate request while a page is in flight.
        QTRY_VERIFY(!model.searching());
        QCOMPARE(model.rowCount(), 3);
        QVERIFY(!model.hasMore());
        QCOMPARE(server.requests.size(), 2);
        model.select(0);
        QTRY_VERIFY(!model.loadingDetails());
        QCOMPARE(model.details()[u"title"_s].toString(), u"Test title"_s);
        const auto files = model.details()[u"files"_s].toList();
        QCOMPARE(files.size(), source ? 2 : 1);
        if (source) {
            QVERIFY(files[0].toMap()[u"url"_s].toUrl().path().contains(u"Mirror.zip/World of Spectrum"_s));
            QVERIFY(files[1].toMap()[u"url"_s].toUrl().path().startsWith(u"/computing/zxdb/"_s));
            QCOMPARE(model.details()[u"screenshot"_s].toUrl().path(), u"/media/zxscreens/1/test.gif"_s);
        }
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        QSignalSpy downloaded(&model, &ZXGamesModel::downloaded);
        model.download(0, folder.path());
        QTRY_COMPARE(downloaded.size(), 1);
        const auto path = downloaded[0][0].toString();
        QVERIFY(!QFileInfo(path).fileName().contains(u':'));
        QCOMPARE(QFileInfo(path).absolutePath(), folder.path());
        QCOMPARE(readFile(path), tape);
        // A failed replacement preserves the successful download for retry.
        server.handler = [](const QUrl &) -> Response { return {"bad", 404}; };
        model.download(0, folder.path());
        QTRY_VERIFY(!model.downloading());
        QVERIFY(!model.errorString().isEmpty());
        QCOMPARE(readFile(path), tape);
        QCOMPARE(downloaded.size(), 1);
        server.handler = [](const QUrl &) -> Response { return {tape, 200, "application/octet-stream"}; };
        model.download(0, folder.path());
        QTRY_COMPARE(downloaded.size(), 2);
        QVERIFY(model.errorString().isEmpty());
    }

    void cancellation()
    {
        Server server;
        server.handler = [](const QUrl &url) -> Response {
            if (url.path().startsWith(u"/archive/"_s))
                return {page(0), 200, "application/json", 180};
            if (url.path().contains(u"entries/"_s))
                return {entry(1), 200, "application/json", 180};
            return {page(1)};
        };
        ZXGamesModel model(server.endpoints());
        model.search(u"Old"_s);
        QTRY_COMPARE(server.requests.size(), 1);
        model.setSource(1);
        model.search(u"New"_s);
        QTRY_VERIFY(!model.searching());
        model.select(0);
        QTRY_COMPARE(server.requests.size(), 3);
        model.search(u"Replacement"_s);
        QTRY_VERIFY(!model.searching());
        QTest::qWait(220);
        QCOMPARE(model.rowCount(), 2);
        QVERIFY(model.details()[u"title"_s].toString().isEmpty());
        QVERIFY(model.errorString().isEmpty());
        QVERIFY(!model.loadingDetails());
        model.select(0);
        model.cancelSelection(); // Closing a title popup drops only its selection.
        QTest::qWait(220);
        QVERIFY(!model.loadingDetails());
        QVERIFY(model.details()[u"title"_s].toString().isEmpty());
        QCOMPARE(model.rowCount(), 2);
    }

    void failureRecovery()
    {
        Server server;
        ZXGamesModel model(server.endpoints());
        server.handler = [](const QUrl &) -> Response { return {"Unavailable", 503}; };
        model.search(u"test"_s);
        QTRY_VERIFY(!model.searching());
        QVERIFY(!model.errorString().isEmpty());
        QVERIFY(!model.searched());
        for (const auto &bad : {QByteArray("bad JSON"), QByteArray("{}")}) {
            server.handler = [bad](const QUrl &) -> Response { return {bad}; };
            model.search(u"test"_s);
            QTRY_VERIFY(!model.searching());
            QVERIFY(!model.errorString().isEmpty());
        }
        server.handler = [](const QUrl &) -> Response { return {R"({"response":{"numFound":0,"docs":[]}})"}; };
        model.search(u"nothing"_s);
        QTRY_VERIFY(!model.searching());
        QVERIFY(model.errorString().isEmpty());
        QVERIFY(model.searched());
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.hasMore());
        server.handler = [](const QUrl &url) -> Response {
            return {url.path().contains(u"metadata/"_s) ? entry(0, false) : page(0)};
        };
        model.search(u"test"_s);
        QTRY_VERIFY(!model.searching());
        model.select(0);
        QTRY_VERIFY(!model.loadingDetails());
        QVERIFY(model.details()[u"files"_s].toList().isEmpty());
        QVERIFY(!model.details()[u"title"_s].toString().isEmpty());
        server.handler = [](const QUrl &) -> Response { return {"Missing", 404}; };
        model.select(0);
        QTRY_VERIFY(!model.loadingDetails());
        QVERIFY(!model.errorString().isEmpty());
        QVERIFY(model.details()[u"title"_s].toString().isEmpty());
    }

    void downloadRecovery()
    {
        Server server;
        server.handler = [](const QUrl &url) -> Response {
            if (url.path().contains(u"metadata/"_s))
                return {entry(0)};
            if (url.path().contains(u"search"_s))
                return {page(0)};
            return {tape, 200, "application/octet-stream"};
        };
        ZXGamesModel model(server.endpoints());
        model.search(u"test"_s);
        QTRY_VERIFY(!model.searching());
        model.select(0);
        QTRY_VERIFY(!model.loadingDetails());
        QTemporaryDir folder;
        QSignalSpy downloaded(&model, &ZXGamesModel::downloaded);
        const auto details = zxonline::parseDetails(QJsonDocument::fromJson(entry(0)).object(), server.endpoints(), 0, u"1"_s);
        const auto path = QDir(folder.path()).filePath(zxonline::downloadName(0, u"1"_s, details.files[0]));
        QVERIFY(QDir().mkdir(path));
        QFile marker(QDir(path).filePath(u"keep"_s));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.write("preserved");
        marker.close();
        model.download(0, folder.path());
        QTRY_VERIFY(!model.downloading());
        QCOMPARE(downloaded.size(), 0);
        QVERIFY(!model.errorString().isEmpty());
        QCOMPARE(readFile(marker.fileName()), QByteArray("preserved"));
        QVERIFY(QDir(path).removeRecursively());
        model.download(0, folder.path());
        QTRY_COMPARE(downloaded.size(), 1);
        QCOMPARE(readFile(path), tape);
        for (const Response response : {Response{"<html>missing</html>", 200, "text/html"},
                                       Response{QByteArray::fromHex("02000000"), 200, "application/octet-stream"}}) {
            server.handler = [response](const QUrl &) { return response; };
            model.download(0, folder.path());
            QTRY_VERIFY(!model.downloading());
            QVERIFY(!model.errorString().isEmpty());
            QCOMPARE(readFile(path), tape);
        }
        server.handler = [](const QUrl &) -> Response { return {tape, 200, "application/octet-stream", 180}; };
        model.download(0, folder.path());
        model.cancelSelection();
        QTest::qWait(220);
        QVERIFY(!model.downloading());
        QCOMPARE(downloaded.size(), 1);
        QCOMPARE(readFile(path), tape);
    }
};

QTEST_GUILESS_MAIN(OnlineTest)
#include "test_online.moc"
