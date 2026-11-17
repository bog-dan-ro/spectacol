/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class GraphicsSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList choices READ choices CONSTANT FINAL)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY changed FINAL)
    Q_PROPERTY(QString activeApi READ activeApi CONSTANT FINAL)
    Q_PROPERTY(QString measurements READ measurements CONSTANT FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY changed FINAL)
    Q_PROPERTY(bool retestPending READ retestPending NOTIFY changed FINAL)
    Q_PROPERTY(bool benchmarkAvailable READ benchmarkAvailable CONSTANT FINAL)

public:
    explicit GraphicsSettings(QObject *parent = nullptr);
    QStringList choices() const;
    int selectedIndex() const;
    void setSelectedIndex(int index);
    QString activeApi() const;
    QString measurements() const;
    bool restartRequired() const;
    bool retestPending() const;
    bool benchmarkAvailable() const;
    Q_INVOKABLE void retest();

signals:
    void changed();

private:
    int m_startupIndex = 0;
};
