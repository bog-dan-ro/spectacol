/* Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "spectrumscreen.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
class MediaIconsTest : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QString write(const QString &name, const QByteArray &bytes)
    {
        const auto path = QDir(home.path()).filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
            qFatal("Cannot write preview fixture");
        return path;
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
        SpectrumScreenImageProvider provider;
        for (const auto &path : paths) {
            for (int side : {16, 24, 32, 64, 512, 1536}) {
                QSize size;
                const auto image = provider.requestImage(path, &size, QSize(side, side));
                QCOMPARE(size, image.size());
                QVERIFY2(!image.isNull(), qPrintable(path));
                QCOMPARE(image.size(), QSize(side, side));
                QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
                QVERIFY(image.pixelColor(side / 2, side / 2).alpha() > 0);
            }
            QCOMPARE(provider.requestImage(path, nullptr, {}).size(), QSize(256, 256));
            QCOMPARE(provider.requestImage(path, nullptr, QSize(320, 256)).size(), QSize(256, 256));
        }
    }
    void preservesScreenPixels()
    {
        QByteArray screen(6912, '\0');
        for (int i = 0; i < 6144; ++i) screen[i] = char(i % 256);
        for (int i = 6144; i < screen.size(); ++i) screen[i] = char(0x45);
        const auto path = write(u"game.scr"_s, screen);
        const auto compressed = u"compressed64:0:"_s
                + QString::fromLatin1(qCompress(screen).toBase64(QByteArray::Base64UrlEncoding));
        const auto expected = scr2Image(screen);
        QCOMPARE(expected.size(), QSize(256, 192));
        SpectrumScreenImageProvider provider;
        for (const auto &id : {path, compressed}) {
            for (int side : {16, 512, 1536}) {
                QSize size;
                QCOMPARE(provider.requestImage(id, &size, QSize(side, side)), expected);
                QCOMPARE(size, expected.size());
            }
        }
    }
};
QTEST_MAIN(MediaIconsTest)
#include "test_media_icons.moc"
