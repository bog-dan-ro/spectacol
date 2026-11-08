/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include <QMap>
#include <mutex>
#include <vector>

class RomOverrides : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ZxEmulator.roms")
    Q_PROPERTY(QStringList machines READ machines CONSTANT FINAL)
    Q_PROPERTY(int machineIndex READ machineIndex WRITE setMachineIndex NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList romSlots READ romSlots NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged FINAL)
public:
    explicit RomOverrides(QObject *parent = nullptr);
    QStringList machines() const
    {
        return m_names;
    }
    int machineIndex() const
    {
        return m_index;
    }
    void setMachineIndex(int index);
    QVariantList romSlots() const;
    QString error() const
    {
        return m_error;
    }
    Q_INVOKABLE bool useFile(int slot, const QString &path);
    Q_INVOKABLE bool restore(int slot);
    Q_INVOKABLE bool restoreMachine();
    // Called under the emulator lock, possibly on the Engine thread. No file I/O.
    std::vector<uint8_t> resolve(std::string_view machine, std::string_view name,
                                 std::size_t size) const;
signals:
    void changed();
    void errorChanged();

private:
    struct Slot
    {
        QString name;
        qsizetype size;
    };
    struct Override
    {
        QByteArray bytes;
        QString label;
    };
    bool fail(const QString &message);
    QString key(const QString &machine, const QString &name) const;
    QString storedPath(const QString &machine, const QString &name) const;
    QStringList m_ids, m_names;
    QMap<QString, QList<Slot>> m_slots;
    QMap<QString, QByteArray> m_bundled;
    QMap<QString, Override> m_overrides;
    QString m_error;
    int m_index = 0;
    mutable std::mutex m_mutex;
};
