/* fusecreen.h: fuse screen texture

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

#ifndef FUSETEXTURE_H
#define FUSETEXTURE_H

#include <libspectrum.h>

#include <qopengl.h>
#include <QMutex>
#include <QSGTexture>

#include <memory>

class FuseTexture : public QSGTexture
{
    Q_OBJECT
private:
    FuseTexture();
    ~FuseTexture();
public:
    static FuseTexture *instance();

    // QSGTexture interface
public:
    void resize(uint32_t w, uint32_t h);
    void rescale();
    int textureId() const;
    QSize textureSize() const;
    QSize imageSize() const;
    bool hasAlphaChannel() const;
    bool hasMipmaps() const;
    void bind();

    void putpixel( int x, int y, int colour );
    void plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper);
    void plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper);
    void update(int x, int y, int w, int h);
    void frameEnd();
    void saveScreen();
    void restoreScreen();

private:
    QRect updateGlPixels();

signals:
    void needsUpdate();
    void sizeChanged(const QSizeF&);
    void screenGeometryChanged();

private:
    GLuint m_textureId = 0;
    QSize m_texSize;
    uint32_t m_width = 0, m_height = 0, m_scale = 0;
    uint32_t *m_spectrumPixels = nullptr, *m_spectrumScaledPixels = nullptr, *m_glPixels = nullptr, *m_savedSpectrumPixels = nullptr;
    QRect m_updateRect;
    bool m_recreate = false;
    bool m_update = false;
    bool m_fullScreen = false;
    QMutex m_copyPixelsMutex;
    QMutex m_syncVars;
};

#endif // FUSETEXTURE_H
