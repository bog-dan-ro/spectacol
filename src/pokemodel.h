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

#include <QQmlEngine>

#include <QAbstractListModel>

#include <QStringList>
#include <string>
#include <vector>
#include <cstdint>
#include <zxkit/core/Emulator.h>
#include <zxkit/core/Shared.h>

// The pokes (trainers) the user applied to the running program
class PokeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ZxEmulator.pokes")
    Q_PROPERTY(QStringList bankNames READ bankNames NOTIFY banksChanged FINAL)
    Q_PROPERTY(int selectedBank READ selectedBank WRITE setSelectedBank NOTIFY selectedBankChanged
                       FINAL)
    Q_PROPERTY(int addressLimit READ addressLimit NOTIFY selectedBankChanged FINAL)

    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString pokesFolder READ pokesFolder WRITE setPokesFolder NOTIFY sourceChanged FINAL)
public:
    QString sourcePath() const
    {
        return m_source;
    }
    QString pokesFolder() const
    {
        return m_folder;
    }
    void setPokesFolder(const QString &folder);
    Q_INVOKABLE bool choosePokesFolder(const QString &folder);
    void gameLoaded(const QString &path);
    Q_INVOKABLE bool loadPok(const QString &path);
    Q_INVOKABLE QVariantList requestedValues(int row) const;
    enum Roles : int { Label = Qt::DisplayRole, Active = Qt::UserRole + 1, Disabled };
    Q_ENUM(Roles)

public:
    explicit PokeModel(const zxkit::Shared<zxkit::Emulator> &core, QObject *parent = nullptr);
    QStringList bankNames() const
    {
        return m_bankNames;
    }
    int selectedBank() const
    {
        return m_selectedBank;
    }
    void setSelectedBank(int index);
    int addressLimit() const;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void update();
    bool addPoke(int bank, int address, int value);
    void refreshBanks();
    bool setActive(int index, bool active = true, const QVariantList &values = {});
    void clear();

signals:
    void sourceChanged();
    void failed(const QString &message);
    void memoryChanged();
    void banksChanged();
    void selectedBankChanged();

private:
    struct Bank
    {
        int id;
        int maximum;
    };
    QStringList m_bankNames;
    std::vector<Bank> m_banks;
    int m_selectedBank = 0;
    struct Patch
    {
        int bank = 0, address = 0;
        uint8_t original = 0;
        uint16_t value = 0;
        bool resolved = false;
    };
    struct Trainer
    {
        QString name;
        std::vector<Patch> patches;
        std::string machine;
        bool active = false;
        bool disabled = false;
    };
    using Trainers = std::vector<Trainer>;

private:
    Trainers m_trainers;
    zxkit::Shared<zxkit::Emulator> m_core;
    QString m_source, m_folder, m_game;
    bool fail(const QString &message);
    void discover();
};
