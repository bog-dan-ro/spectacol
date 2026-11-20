/* Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "spectrumscreen.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
class MediaIconsTest : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QTemporaryDir cache;
    QString write(const QString &name, const QByteArray &bytes)
    {
        const auto path = QDir(home.path()).filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
            qFatal("Cannot write preview fixture");
        return path;
    }
    // What the engine gets for id, once the provider's threads have it
    static QImage request(SpectrumScreenImageProvider &provider, const QString &id,
                          const QSize &requestedSize = {})
    {
        std::unique_ptr<QQuickImageResponse> response(
                provider.requestImageResponse(id, requestedSize));
        QSignalSpy finished(response.get(), &QQuickImageResponse::finished);
        if (!finished.wait(5000))
            return {};
        std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
        return factory ? factory->image().convertToFormat(QImage::Format_RGBA8888) : QImage();
    }
    static QImage rgba(const QImage &image)
    {
        return image.convertToFormat(QImage::Format_RGBA8888);
    }
    static QByteArray screenBytes()
    {
        QByteArray screen(6912, '\0');
        for (int i = 0; i < 6144; ++i)
            screen[i] = char(i % 256);
        for (int i = 6144; i < screen.size(); ++i)
            screen[i] = char(0x45);
        return screen;
    }
    static QString coverId(const QString &path)
    {
        const QFileInfo info(path);
        return ScreenCover{ path, info.fileName(), info.size(),
                            info.lastModified().toMSecsSinceEpoch() }
                .id();
    }
private slots:
    void placeholdersAtDisplaySize()
    {
        QVERIFY(home.isValid());
        const QStringList paths {
            home.path(),
            write(u"disk.trd"_s, QByteArray(655360, '\0')),
            home.filePath(u"missing.tap"_s),
            write(u"unknown.tap"_s, "no screen preview"),
            u"compressed64:0:"_s + QString::fromLatin1(qCompress("short").toBase64(QByteArray::Base64UrlEncoding))
        };
        SpectrumScreenImageProvider provider(cache.path());
        for (const auto &path : paths) {
            for (int side : {16, 24, 32, 64, 512, 1536}) {
                const auto image = request(provider, path, QSize(side, side));
                QVERIFY2(!image.isNull(), qPrintable(path));
                QCOMPARE(image.size(), QSize(side, side));
                QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
                QVERIFY(image.pixelColor(side / 2, side / 2).alpha() > 0);
            }
            QCOMPARE(request(provider, path).size(), QSize(256, 256));
            QCOMPARE(request(provider, path, QSize(320, 256)).size(), QSize(256, 256));
        }
    }
    void preservesScreenPixels()
    {
        const auto screen = screenBytes();
        const auto path = write(u"game.scr"_s, screen);
        const auto compressed = u"compressed64:0:"_s
                + QString::fromLatin1(qCompress(screen).toBase64(QByteArray::Base64UrlEncoding));
        const auto expected = rgba(scr2Image(screen));
        QCOMPARE(expected.size(), QSize(256, 192));
        SpectrumScreenImageProvider provider(cache.path());
        for (const auto &id : { path, compressed, coverId(path) }) {
            for (int side : { 16, 512, 1536 })
                QCOMPARE(request(provider, id, QSize(side, side)), expected);
        }
    }
    void coverIds()
    {
        const ScreenCover cover{
            u"content://com.android.externalstorage.documents/tree/primary%3AGames/document/primary%3AGames%2Fa b.tap"_s,
            u"a b.tap"_s, 12345, 1700000000123
        };
        const auto parsed = ScreenCover::fromId(cover.id());
        QVERIFY(parsed);
        QCOMPARE(parsed->path, cover.path);
        QCOMPARE(parsed->name, cover.name);
        QCOMPARE(parsed->size, cover.size);
        QCOMPARE(parsed->modified, cover.modified);
        // Survives the image URL the engine parses
        const QUrl url(u"image://spectrum/"_s + cover.id());
        QCOMPARE(url.path().mid(1), cover.id());
        QVERIFY(!ScreenCover::fromId(u"cover/1/2/!!/x"_s));
        QVERIFY(!ScreenCover::fromId(u"/tmp/game.tap"_s));
    }
    void screensAreReadOnce()
    {
        const auto screen = screenBytes();
        const auto path = write(u"once.scr"_s, screen);
        const auto id = coverId(path);
        const auto empty = write(u"empty.tap"_s, "no screen");
        const auto emptyId = coverId(empty);
        QTemporaryDir folder;
        SpectrumScreenImageProvider provider(folder.path());
        QCOMPARE(request(provider, id), rgba(scr2Image(screen)));
        const auto icon = request(provider, emptyId, QSize(64, 64));
        // Gone, the files still show what they showed: the cache answers
        QVERIFY(QFile::remove(path));
        QVERIFY(QFile::remove(empty));
        QCOMPARE(request(provider, id), rgba(scr2Image(screen)));
        QCOMPARE(request(provider, emptyId, QSize(64, 64)), icon);

        // A damaged entry isn't trusted: the file is read again (and it has gone)
        const auto entries = QDir(folder.path()).entryInfoList({ u"*.scr"_s }, QDir::Files);
        QCOMPARE(entries.size(), 2);
        for (const auto &entry : entries) {
            QFile file(entry.filePath());
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("damaged");
        }
        const auto broken = request(provider, id, QSize(64, 64));
        QCOMPARE(broken.size(), QSize(64, 64));

        ScreenCache cache(folder.path());
        const auto info = QFileInfo(write(u"again.scr"_s, screen));
        QVERIFY(cache.store(info.filePath(), info.size(), 1, screen));
        QCOMPARE(cache.find(info.filePath(), info.size(), 1), screen);
        QVERIFY(!cache.find(info.filePath(), info.size(), 2));
        QVERIFY(!cache.store(info.filePath(), info.size(), 3, "short"));
    }
    void pruneKeepsRecentScreens()
    {
        QTemporaryDir folder;
        ScreenCache cache(folder.path());
        QByteArray noisy(6912, '\0');
        for (int i = 0; i < noisy.size(); ++i)
            noisy[i] = char((i * 2654435761u) >> 13);
        for (int i = 0; i < 10; ++i) {
            QVERIFY(cache.store(u"/games/%1.tap"_s.arg(i), 100, 1, noisy));
            // Older first: the least recently used go
            QFile entry(QDir(folder.path())
                                .entryInfoList({ u"*.scr"_s }, QDir::Files, QDir::Time)
                                .first()
                                .filePath());
            QVERIFY(entry.open(QIODevice::ReadWrite));
            entry.setFileTime(QDateTime::currentDateTime().addSecs(-1000 + i),
                              QFileDevice::FileModificationTime);
        }
        // What each entry takes on disk, in whole blocks
        const auto entrySize =
                QDir(folder.path()).entryInfoList({ u"*.scr"_s }, QDir::Files).first().size();
        cache.prune(4 * ((entrySize + 4095) / 4096 * 4096));
        const auto kept = QDir(folder.path()).entryInfoList({ u"*.scr"_s }, QDir::Files);
        QCOMPARE(kept.size(), 4);
        for (int i = 6; i < 10; ++i)
            QVERIFY(cache.find(u"/games/%1.tap"_s.arg(i), 100, 1));
    }
    void canceledResponsesFinish()
    {
        QStringList ids;
        for (int i = 0; i < 32; ++i)
            ids.append(coverId(write(u"cancel%1.scr"_s.arg(i), screenBytes())));
        QTemporaryDir folder;
        SpectrumScreenImageProvider provider(folder.path());
        std::vector<std::unique_ptr<QQuickImageResponse>> responses;
        std::vector<std::unique_ptr<QSignalSpy>> spies;
        for (const auto &id : ids) {
            responses.emplace_back(provider.requestImageResponse(id, {}));
            spies.push_back(std::make_unique<QSignalSpy>(responses.back().get(),
                                                         &QQuickImageResponse::finished));
            responses.back()->cancel();
        }
        for (const auto &spy : spies)
            QTRY_COMPARE(spy->count(), 1);
    }
};
QTEST_MAIN(MediaIconsTest)
#include "test_media_icons.moc"
