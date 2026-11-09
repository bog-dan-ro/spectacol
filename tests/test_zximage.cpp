/* Copyright (c) 2015-2026, BogDan Vatra. SPDX-License-Identifier: GPL-3.0-or-later */
#include "zximage.h"
#include "screenfilterinput.h"
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
        QCOMPARE(retained.pixelScale, QSize(1, 1));
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

    void nextPresentationGrid()
    {
        auto image = ZxImage::instance();
        image->setBorderMargins({ 32, 32, 32, 32 });
        auto source = zxkit::makeShared<zxkit::VideoFrame>(640, 512);
        {
            auto frame = source.writeLock();
            frame->setPaperRect({ 64, 64, 512, 384 });
            frame->clear(zxkit::packRgba(1, 2, 3));
        }
        image->presentFrame(source);
        const auto frame = image->takeFrame();
        QCOMPARE(frame.pixelScale, QSize(2, 2));
        QCOMPARE(frame.image.size(), QSize(640, 512));
        QCOMPARE(filterInputPixelScale(frame.image, frame.pixelScale), QSize(2, 2));
        image->setBorderMargins({ 0, 0, 0, 0 });
        const auto cropped = image->takeFrame();
        QCOMPARE(cropped.pixelScale, QSize(2, 2));
        QCOMPARE(cropped.image.size(), QSize(512, 384));
        image->setBorderMargins({ 32, 32, 32, 32 });
    }

    void preserveHighResolutionPixels()
    {
        QImage image(10, 8, QImage::Format_RGBA8888);
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                image.setPixelColor(x, y, QColor(x / 2, y / 2, 127));
        QCOMPARE(filterInputPixelScale(image, { 2, 2 }), QSize(2, 2));
        QCOMPARE(filterInputPixelScale(image, { 1, 1 }), QSize(1, 1));

        // High-resolution columns remain intact, even when rows are doubled.
        image.setPixelColor(1, 0, Qt::red);
        image.setPixelColor(1, 1, Qt::red);
        QCOMPARE(filterInputPixelScale(image, { 2, 2 }), QSize(1, 2));
        image.setPixelColor(4, 3, Qt::green);
        QCOMPARE(filterInputPixelScale(image, { 2, 2 }), QSize(1, 1));

        QImage oddSize(9, 7, QImage::Format_RGBA8888);
        oddSize.fill(Qt::black);
        QCOMPARE(filterInputPixelScale(oddSize, { 2, 2 }), QSize(1, 1));
        QCOMPARE(filterInputPixelScale({}, { 2, 2 }), QSize(1, 1));
    }
};
QTEST_GUILESS_MAIN(ImageTest)
#include "test_zximage.moc"
