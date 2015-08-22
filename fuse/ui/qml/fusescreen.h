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
#include <vector>
class DisassambleModel;
class BreakpointsModel;
class QAbstractItemModel;

class FuseScreen : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY screenChanged)
    Q_PROPERTY(QUrl dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)
    Q_PROPERTY(QStringList filtersModel READ filtersModel)
    Q_PROPERTY(int selectedFilterIndex READ selectedFilterIndex WRITE setSelectedFilterIndex NOTIFY selectedFilterIndexChanged)

    /* regs properties */
    Q_PROPERTY(QString PC READ PC WRITE setPC NOTIFY registersChanged)
    Q_PROPERTY(QString SP READ SP WRITE setSP NOTIFY registersChanged)
    Q_PROPERTY(QString IX READ IX WRITE setIX NOTIFY registersChanged)
    Q_PROPERTY(QString IY READ IY WRITE setIY NOTIFY registersChanged)
    Q_PROPERTY(QString R READ R WRITE setR NOTIFY registersChanged)
    Q_PROPERTY(QString I READ I WRITE setI NOTIFY registersChanged)
    Q_PROPERTY(QString R7 READ R7 WRITE setR7 NOTIFY registersChanged)
    Q_PROPERTY(QString IFF1 READ IFF1 WRITE setIFF1 NOTIFY registersChanged)
    Q_PROPERTY(QString IFF2 READ IFF2 WRITE setIFF2 NOTIFY registersChanged)
    Q_PROPERTY(QString IM READ IM WRITE setIM NOTIFY registersChanged)

    Q_PROPERTY(QString AF READ AF WRITE setAF NOTIFY registersChanged)
    Q_PROPERTY(QString BC READ BC WRITE setBC NOTIFY registersChanged)
    Q_PROPERTY(QString DE READ DE WRITE setDE NOTIFY registersChanged)
    Q_PROPERTY(QString HL READ HL WRITE setHL NOTIFY registersChanged)
    Q_PROPERTY(QString AF_ READ AF_ WRITE setAF_ NOTIFY registersChanged)
    Q_PROPERTY(QString BC_ READ BC_ WRITE setBC_ NOTIFY registersChanged)
    Q_PROPERTY(QString DE_ READ DE_ WRITE setDE_ NOTIFY registersChanged)
    Q_PROPERTY(QString HL_ READ HL_ WRITE setHL_ NOTIFY registersChanged)

public:
    enum ErrorLevel {
        Info,
        Warning,
        Error
    };

    Q_ENUMS(ErrorLevel)

public:
    FuseScreen();
    ~FuseScreen();
    bool paused() const;
    void setPaused(bool paused);

    bool fullScreen() const;
    void setFullScreen(bool fullScreen);

    QUrl dataPath() const;
    void setDataPath(const QUrl &dataPath);

    bool saveSnapshotEnabled() const;

    QStringList filtersModel() const;

    int selectedFilterIndex() const;
    void setSelectedFilterIndex(int selectedFilterIndex);

    QString PC() const;
    void setPC(const QString &value);
    QString SP() const;
    void setSP(const QString &value);
    QString IX() const;
    void setIX(const QString &value);
    QString IY() const;
    void setIY(const QString &value);
    QString R() const;
    void setR(const QString &value);
    QString I() const;
    void setI(const QString &value);
    QString R7() const;
    void setR7(const QString &value);
    QString IFF1() const;
    void setIFF1(const QString &value);
    QString IFF2() const;
    void setIFF2(const QString &value);
    QString IM() const;
    void setIM(const QString &value);

    QString AF() const;
    void setAF(const QString &value);
    QString BC() const;
    void setBC(const QString &value);
    QString DE() const;
    void setDE(const QString &value);
    QString HL() const;
    void setHL(const QString &value);
    QString AF_() const;
    void setAF_(const QString &value);
    QString BC_() const;
    void setBC_(const QString &value);
    QString DE_() const;
    void setDE_(const QString &value);
    QString HL_() const;
    void setHL_(const QString &value);

public slots:
    QUrl snapshotsPath() const;
    void load(const QUrl &filePath);
    void save(const QUrl &filePath);
    void reset();
    void hardReset();
    void quickSaveSnapshot();
    void quickLoadSnapshot();
    void disassamble();
    void disassamble(uint16_t address, uint16_t delta = -10, uint16_t length = 0xff);
    QString snapshotFileName(bool addExtension = true) const;
    QAbstractItemModel *disassambleModel() const;
    QAbstractItemModel *breakpointsModel() const;


signals:
    void pausedChanged();
    void screenChanged();
    void dataPathChanged();
    void saveSnapshotEnabledChanged();
    void selectedFilterIndexChanged();
    void registersChanged();
    void showDebugger();
    void hideDebugger();

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
    void updateScalers() const;

private:
    qreal m_aspectRatio = 4/3;
    bool m_fullScreen = false;
    QString m_loadedFileName;
    mutable std::vector<int> m_supportedScalers;
    DisassambleModel *m_disassambleModel;
    BreakpointsModel *m_breakpointsModel;
};

extern FuseScreen *g_fuseEmulator;

#endif // FUSESCREEN_H
