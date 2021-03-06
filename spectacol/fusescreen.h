/* fusecreen.h: QML Item representing the fuse screen

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

#ifndef FUSESCREEN_H
#define FUSESCREEN_H

#include "fusesettings.h"

#include <QQuickItem>

#include <vector>

class DisassambleModel;
class BreakpointsModel;
class QAbstractItemModel;

class FuseScreen : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY screenChanged)
    Q_PROPERTY(bool smoothScaling READ smoothScaling WRITE setSmoothScaling NOTIFY smoothScalingChanged)

public:
    FuseScreen();

    bool fullScreen() const;
    void setFullScreen(bool fullScreen);

    bool smoothScaling() const;
    void setSmoothScaling(bool smoothScaling);

public slots:
    void updateFillMode();


signals:
    void screenChanged();
    void smoothScalingChanged(bool smoothScaling);

    // QQuickItem interface
protected:
    QSGNode *updatePaintNode(QSGNode *n, UpdatePaintNodeData *);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    qreal m_aspectRatio = 4/3;
    QSize m_imageSize = QSize(1, 1);
    FuseSettings::FillMode m_fillMode = FuseSettings::PreserveAspectFit;
    bool m_smoothScaling = false;
};

#endif // FUSESCREEN_H
