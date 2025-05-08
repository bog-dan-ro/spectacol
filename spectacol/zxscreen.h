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

#include "fusesettings.h"

#include <QQmlEngine>
#include <QQuickRhiItem>

#include <vector>

class DisassambleModel;
class BreakpointsModel;
class QAbstractItemModel;

class ZxScreen : public QQuickRhiItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY screenChanged)
    Q_PROPERTY(bool smoothScaling READ smoothScaling WRITE setSmoothScaling NOTIFY smoothScalingChanged)

public:
    explicit ZxScreen(QQuickItem *parent = nullptr);

    bool fullScreen() const;
    void setFullScreen(bool fullScreen);

    bool smoothScaling() const;
    void setSmoothScaling(bool smoothScaling);

public slots:
    void updateFillMode();

signals:
    void screenChanged();
    void smoothScalingChanged(bool smoothScaling);

protected:
    // QQuickRhiItem interface
    QQuickRhiItemRenderer *createRenderer() final;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void mousePressEvent(QMouseEvent *event) final;
    void mouseMoveEvent(QMouseEvent *event) final;
    void mouseReleaseEvent(QMouseEvent *event) final;

private:
    QSizeF m_textureSize;
    qreal m_devicePixelRatio = 1.0;
    FuseSettings::FillMode m_fillMode = FuseSettings::PreserveAspectFit;
    bool m_smoothScaling = false;
    QPointer<QQuickItem> m_parentItem;
};
