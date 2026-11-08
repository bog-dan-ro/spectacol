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

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>
#include <array>
#include <vector>
#include <zxkit/core/Emulator.h>
#include <zxkit/core/Shared.h>

class DebuggerMemoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use the ZxEmulator debugger models")
    Q_PROPERTY(QStringList bankNames READ bankNames NOTIFY banksChanged FINAL)
    Q_PROPERTY(int bankId READ bankId WRITE setBankId NOTIFY banksChanged FINAL)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged FINAL)
    Q_PROPERTY(int cursor READ cursor WRITE setCursor NOTIFY cursorChanged FINAL)
    Q_PROPERTY(int address READ address NOTIFY cursorChanged FINAL)
    Q_PROPERTY(int addressLimit READ addressLimit NOTIFY banksChanged FINAL)
    Q_PROPERTY(int byteValue READ byteValue NOTIFY cursorChanged FINAL)
    Q_PROPERTY(QString addressText READ addressText NOTIFY cursorChanged FINAL)
    Q_PROPERTY(bool editable READ editable WRITE setEditable NOTIFY editableChanged FINAL)
    Q_PROPERTY(bool writable READ writable NOTIFY cursorChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged FINAL)
public:
    enum Role : int { Address = Qt::UserRole + 1, AddressText, Cells, Ascii };
    explicit DebuggerMemoryModel(const zxkit::Shared<zxkit::Emulator> &core,
                                 QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QStringList bankNames() const
    {
        return m_banks;
    }
    int bankId() const
    {
        return m_bank;
    }
    void setBankId(int bank);
    int columns() const
    {
        return m_columns;
    }
    void setColumns(int columns);
    int cursor() const
    {
        return m_cursor;
    }
    void setCursor(int cursor);
    int address() const
    {
        return m_base + m_cursor;
    }
    int addressLimit() const
    {
        return m_size - 1;
    }
    int byteValue() const
    {
        return m_bytes[m_cursor];
    }
    QString addressText() const;
    bool editable() const
    {
        return m_editable;
    }
    bool writable() const
    {
        return m_writable;
    }
    void setEditable(bool editable);
    QString error() const
    {
        return m_error;
    }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool goTo(const QString &hex);
    Q_INVOKABLE void moveCursor(int delta);
    Q_INVOKABLE bool writeByte(const QString &hex, bool allowRom = false);
    static bool parseHex(const QString &text, quint32 limit, quint32 &value);
signals:
    void banksChanged();
    void columnsChanged();
    void cursorChanged();
    void editableChanged();
    void errorChanged();

private:
    bool fail(const QString &text);
    zxkit::Shared<zxkit::Emulator> m_core;
    std::array<uint8_t, 256> m_bytes{};
    QStringList m_banks;
    QString m_error;
    int m_bank = -1;
    int m_base = 0;
    int m_cursor = 0;
    int m_size = 65536;
    int m_count = 256;
    int m_columns = 16;
    bool m_editable = true;
    bool m_writable = false;
};

// Frozen on refresh/pause. Reads raw register latches and effective MMU mappings,
// never CPU I/O ports or register handlers with side effects.
class DebuggerStateModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use the ZxEmulator debugger models")
    Q_PROPERTY(bool available READ available NOTIFY availableChanged FINAL)
public:
    enum class Kind { NextRegisters, Mmu };
    enum Role : int { Address = Qt::UserRole + 1, AddressText, ValueText, Details };
    DebuggerStateModel(const zxkit::Shared<zxkit::Emulator> &core, Kind kind,
                       QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool available() const
    {
        return m_available;
    }
    Q_INVOKABLE void refresh();
signals:
    void availableChanged();

private:
    struct Row
    {
        int address;
        QString location, value, details;
    };
    zxkit::Shared<zxkit::Emulator> m_core;
    Kind m_kind;
    bool m_available = false;
    std::vector<Row> m_rows;
};
