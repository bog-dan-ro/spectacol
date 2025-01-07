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

#include "qmlui.h"
#include "spectrumscreen.h"

#include <QFileInfo>
#include <QIcon>

extern "C"  {
# include <libspectrum.h>
}

enum {
    SPECTRUM_TAPE_BLOCK_SCREEN_SIZE = 1/*first byte*/ + 32 * 24 * 8 + 32 * 24 /*screen data */ + 1 /* last byte */
};

constexpr uint16_t rgb16(int r, int g, int b)
{
    return (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
}

static const uint16_t palette[16] = {
    rgb16(  0,   0,   0),
    rgb16(   0,   0, 192),
    rgb16( 192,   0,   0),
    rgb16( 192,   0, 192),
    rgb16(   0, 192,   0),
    rgb16(   0, 192, 192),
    rgb16( 192, 192,   0),
    rgb16( 192, 192, 192),
    rgb16(   0,   0,   0),
    rgb16(   0,   0, 255),
    rgb16( 255,   0,   0),
    rgb16( 255,   0, 255),
    rgb16(   0, 255,   0),
    rgb16(   0, 255, 255),
    rgb16( 255, 255,   0),
    rgb16( 255, 255, 255)
};

static void inline plot8(uint16_t *dataPtr, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper)
{
    Q_ASSERT(ink < 16 && paper < 16);
    uint16_t inkColor = palette[ink];
    uint16_t paperColor = palette[paper];
    *dataPtr++ = (data & 0x80) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x40) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x20) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x10) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x08) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x04) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x02) ? inkColor : paperColor;
    *dataPtr   = (data & 0x01) ? inkColor : paperColor;
}

inline QImage spectrumScreen2Image(const char *screen)
{
    if (!screen)
        return QImage();

    const char *colors = screen + 32 * 24 * 8;
    QImage ret(256 ,192, QImage::Format_RGB16);
    uint16_t *bits = (uint16_t *)ret.bits();
    uint16_t pitch = ret.bytesPerLine() / 2;
    for (int screenParts = 0; screenParts < 3; screenParts++) {
        for (int screenRows = 0; screenRows < 8; screenRows++) {
            for (int x = 0; x < 32; x++) {
                libspectrum_byte ink = (*colors & 0x07) | ( (*colors & 0x40) >> 3 );
                libspectrum_byte paper = (*colors++ & ( 0x0f << 3 ) ) >> 3;
                for (int y = 0; y < 8; y++)
                    plot8(bits + x * 8 + y * pitch, screen[x + y * 32 * 8], ink, paper);
            }
            screen += 32;
            bits += pitch * 8;
        }
        screen += 32 * 8 * 7;
    }
    return ret;
}

QImage buff2Image(const unsigned char *buffer, size_t bufferSize, const QString &fileName)
{
    BlockErrors silenter;
    QImage ret;
    libspectrum_id_t fileType;
    libspectrum_class_t fileClass;
    libspectrum_error error = libspectrum_identify_file_with_class(&fileType, &fileClass, fileName.toUtf8().constData(), buffer, bufferSize);
    if (error != LIBSPECTRUM_ERROR_NONE)
        return ret;

    if (fileClass != LIBSPECTRUM_CLASS_SNAPSHOT) {
        switch (fileClass) {
        case LIBSPECTRUM_CLASS_TAPE: {
            // try to find first block that has the screen size

            libspectrum_tape *tape = libspectrum_tape_alloc();
            libspectrum_tape_iterator iterator;

            libspectrum_tape_read(tape, buffer, bufferSize, LIBSPECTRUM_ID_UNKNOWN, fileName.toUtf8().constData());

            for (auto block = libspectrum_tape_iterator_init(&iterator, tape); block;
                 block = libspectrum_tape_iterator_next( &iterator )) {
                if (libspectrum_tape_block_data_length(block) == SPECTRUM_TAPE_BLOCK_SCREEN_SIZE) {
                    ret = spectrumScreen2Image((const char*)libspectrum_tape_block_data(block) + 1);
                    break;
                }
            }
            libspectrum_tape_free(tape);
        }
            break;

        case LIBSPECTRUM_CLASS_DISK_DIDAKTIK:
        case LIBSPECTRUM_CLASS_DISK_GENERIC:
        case LIBSPECTRUM_CLASS_DISK_OPUS:
        case LIBSPECTRUM_CLASS_DISK_PLUS3:
        case LIBSPECTRUM_CLASS_DISK_PLUSD:
        case LIBSPECTRUM_CLASS_DISK_TRDOS:
            ret = QImage(":/images/floppy.svg");

        default:
            break;
        }

        return ret;
    }

    libspectrum_snap *snap = libspectrum_snap_alloc();
    if (!snap)
        return ret;

    error = libspectrum_snap_read(snap, buffer, bufferSize, fileType, fileName.toUtf8().constData());
    if (error == LIBSPECTRUM_ERROR_NONE)
        ret = spectrumScreen2Image((const char*)libspectrum_snap_pages(snap, 5));

    libspectrum_snap_free(snap);
    return ret;
}


QImage buff2Image(const QByteArray &buffer, const QString &fileName)
{
    return buff2Image((const unsigned char *)buffer.constData(), buffer.size(), fileName);
}

QImage scr2Image(const QByteArray &buffer)
{
    return scr2Image(buffer.constData(), buffer.size());
}

QImage scr2Image(const char *buffer, size_t bufferSize)
{
    if (bufferSize < SPECTRUM_TAPE_BLOCK_SCREEN_SIZE - 2)
        return QImage();
    return spectrumScreen2Image(buffer);
}

QImage SpectrumScreenImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(size)
    Q_UNUSED(requestedSize)

    if (id.startsWith("compressed64:")) {
        auto pos = id.indexOf(QLatin1Char(':'), 13);
        if (pos != -1) {
            bool ok;
            int delta = id.mid(13, pos - 13).toInt(&ok);
            if (ok && delta >=0) {
                auto buff = qUncompress(QByteArray::fromBase64(id.mid(pos + 1).toLatin1(), QByteArray::Base64UrlEncoding));
                if (buff.size() - delta >= 0) {
                    auto img = scr2Image(buff.constData() + delta, buff.size() - delta);
                    if (!img.isNull())
                        return img;
                }
            }
            return QIcon(":/images/binary.svg").pixmap(QSize(256, 192)).toImage();
        }
    }

    QFileInfo inf(id);
    if (inf.isDir())
        return QIcon(":/images/folder.svg").pixmap(QSize(256, 192)).toImage();
    if (!inf.isFile())
        return QIcon(":/images/cassette-wait.svg").pixmap(QSize(256, 192)).toImage();

    QFile file(id);
    if (!file.open(QIODevice::ReadOnly))
        return QImage(":/images/cassette-broken.svg");

    QImage ret = buff2Image(file.readAll(), inf.fileName());
    if (ret.isNull())
        return QImage(":/images/cassette-unknown.svg");
    return ret;
}
