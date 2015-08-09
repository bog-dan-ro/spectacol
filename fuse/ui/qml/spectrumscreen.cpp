#include "spectrumscreen.h"

#include <libspectrum.h>

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

inline QImage spectrumScreen2Image(const unsigned char *screen)
{
    if (!screen)
        return QImage();

    const unsigned char *colors = screen + 32 * 24 * 8;
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
    QImage ret;
    libspectrum_id_t fileType;
    libspectrum_init_t init = libspectrum_default_init();
    libspectrum_init(&init);
    libspectrum_error error = libspectrum_identify_file(init.context, &fileType, fileName.toUtf8().constData(), buffer, bufferSize);
    if (error != LIBSPECTRUM_ERROR_NONE) {
        libspectrum_end(init.context);
        return ret;
    }

    libspectrum_class_t fileClass;
    error = libspectrum_identify_class(init.context, &fileClass, fileType);
    if (error != LIBSPECTRUM_ERROR_NONE || fileClass != LIBSPECTRUM_CLASS_SNAPSHOT) {
        libspectrum_end(init.context);
        return ret;
    }

    libspectrum_snap *snap = libspectrum_snap_alloc(init.context);
    if (!snap) {
        libspectrum_end(init.context);
        return ret;
    }

    error = libspectrum_snap_read(snap, buffer, bufferSize, fileType, fileName.toUtf8().constData());
    if (error == LIBSPECTRUM_ERROR_NONE)
        ret = spectrumScreen2Image(libspectrum_snap_pages(snap, 5));

    libspectrum_snap_free(snap);
    libspectrum_end(init.context);
    return ret;
}


QImage buff2Image(const QByteArray &buffer, const QString &fileName)
{
    return buff2Image((const unsigned char *)buffer.constData(), buffer.size(), fileName);
}
