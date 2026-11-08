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

#include <QQmlEngine>

#include <QObject>

class ZxEmulator;
class Disks : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use ZxEmulator.disks instead")
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit Disks(ZxEmulator *parent = nullptr);

    int count() const;

public slots:
    void insertDisk(int idx, const QString &path);
    void ejectDisk(int idx);

signals:
    void countChanged();

private:
    void updateCount();

    ZxEmulator *m_zxEmulator = nullptr;
    int m_count = 0;
};
