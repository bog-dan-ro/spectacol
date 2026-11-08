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
#include <QMap>
#include <zxkit/core/Emulator.h>

class DebugSymbols : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ZxEmulator.symbols")
    Q_PROPERTY(QString path READ path NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY changed FINAL)
    Q_PROPERTY(int count READ count NOTIFY changed FINAL)
public:
    enum Role { Label = Qt::UserRole + 1, Address, Location, Page };
    explicit DebugSymbols(QObject *parent = nullptr) : QAbstractListModel(parent) { }
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString path() const
    {
        return m_path;
    }
    QString error() const
    {
        return m_error;
    }
    QString search() const
    {
        return m_search;
    }
    int count() const
    {
        return int(m_labels.size());
    }
    void setSearch(const QString &search);
    Q_INVOKABLE bool load(const QString &path);
    Q_INVOKABLE void clear();
    // The caller already holds the emulator lock. Data belongs to the GUI thread.
    QString annotation(const zxkit::Emulator &core, uint16_t address) const;
    int resolve(const zxkit::Emulator &core, const QString &label) const;
    int rowAddress(const zxkit::Emulator &core, int row) const;
signals:
    void changed();

private:
    struct LabelData
    {
        QString name;
        uint16_t address;
        int page;
    };
    bool mapped(const zxkit::Emulator &core, uint16_t address, int page) const;
    bool fail(const QString &error);
    void refilter();
    QString m_path, m_error, m_search;
    QList<LabelData> m_labels;
    QList<int> m_visible;
    QMap<int, zxkit::SymbolTable> m_tables;
    int m_pageSize = 16384, m_pageCount = 0;
};
