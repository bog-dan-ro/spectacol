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

#include "screencache.h"

#include <QImage>
#include <QQuickAsyncImageProvider>
#include <QThreadPool>

QImage buff2Image(const QByteArray &buffer, const QString &fileName = QString());
QImage buff2Image(const unsigned char *buffer, size_t bufferSize,
                  const QString &fileName = QString());
QImage scr2Image(const QByteArray &buffer);
QImage scr2Image(const char *buffer, size_t bufferSize);

// image://spectrum/ - the screens media files show, for the file browsers and the tape browser:
//   cover/...: a file's screen, see ScreenCover; read once, then kept in a ScreenCache
//   compressed64:DELTA:DATA: the screen at DELTA in qCompress-ed, base64url data
//   PATH: a local file's screen
// Several screens load at once on the provider's own threads; the ones no longer shown are
// canceled. Placeholders are icons at the requested size; screens keep their native pixels.
class SpectrumScreenImageProvider : public QQuickAsyncImageProvider
{
public:
    explicit SpectrumScreenImageProvider(QString cacheFolder = ScreenCache::defaultFolder());
    ~SpectrumScreenImageProvider() override;

    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;

private:
    QThreadPool m_pool;
    ScreenCache m_cache;
};
