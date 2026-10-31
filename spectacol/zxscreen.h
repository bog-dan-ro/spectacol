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

#include "zxsettings.h"

#include <QQmlEngine>
#include <QQuickItem>

#include <optional>

class ZxScreen : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
    // Releases the Kempston mouse while something covers the screen (pages, menus)
    Q_PROPERTY(bool mouseGrabSuspended READ mouseGrabSuspended WRITE setMouseGrabSuspended NOTIFY mouseGrabSuspendedChanged)

public:
    explicit ZxScreen(QQuickItem *parent = nullptr);

    bool fullScreen() const;
    void setFullScreen(bool fullScreen);

    bool mouseGrabSuspended() const;
    void setMouseGrabSuspended(bool suspended);

signals:
    void fullScreenChanged(bool fullScreen);
    void mouseGrabSuspendedChanged(bool mouseGrabSuspended);

protected:
    // QQuickItem interface
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) final;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) final;
    void itemChange(ItemChange change, const ItemChangeData &value) final;
    void mousePressEvent(QMouseEvent *event) final;
    void mouseMoveEvent(QMouseEvent *event) final;
    void mouseReleaseEvent(QMouseEvent *event) final;
    void hoverEnterEvent(QHoverEvent *event) final;
    void hoverMoveEvent(QHoverEvent *event) final;

private:
    QRectF imageRect(QSize imageSize) const;
    void updateImplicitSize();
    void updateMouseGrab(bool grabbed);
    void sendMouseMotion(QPointF position);

private:
    QSize m_imageSize;
    ZxSettings::FillMode m_fillMode = ZxSettings::PreserveAspectFit;
    bool m_smoothScaling = false;
    bool m_fullScreen = false;
    bool m_mouseGrabSuspended = false;
    std::optional<QPointF> m_lastMousePosition = std::nullopt;
    QPointF m_mouseMotionRemainder;
};
