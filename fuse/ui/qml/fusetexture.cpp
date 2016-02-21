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
#include "xbrz.h"

#include <QSemaphore>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QFile>

#include <vector>

#include <fuse.h>
#include <settings.h>
#include <ui/uidisplay.h>
#include <ui/widget/widget.h>

#ifdef _OPENMP
   #include <omp.h>
#else
   #define omp_get_max_threads() 1
#endif

#include "qmlui.h"

static QSemaphore s_semaphore;

extern "C" int uidisplay_init( int width, int height )
{
    FuseTexture::instance()->resize(width, height);
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
    QSettings s;
    m_scale = s.value("scale", 4).toInt();

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
        m_spectrumPixels = new uint32_t[m_width * m_height];
    }
    rescale();
}

void FuseTexture::rescale()
{
    QMutexLocker lock(&m_syncVars);

    delete[] m_spectrumScaledPixels;
    if (m_scale == 1)
        m_spectrumScaledPixels = nullptr;
    else
        m_spectrumScaledPixels = new uint32_t[m_width * m_scale * m_height * m_scale];

    m_texSize = QSize(nextpow2(m_width * m_scale), nextpow2(m_height * m_scale));
    delete[] m_glPixels;
    m_glPixels = new uint32_t[m_texSize.width() * m_texSize.height()];

    m_recreate = true;
    m_updateRect = QRect(0, 0, m_width, m_height);
    emit sizeChanged(imageSize());
    emit screenGeometryChanged();
    emit needsUpdate();
}

void FuseTexture::rescale(uint32_t scale)
{
    m_scale = scale;
    {
        QSettings s;
        s.setValue("scale", scale);
    }
    rescale();
}

int FuseTexture::scale() const
{
    QMutexLocker lock(&m_syncVars);
    return m_scale;
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

    GLint format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

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
    m_savedSpectrumPixels = new uint32_t[m_width * m_height];
    memcpy(m_savedSpectrumPixels, m_spectrumPixels, sizeof(uint16_t) * m_width * m_height);
}

void FuseTexture::restoreScreen()
{
    memcpy(m_spectrumPixels, m_savedSpectrumPixels, sizeof(uint16_t) * m_width * m_height);
    update(0, 0, m_width, m_height);
}

static const uint32_t palette[16] = {
    0x000000,
    0xCD0000,
    0x0000CD,
    0xCD00CD,
    0x00CD00,
    0xCDCD00,
    0x00CDCD,
    0xCDCDCD,
    0x000000,
    0xFF0000,
    0x0000FF,
    0xFF00FF,
    0x00FF00,
    0xFFFF00,
    0x00FFFF,
    0xFFFFFF
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
    uint32_t inkColor = palette[ink];
    uint32_t paperColor = palette[paper];
    uint32_t *dataPtr = m_spectrumPixels + x + m_width * y;
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
    uint32_t inkColor = palette[ink];
    uint32_t paperColor = palette[paper];
    uint32_t *dataPtr = m_spectrumPixels;
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
    if (!m_spectrumPixels || !m_glPixels || !m_scale)
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

    uint32_t *glPixels = m_glPixels;
    uint32_t *spectrumPixels = nullptr;
    const int specPitch = m_width * m_scale;

    if (m_scale != 1) {
        if (y > 4)
            y -= 4;
        else
            y = 0;
        if (h < int(m_height - 4))
            h += 4;
        else
            h = m_height;

        QMutexLocker lockCopy(&m_copyPixelsMutex);
        const int dest_x = x * m_scale;
        const int dest_y = y * m_scale;
        static xbrz::ScalerCfg cfg;
        const int maxThreads = omp_get_max_threads();
        const int step = h / maxThreads;
        if (step) {
            #pragma omp parallel for schedule(static)
            for (int line = y; line < y + h; line += step)
                xbrz::scale(m_scale, m_spectrumPixels, m_spectrumScaledPixels, m_width, m_height, xbrz::ColorFormat::RGB, cfg, line, line + step);
        }

        const int lines = h - step * maxThreads;
        if (lines)
            xbrz::scale(m_scale, m_spectrumPixels, m_spectrumScaledPixels, m_width, m_height, xbrz::ColorFormat::RGB, cfg, y + h - lines, y + h);

        spectrumPixels = m_spectrumScaledPixels + dest_x + dest_y * specPitch;
    } else {
        spectrumPixels = m_spectrumPixels + x + y * specPitch;
    }

    int glPitch = m_recreate ? m_texSize.width() : w * m_scale;
    const size_t copy_sz = w * m_scale * sizeof(uint32_t);
    for (u_int32_t i = 0; i < h * m_scale; i++) {
        memcpy(glPixels, spectrumPixels, copy_sz);
        glPixels += glPitch;
        spectrumPixels += specPitch;
    }
    m_update = false;
    m_recreate = false;
    return QRect(x * m_scale, y * m_scale, w * m_scale, h * m_scale);
}
