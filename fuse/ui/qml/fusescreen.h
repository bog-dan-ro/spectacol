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

#include <QQuickItem>

class FuseScreen : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY screenChanged)
    Q_PROPERTY(QUrl dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)

public:
    enum ErrorLevel {
        Info,
        Warning,
        Error
    };

    Q_ENUMS(ErrorLevel)
public:
    FuseScreen();
    bool paused() const;
    void setPaused(bool paused);

    bool fullScreen() const;
    void setFullScreen(bool fullScreen);

    QUrl dataPath() const;
    void setDataPath(const QUrl &dataPath);

    bool saveSnapshotEnabled() const;

public slots:
    QUrl snapshotsPath() const;
    void load(const QUrl &filePath);
    void save(const QUrl &filePath);
    void quickSaveSnapshot();
    void quickLoadSnapshot();
    QString snapshotFileName(bool addExtension = true) const;

signals:
    void pausedChanged();
    void screenChanged();
    void dataPathChanged();
    void saveSnapshotEnabledChanged();

    void error(ErrorLevel level, const QString &message);

    // QQuickItem interface
protected:
    QSGNode *updatePaintNode(QSGNode *n, UpdatePaintNodeData *);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    qreal m_aspectRatio = 4/3;
    bool m_fullScreen = false;
    QString m_loadedFileName;
};

extern FuseScreen *g_fuseEmulator;

#endif // FUSESCREEN_H
