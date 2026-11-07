/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#pragma once

#include <QImage>
#include <QMutex>
#include <QMargins>
#include <QSGTexture>

#include <zxkit/video/VideoFrame.h>

class QRhiTexture;

class ZxImage : public QObject
{
    Q_OBJECT
private:
    ZxImage();
    ~ZxImage();

public:
    struct Frame
    {
        QImage image;
        QRect dirty;
    };

    static ZxImage *instance();

    // Any thread
    QSize imageSize() const;
    // Returns the last finished frame and the area changed since the previous call
    Frame takeFrame();
    QImage image() const;
    QMargins borderLimits() const;
    void setBorderMargins(zxkit::BorderMargins margins);

    // Engine thread: copies the rows that changed since the previous frame
    void presentFrame(const zxkit::VideoFramePtr &frame);

signals:
    void sourceGeometryChanged();
    void needsUpdate();
    void geometryChanged(QSize size);

private:
    bool updateImage(const zxkit::VideoFrame &pixels, bool &changed);
    zxkit::BorderMargins m_borderMargins;
    zxkit::VideoFramePtr m_sourceFrame;
    // The renderer gets shallow copies of m_frameImage: presentFrame() detaches
    // from them, so they never change while QRhi uploads them.
    mutable QMutex m_syncVars;
    QImage m_frameImage;
    QRect m_frameDirty;
};

// The ZX screen texture, it uploads only the parts of the frame that changed
class ZxTexture : public QSGTexture
{
public:
    ZxTexture();
    ~ZxTexture() override;

    // Render thread, while the GUI thread is blocked
    void setFrame(ZxImage::Frame frame);

    // QSGTexture interface
    qint64 comparisonKey() const final;
    QRhiTexture *rhiTexture() const final;
    QSize textureSize() const final;
    bool hasAlphaChannel() const final;
    bool hasMipmaps() const final;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) final;

private:
    QImage m_image;
    QRect m_dirty;
    QSize m_size;
    QSize m_failedSize;
    std::unique_ptr<QRhiTexture> m_texture;
};
