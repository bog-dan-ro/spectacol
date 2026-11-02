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

#include "nextsdcard.h"
#include <QDir>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <zxkit/media/Compression.h>

using namespace Qt::StringLiterals;
namespace {
void put(QByteArray &data, int offset, quint32 value, int size)
{
    for (int i = 0; i < size; ++i)
        data[offset + i] = char(value >> (i * 8));
}
QByteArray image(char marker)
{
    QByteArray data(64 * 512, 0);
    data[0] = char(0xEB);
    data[1] = 0x3C;
    data[2] = char(0x90);
    put(data, 11, 512, 2);
    data[13] = 1;
    put(data, 14, 1, 2);
    data[16] = 1;
    put(data, 17, 16, 2);
    put(data, 19, 64, 2);
    data[21] = char(0xF8);
    put(data, 22, 1, 2);
    data[510] = 0x55;
    data[511] = char(0xAA);
    data[512] = char(0xF8);
    data[513] = char(0xFF);
    data[514] = char(0xFF);
    data.back() = marker;
    return data;
}
QByteArray zip(const QByteArray &data)
{
    const QByteArray name = "cspect-next-1gb.img";
    const auto crc =
        zxkit::computeCrc32(std::span(reinterpret_cast<const uint8_t *>(data.constData()), size_t(data.size())));
    QByteArray result(30, 0);
    put(result, 0, 0x04034B50, 4);
    put(result, 4, 20, 2);
    put(result, 14, crc, 4);
    put(result, 18, data.size(), 4);
    put(result, 22, data.size(), 4);
    put(result, 26, name.size(), 2);
    result += name;
    result += data;
    const auto offset = result.size();
    QByteArray central(46, 0);
    put(central, 0, 0x02014B50, 4);
    put(central, 4, 20, 2);
    put(central, 6, 20, 2);
    put(central, 16, crc, 4);
    put(central, 20, data.size(), 4);
    put(central, 24, data.size(), 4);
    put(central, 28, name.size(), 2);
    result += central;
    result += name;
    QByteArray end(22, 0);
    put(end, 0, 0x06054B50, 4);
    put(end, 8, 1, 2);
    put(end, 10, 1, 2);
    put(end, 12, central.size() + name.size(), 4);
    put(end, 16, offset, 4);
    return result + end;
}
bool write(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
QByteArray read(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}
} // namespace
class NextSdCardTest : public QObject {
    Q_OBJECT
  private slots:
    void updatePreservesImagesAndHandlesFailure()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName(u"SpectacolTests"_s);
        QCoreApplication::setApplicationName(u"NextSdCard"_s);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        qputenv("XDG_DATA_HOME", directory.path().toUtf8());
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray body = zip(image('N'));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto *socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                const auto request = socket->readAll();
                const auto response = request.startsWith("GET /latest ")
                                          ? QByteArray("<a href=\"/sn-emulator-test.zip\">Download</a>")
                                          : body;
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(response.size()) +
                              "\r\nConnection: close\r\n\r\n" + response);
                socket->disconnectFromHost();
            });
        });
        QSettings{}.setValue(u"Next/distroPage"_s, u"http://127.0.0.1:%1/latest"_s.arg(server.serverPort()));
        auto &card = NextSdCard::instance();
        const auto defaultPath = card.imagePath();
        QDir().mkpath(QFileInfo(defaultPath).absolutePath());
        const auto custom = directory.filePath(u"custom.img"_s);
        QVERIFY(write(custom, image('C')));
        QVERIFY(write(defaultPath, image('O')));
        card.setImagePath(custom);
        card.updateDistro();
        QTRY_COMPARE_WITH_TIMEOUT(card.state(), NextSdCard::Ready, 5000);
        QCOMPARE(read(defaultPath), image('N'));
        QCOMPARE(read(custom), image('C'));
        QVERIFY(!card.backupPath().isEmpty());
        QCOMPARE(read(card.backupPath()), image('O'));
        QCOMPARE(card.imagePath(), defaultPath);
        card.setImagePath(custom);
        body = "not a zip";
        card.updateDistro();
        QTRY_COMPARE_WITH_TIMEOUT(card.state(), NextSdCard::Failed, 5000);
        QCOMPARE(card.imagePath(), custom);
        QCOMPARE(read(defaultPath), image('N'));
        QCOMPARE(read(custom), image('C'));
        body = zip(image('X'));
        const auto cancel = connect(&card, &NextSdCard::stateChanged, this, [&] {
            if (card.state() == NextSdCard::Extracting)
                card.cancel();
        });
        card.updateDistro();
        QTRY_COMPARE_WITH_TIMEOUT(card.state(), NextSdCard::Idle, 5000);
        disconnect(cancel);
        QCOMPARE(card.imagePath(), custom);
        QCOMPARE(read(defaultPath), image('N'));
        QVERIFY(!QFile::exists(defaultPath + u".part"_s));
        // Unmount all test-owned paths before their temporary directory goes away.
        card.setImagePath({});
    }
};
QTEST_GUILESS_MAIN(NextSdCardTest)
#include "test_next_sdcard.moc"
