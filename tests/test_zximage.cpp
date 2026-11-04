/* Copyright (c) 2015-2026, BogDan Vatra. SPDX-License-Identifier: GPL-3.0-or-later */
#include "zximage.h"
#include <QSignalSpy>
#include <QTest>

class ImageTest : public QObject {
    Q_OBJECT
private slots:
    void changedPixelsAndRetainedUploads() {
        auto image = ZxImage::instance();
        auto source = zxkit::makeShared<zxkit::VideoFrame>(320, 256);
        source.writeLock()->clear(zxkit::packRgba(1, 2, 3));
        QSignalSpy updates(image, &ZxImage::needsUpdate);
        image->presentFrame(source);
        QCOMPARE(updates.size(), 1);
        auto retained = image->takeFrame(); // Simulate a queued texture upload.
        QVERIFY(!retained.dirty.isEmpty());
        for (int i = 0; i < 20; ++i) image->presentFrame(source);
        QCOMPARE(updates.size(), 1);
        QVERIFY(image->takeFrame().dirty.isEmpty());
        source.writeLock()->scanline(50)[10] = zxkit::packRgba(9, 8, 7);
        image->presentFrame(source);
        QCOMPARE(updates.size(), 2);
        const auto next = image->takeFrame();
        QCOMPARE(next.dirty, QRect(0, 50, 320, 1));
        QCOMPARE(retained.image.pixelColor(10, 50), QColor(1, 2, 3));
        QCOMPARE(next.image.pixelColor(10, 50), QColor(9, 8, 7));
        // Cropping a paused frame still invalidates the texture and geometry.
        QSignalSpy geometry(image, &ZxImage::geometryChanged);
        image->setBorderMargins({0, 0, 0, 0});
        QCOMPARE(updates.size(), 3);
        QCOMPARE(geometry.size(), 1);
        QCOMPARE(image->takeFrame().image.size(), QSize(256, 192));
    }
};
QTEST_GUILESS_MAIN(ImageTest)
#include "test_zximage.moc"
