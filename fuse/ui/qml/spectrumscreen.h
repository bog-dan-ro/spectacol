/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

#ifndef SPECTRUMSHOTSCREEN_H
#define SPECTRUMSHOTSCREEN_H

#include <QImage>
#include <QQuickImageProvider>


QImage buff2Image(const QByteArray &buffer, const QString &fileName = QString());
QImage buff2Image(const unsigned char *buffer, size_t bufferSize, const QString &fileName = QString());
QImage scr2Image(const QByteArray &buffer);
QImage scr2Image(const char *buffer, size_t bufferSize);


class SpectrumScreenImageProvider : public QQuickImageProvider
{
public:
    SpectrumScreenImageProvider()
        : QQuickImageProvider(Image, ForceAsynchronousImageLoading)
    {}

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

#endif // SPECTRUMSHOTSCREEN_H
