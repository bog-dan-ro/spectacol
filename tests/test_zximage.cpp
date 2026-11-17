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
        QCOMPARE(next.dirty, QRect(10, 50, 1, 1));
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

    void cropBeforeFiltering()
    {
        auto image = ZxImage::instance();
        image->setBorderMargins({ 4, 2, 6, 3 });
        auto source = zxkit::makeShared<zxkit::VideoFrame>(416, 312);
        {
            auto frame = source.writeLock();
            frame->setPaperRect({ 80, 60, 256, 192 });
            frame->clear(zxkit::packRgba(200, 30, 70));
            for (uint32_t y = 0; y < 192; ++y)
                for (uint32_t x = 0; x < 256; ++x)
                    frame->putPixel(80 + x, 60 + y, zxkit::packRgba(x, y, 100));
        }
        image->presentFrame(source);
        const auto cropped = image->takeFrame();
        QCOMPARE(cropped.image.size(), QSize(266, 197));
        QCOMPARE(cropped.image.pixelColor(4, 2), QColor(0, 0, 100));
        QCOMPARE(cropped.image.pixelColor(0, 0), QColor(200, 30, 70));

        QSignalSpy updates(image, &ZxImage::needsUpdate);
        // Hidden border activity never reaches the image handed to the filter.
        source.writeLock()->putPixel(5, 4, zxkit::packRgba(7, 8, 9));
        image->presentFrame(source);
        QCOMPARE(updates.size(), 0);
        QVERIFY(image->takeFrame().dirty.isEmpty());
        source.writeLock()->putPixel(76, 58, zxkit::packRgba(10, 20, 30));
        image->presentFrame(source);
        QCOMPARE(updates.size(), 1);
        QCOMPARE(image->takeFrame().dirty, QRect(0, 0, 1, 1));
        image->setBorderMargins({ 32, 32, 32, 32 });
    }

    void accumulatedDirtyColumns()
    {
        auto image = ZxImage::instance();
        image->setBorderMargins({ 0, 0, 0, 0 });
        auto source = zxkit::makeShared<zxkit::VideoFrame>(64, 48);
        source.writeLock()->setPaperRect({ 0, 0, 64, 48 });
        source.writeLock()->clear(zxkit::packRgba(1, 2, 3));
        image->presentFrame(source);
        image->takeFrame();
        source.writeLock()->putPixel(1, 2, zxkit::packRgba(4, 5, 6));
        image->presentFrame(source);
        source.writeLock()->putPixel(40, 25, zxkit::packRgba(7, 8, 9));
        image->presentFrame(source);
        QCOMPARE(image->takeFrame().dirty, QRect(1, 2, 40, 24));
        source.writeLock()->putPixel(63, 47, zxkit::packRgba(10, 11, 12));
        image->presentFrame(source);
        QCOMPARE(image->takeFrame().dirty, QRect(63, 47, 1, 1));
        image->setBorderMargins({ 32, 32, 32, 32 });
    }

    void filteredUploadsRetainTheirPixels()
    {
        zxkit::VideoFrame source(12, 8);
        source.clear(zxkit::packRgba(1, 2, 3));
        QImage image;
        QCOMPARE(copyFilterOutput(source, image, { 5, 3, 1, 1 }), QRect(0, 0, 12, 8));
        auto queuedUpload = image;
        source.putPixel(5, 3, zxkit::packRgba(90, 80, 70));
        QCOMPARE(copyFilterOutput(source, image, { 5, 3, 1, 1 }), QRect(5, 3, 1, 1));
        QCOMPARE(queuedUpload.pixelColor(5, 3), QColor(1, 2, 3));
        QCOMPARE(image.pixelColor(5, 3), QColor(90, 80, 70));
        QCOMPARE(image.pixelColor(6, 3), QColor(1, 2, 3));
        queuedUpload = {};
        const auto storage = image.constBits();
        source.putPixel(0, 0, zxkit::packRgba(4, 5, 6));
        copyFilterOutput(source, image, { 0, 0, 1, 1 });
        QCOMPARE(image.constBits(), storage);
        QCOMPARE(image.pixelColor(0, 0), QColor(4, 5, 6));
        zxkit::VideoFrame resized(7, 5);
        resized.clear(zxkit::packRgba(11, 12, 13));
        QCOMPARE(copyFilterOutput(resized, image, { 1, 1, 1, 1 }), QRect(0, 0, 7, 5));
        QCOMPARE(image.pixelColor(6, 4), QColor(11, 12, 13));
    }
};
QTEST_GUILESS_MAIN(ImageTest)
#include "test_zximage.moc"
