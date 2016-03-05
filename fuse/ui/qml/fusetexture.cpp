/* fusetexture.cpp: fuse screen texture

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

#include "fusetexture.h"
#include "fuseemulator.h"

#include <QSemaphore>
#include <QDebug>
#include <QTimer>

#include <vector>

#include <fuse.h>
#include <settings.h>
#include <ui/uidisplay.h>
#include <ui/widget/widget.h>

QSemaphore s_semaphore;

#include "qmlui.h"

extern "C" int uidisplay_init( int width, int height )
{
    FuseTexture::instance()->resize(width, height);

    scaler_register_clear();
    scaler_select_bitformat( 565 );		/* 16bit always */

    scaler_register( SCALER_NORMAL );
#ifndef Q_OS_ANDROID
    scaler_register( SCALER_2XSAI );
    scaler_register( SCALER_SUPER2XSAI );
    scaler_register( SCALER_SUPEREAGLE );
    scaler_register( SCALER_ADVMAME2X );
    scaler_register( SCALER_ADVMAME3X );
    scaler_register( SCALER_DOTMATRIX );
    scaler_register( SCALER_PALTV );
    scaler_register( SCALER_HQ2X );
    if( machine_current->timex ) {
      scaler_register( SCALER_HALF );
      scaler_register( SCALER_HALFSKIP );
      scaler_register( SCALER_TIMEXTV );
      scaler_register( SCALER_TIMEX1_5X );
    } else {
      scaler_register( SCALER_TV2X );
      scaler_register( SCALER_TV3X );
      scaler_register( SCALER_PALTV2X );
      scaler_register( SCALER_PALTV3X );
      scaler_register( SCALER_HQ3X );
    }
#endif

    if( scaler_is_supported( current_scaler ) ) {
      scaler_select_scaler( current_scaler );
    } else {
      scaler_select_scaler( SCALER_NORMAL );
    }

    display_ui_initialised = 1;
    s_semaphore.release();
    return 0;
}

extern "C" int uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  FuseTexture::instance()->rescale();

  fuse_emulation_unpause();

  return 0;
}

extern "C" int uidisplay_end( void )
{
    return 0;
}

extern "C" void uidisplay_putpixel( int x, int y, int colour )
{
    FuseTexture::instance()->putpixel(x, y, colour);
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
extern "C" void uidisplay_plot8( int x, int y, libspectrum_byte data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
    FuseTexture::instance()->plot8(x, y, data, ink, paper);
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
extern "C" void uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
    FuseTexture::instance()->plot16(x, y, data, ink, paper);
}

extern "C" void uidisplay_area( int x, int y, int w, int h )
{
    FuseTexture::instance()->update(x, y, w, h);
}

extern "C" void uidisplay_frame_end( void )
{
    FuseTexture::instance()->frameEnd();
}

extern "C" void uidisplay_frame_save( void )
{
    FuseTexture::instance()->saveScreen();
}

extern "C" void uidisplay_frame_restore( void )
{
    FuseTexture::instance()->restoreScreen();
}

FuseTexture::FuseTexture()
{
    g_fuseEmulator->startFuseThread();
}

FuseTexture::~FuseTexture()
{
    delete[] m_spectrumPixels;
    delete[] m_spectrumScaledPixels;
    delete[] m_savedSpectrumPixels;
    delete[] m_glPixels;
}

FuseTexture *FuseTexture::instance()
{
    static FuseTexture self;
    return &self;
}

static uint32_t nextpow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}

void FuseTexture::resize(uint32_t w, uint32_t h)
{
    {
        QMutexLocker lock(&m_syncVars);
        m_width = w;
        m_height = h;
        delete[] m_spectrumPixels;
        m_spectrumPixels = new uint16_t[m_width * m_height];
    }
    rescale();
}

void FuseTexture::rescale()
{

    uint32_t scale = scaler_get_scaling_factor( current_scaler );
    if (!scale)
        scale = 1;

    QMutexLocker lock(&m_syncVars);
    if (m_scale == scale)
        return;

    m_scale = scale;
    delete[] m_spectrumScaledPixels;
    if (m_scale == 1) {
        m_spectrumScaledPixels = nullptr;
    } else {
        m_spectrumScaledPixels = new uint16_t[m_width * m_scale * m_height * m_scale];
    }

    m_texSize = QSize(nextpow2(m_width * m_scale), nextpow2(m_height * m_scale));
    delete[] m_glPixels;
    m_glPixels = new uint16_t[m_texSize.width() * m_texSize.height()];

    m_recreate = true;
    m_updateRect = QRect(0, 0, m_width, m_height);
    emit sizeChanged(imageSize());
    emit screenGeometryChanged();
    emit needsUpdate();
}


int FuseTexture::textureId() const
{
    return m_textureId;
}

QSize FuseTexture::textureSize() const
{
    return m_texSize;
}

QSize FuseTexture::imageSize() const
{
    if (!m_scale)
        s_semaphore.acquire();

    return QSize(m_width * m_scale, m_height * m_scale);
}

bool FuseTexture::hasAlphaChannel() const
{
    return false;
}

bool FuseTexture::hasMipmaps() const
{
    return false;
}

void FuseTexture::bind()
{
#ifndef QT_NO_DEBUG
    while (glGetError() != GL_NO_ERROR) { }
#endif
    bool recreate;
    {
        QMutexLocker lock(&m_syncVars);
        if (!m_recreate && !m_update && m_textureId) {
            glBindTexture(GL_TEXTURE_2D, m_textureId);
            return;
        }
        if (!m_glPixels)
            return;
        recreate = m_recreate;
    }

    GLint format = GL_RGB;
    GLenum type = GL_UNSIGNED_SHORT_5_6_5;

    QRect updateRect = updateGlPixels();

    if (recreate || !m_textureId) {
        if (m_textureId) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &m_textureId);
        }
        glGenTextures(1, &m_textureId);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, m_texSize.width(), m_texSize.height(), 0, format, type, m_glPixels);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_texSize.width(), m_texSize.height(), format, type, m_glPixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, updateRect.x(), updateRect.y(), updateRect.width(), updateRect.height(), format, type, m_glPixels);
    }

#ifndef QT_NO_DEBUG
    // Gracefully fail in case of an error...
    GLuint error = glGetError();
    if (error != GL_NO_ERROR) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
        return;
    }
#endif
    updateBindOptions(true);
}

void FuseTexture::update(int x, int y, int w, int h)
{
    QMutexLocker lock(&m_syncVars);
    m_updateRect = m_updateRect.united(QRect(x, y, w, h));
    m_update = true;
}

void FuseTexture::frameEnd()
{
    if (m_fullScreen != bool(settings_current.full_screen)) {
        m_fullScreen = settings_current.full_screen;
        emit screenGeometryChanged();
    }

    QMutexLocker lock(&m_syncVars);
    if (m_update || m_recreate)
        emit needsUpdate();
}

void FuseTexture::saveScreen()
{
    delete []m_savedSpectrumPixels;
    m_savedSpectrumPixels = new uint16_t[m_width * m_height];
    memcpy(m_savedSpectrumPixels, m_spectrumPixels, sizeof(uint16_t) * m_width * m_height);
}

void FuseTexture::restoreScreen()
{
    memcpy(m_spectrumPixels, m_savedSpectrumPixels, sizeof(uint16_t) * m_width * m_height);
    update(0, 0, m_width, m_height);
}

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

void FuseTexture::putpixel(int x, int y, int colour)
{
    QMutexLocker lock(&m_copyPixelsMutex);
    Q_ASSERT(colour < 16);
    Q_ASSERT(uint32_t(x) < m_width && uint32_t(y) < m_height);
    m_spectrumPixels[x + m_width * y] = palette[colour];
}

void FuseTexture::plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper)
{
    QMutexLocker lock(&m_copyPixelsMutex);
    x <<= 3;
    Q_ASSERT(ink < 16 && paper < 16);
    Q_ASSERT(uint32_t(x) < m_width && uint32_t(y) < m_height);
    uint16_t inkColor = palette[ink];
    uint16_t paperColor = palette[paper];
    uint16_t *dataPtr = m_spectrumPixels + x + m_width * y;
    *dataPtr++ = (data & 0x80) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x40) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x20) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x10) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x08) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x04) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x02) ? inkColor : paperColor;
    *dataPtr   = (data & 0x01) ? inkColor : paperColor;
}

void FuseTexture::plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper)
{
    QMutexLocker lock(&m_copyPixelsMutex);
    Q_ASSERT(ink < 16 && paper < 16);
    Q_ASSERT(uint32_t(x) < m_width && uint32_t(y) < m_height);
    uint16_t inkColor = palette[ink];
    uint16_t paperColor = palette[paper];
    uint16_t *dataPtr = m_spectrumPixels;
    dataPtr += x + m_width * y;
    *dataPtr++ = (data & 0x8000) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x4000) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x2000) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x1000) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0800) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0400) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0200) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0100) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0080) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0040) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0020) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0010) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0008) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0004) ? inkColor : paperColor;
    *dataPtr++ = (data & 0x0002) ? inkColor : paperColor;
    *dataPtr   = (data & 0x0001) ? inkColor : paperColor;
}

QRect FuseTexture::updateGlPixels()
{
    QMutexLocker lock(&m_syncVars);
    if (!m_spectrumPixels || !m_glPixels)
        return QRect();

    int x = m_updateRect.x(), y = m_updateRect.y(), w = m_updateRect.width(), h = m_updateRect.height();
    Q_ASSERT(x >= 0);
    Q_ASSERT(y >= 0);
    Q_ASSERT(w >= 0);
    Q_ASSERT(h >= 0);
    m_updateRect = QRect();

    if (!w)
        w = m_width;
    if (!h)
        h = m_height;
    int copy_x = x;
    int copy_y = y;
    int copy_w = w;
    int copy_h = h;
    uint16_t *glPixels = m_glPixels;
    uint16_t *spectrumPixels = nullptr;
    int specPitch = m_width * m_scale;

    if (m_scale != 1) {
        /* Extend the dirty region by 1 pixel for scalers
           that "smear" the screen, e.g. 2xSAI */
        if( scaler_flags & SCALER_FLAGS_EXPAND )
            scaler_expander(&x, &y, &w, &h, m_width, m_height);

        QMutexLocker lockCopy(&m_copyPixelsMutex);
        int dest_x = x * m_scale;
        int dest_y = y * m_scale;
        const libspectrum_byte *src = (const libspectrum_byte *)(m_spectrumPixels + x + y * m_width);
        spectrumPixels = m_spectrumScaledPixels + dest_x + dest_y * specPitch;
        libspectrum_byte *dst = (libspectrum_byte *)(spectrumPixels);
        scaler_proc16(src, m_width * sizeof(uint16_t), dst, specPitch * sizeof(uint16_t), w, h);
        spectrumPixels += abs(copy_y - y) * m_scale * specPitch;
        spectrumPixels += abs(copy_x - x) * m_scale;
    } else {
        spectrumPixels = m_spectrumPixels + x + y * specPitch;
    }

    int glPitch = m_recreate ? m_texSize.width() : copy_w * m_scale;
    const size_t copy_sz = copy_w * m_scale * sizeof(uint16_t);
    for (u_int32_t i = 0; i < copy_h * m_scale; i++) {
        memcpy(glPixels, spectrumPixels, copy_sz);
        glPixels += glPitch;
        spectrumPixels += specPitch;
    }
    m_update = false;
    m_recreate = false;
    return QRect(copy_x * m_scale, copy_y * m_scale, copy_w * m_scale, copy_h * m_scale);
}
