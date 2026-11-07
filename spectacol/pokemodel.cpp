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

#include "pokemodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

using namespace Qt::StringLiterals;

PokeModel::PokeModel(const zxkit::Shared<zxkit::Emulator> &core, QObject *parent)
    : QAbstractListModel(parent), m_core(core)
{
    m_folder = QSettings().value(u"Trainers/pokesFolder"_s).toString();
}

int PokeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return int(m_trainers.size());
}

QVariant PokeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || size_t(index.row()) >= m_trainers.size())
        return {};
    const auto &trainer = m_trainers[index.row()];
    switch (role) {
    case Label:
        return trainer.name;
    case Active:
        return trainer.active;
    case Disabled:
        return trainer.disabled;
    default:
        return {};
    }
}

QHash<int, QByteArray> PokeModel::roleNames() const
{
    static QHash<int, QByteArray> roles = { { Label, "label" },
                                            { Active, "active" },
                                            { Disabled, "disabled" } };
    return roles;
}

void PokeModel::update()
{
    beginResetModel();
    endResetModel();
    emit sourceChanged();
}

namespace {
// Bank 0 denotes the CPU's effective write mapping; positive IDs are physical MMU banks.
QString writePoke(zxkit::Emulator &core, int bankId, int address, uint8_t value,
                  uint8_t *previous = nullptr)
{
    if (address < 0)
        return QObject::tr("Invalid address");
    auto &mmu = core.mmu();
    if (!bankId) {
        if (address > 65535)
            return QObject::tr("Address is outside CPU memory");
        const auto &mapping = mmu.effectiveMapping(unsigned(address) / 1024);
        if (!mapping.writable || !mapping.writeBankId)
            return QObject::tr("This CPU address is read-only or unmapped");
        // Save the byte from the write bank, which can differ from the visible read bank.
        if (previous)
            *previous = mmu.bank(mapping.writeBankId).page1k(mapping.writePage1k)[address & 1023];
        mmu.poke(uint16_t(address), value);
    } else {
        if (bankId < 0 || std::size_t(bankId) > mmu.banks().size())
            return QObject::tr("Memory banks changed; choose a bank again");
        auto &bank = mmu.bank(zxkit::BankId(bankId));
        if (!bank.isWritable())
            return QObject::tr("The selected bank is read-only");
        if (std::size_t(address) >= bank.size())
            return QObject::tr("Address is outside the selected bank");
        auto &byte = bank.page1k(unsigned(address) / 1024)[address & 1023];
        if (previous)
            *previous = byte;
        byte = value;
    }
    return {};
}
} // namespace

void PokeModel::refreshBanks()
{
    QStringList names{ tr("CPU address space") };
    std::vector<Bank> banks{ { 0, 65535 } };
    {
        const auto core = m_core.readLock();
        for (const auto &bank : core->mmu().banks()) {
            if (!bank.isWritable() || !bank.size())
                continue;
            names.append(QString::fromUtf8(bank.name().data(), qsizetype(bank.name().size()))
                         + u" (%1 KiB)"_s.arg(bank.size() / 1024));
            banks.push_back({ int(bank.id()), int(bank.size() - 1) });
        }
    }
    m_banks = std::move(banks);
    m_bankNames = std::move(names);
    m_selectedBank = 0;
    emit banksChanged();
    emit selectedBankChanged();
}
void PokeModel::setSelectedBank(int index)
{
    if (index < 0 || std::size_t(index) >= m_banks.size() || index == m_selectedBank)
        return;
    m_selectedBank = index;
    emit selectedBankChanged();
}
int PokeModel::addressLimit() const
{
    return m_selectedBank >= 0 && std::size_t(m_selectedBank) < m_banks.size()
            ? m_banks[m_selectedBank].maximum
            : 0;
}
bool PokeModel::fail(const QString &message)
{
    emit failed(message);
    return false;
}
bool PokeModel::addPoke(int bank, int address, int value)
{
    if (bank < 0 || std::size_t(bank) >= m_banks.size() || value < 0 || value > 255)
        return fail(tr("Choose a writable bank and a byte from 0 to 255"));
    Trainer trainer;
    Patch patch{ m_banks[bank].id, address, 0, uint16_t(value), true };
    QString error;
    {
        auto core = m_core.writeLock();
        trainer.machine = core->machine().id();
        if (!patch.bank && address >= 0 && address <= 65535) {
            const auto &mapping = core->mmu().effectiveMapping(unsigned(address) / 1024);
            if (!mapping.writable || !mapping.writeBankId)
                error = tr("This CPU address is read-only or unmapped");
            else {
                patch.bank = mapping.writeBankId;
                patch.address = mapping.writePage1k * 1024 + (address & 1023);
            }
        }
        for (const auto &other : m_trainers)
            if (other.active)
                for (const auto &byte : other.patches)
                    if (byte.bank == patch.bank && byte.address == patch.address)
                        error = tr("Turn off the overlapping trainer first");
        if (error.isEmpty())
            error = writePoke(*core, patch.bank, patch.address, uint8_t(value), &patch.original);
    }
    if (!error.isEmpty())
        return fail(error);
    trainer.patches.push_back(patch);
    trainer.name = u"%1 : $%2 = $%3"_s.arg(m_bankNames[bank])
                           .arg(QString::number(address, 16).toUpper())
                           .arg(QString::number(value, 16).rightJustified(2, u'0').toUpper());
    trainer.active = true;
    const int row = int(m_trainers.size());
    beginInsertRows({}, row, row);
    m_trainers.push_back(std::move(trainer));
    endInsertRows();
    emit memoryChanged();
    return true;
}
QVariantList PokeModel::requestedValues(int row) const
{
    QVariantList result;
    if (row < 0 || std::size_t(row) >= m_trainers.size())
        return result;
    for (const auto &patch : m_trainers[row].patches)
        if (patch.value == 256)
            result.append(tr("Value at $%1 (0–255)").arg(patch.address, 4, 16, u'0'));
    return result;
}
bool PokeModel::setActive(int row, bool active, const QVariantList &values)
{
    if (row < 0 || std::size_t(row) >= m_trainers.size())
        return false;
    auto &trainer = m_trainers[row];
    if (trainer.active == active || trainer.disabled)
        return false;
    auto patches = trainer.patches;
    QString error;
    {
        auto core = m_core.writeLock();
        auto &mmu = core->mmu();
        if (core->machine().id() != trainer.machine)
            error = tr("This poke belongs to another machine");
        int valueIndex = 0;
        // Resolve and validate the entire group before writing any byte.
        for (auto &patch : patches) {
            if (!error.isEmpty())
                break;
            if (active && patch.value == 256) {
                bool ok = false;
                const int value = values.value(valueIndex++).toInt(&ok);
                if (!ok || value < 0 || value > 255) {
                    error = tr("Enter a byte from 0 to 255 for every requested value");
                    break;
                }
                patch.value = uint16_t(value);
            }
            if (!patch.resolved) {
                if (patch.bank == 8) {
                    const auto &mapping = mmu.effectiveMapping(unsigned(patch.address) / 1024);
                    if (!mapping.writable || !mapping.writeBankId) {
                        error = tr("This CPU address is read-only or unmapped");
                        break;
                    }
                    patch.bank = mapping.writeBankId;
                    patch.address = mapping.writePage1k * 1024 + (patch.address & 1023);
                } else {
                    const int page = patch.bank;
                    patch.bank = core->machine().ramBankId();
                    const auto size = mmu.bank(zxkit::BankId(patch.bank)).size();
                    int physicalPage = page;
                    if (size <= 48 * 1024) {
                        physicalPage = page == 5 ? 0 : page == 2 ? 1 : page == 0 ? 2 : -1;
                    }
                    if (physicalPage < 0 || std::size_t((physicalPage + 1) * 16384) > size) {
                        error = tr("The trainer's RAM page is unavailable on this machine");
                        break;
                    }
                    patch.address = physicalPage * 16384 + (patch.address & 16383);
                }
                patch.resolved = true;
            }
            if (patch.bank <= 0 || std::size_t(patch.bank) > mmu.banks().size()) {
                error = tr("Memory banks changed; reload the trainer");
                break;
            }
            auto &bank = mmu.bank(zxkit::BankId(patch.bank));
            if (!bank.isWritable() || patch.address < 0
                || std::size_t(patch.address) >= bank.size()) {
                error = tr("The trainer targets unavailable or read-only memory");
                break;
            }
            if (active)
                patch.original = bank.bytes()[patch.address];
        }
        // Separate active trainers must not silently undo each other's bytes.
        if (active && error.isEmpty()) {
            for (const auto &other : m_trainers) {
                if (&other == &trainer || !other.active)
                    continue;
                for (const auto &a : other.patches)
                    for (const auto &b : patches)
                        if (a.bank == b.bank && a.address == b.address)
                            error = tr("Turn off the overlapping trainer first");
            }
        }
        if (error.isEmpty()) {
            if (active) {
                for (const auto &patch : patches)
                    writePoke(*core, patch.bank, patch.address, uint8_t(patch.value));
            } else {
                for (auto it = patches.rbegin(); it != patches.rend(); ++it)
                    writePoke(*core, it->bank, it->address, it->original);
            }
        }
    }
    if (!error.isEmpty())
        return fail(error);
    trainer.patches = std::move(patches);
    trainer.active = active;
    emit dataChanged(index(row), index(row), { Active, Disabled });
    emit memoryChanged();
    return true;
}
void PokeModel::setPokesFolder(const QString &folder)
{
    choosePokesFolder(folder);
}
bool PokeModel::choosePokesFolder(const QString &folder)
{
    const QUrl url(folder);
    const auto path = url.isLocalFile() ? url.toLocalFile() : folder;
    if (!path.isEmpty() && !QFileInfo(path).isDir())
        return fail(tr("Choose an existing POKES folder"));
    m_folder = path;
    QSettings().setValue(u"Trainers/pokesFolder"_s, path);
    emit sourceChanged();
    // Do not discard active/custom pokes when changing a search preference.
    if (m_trainers.empty())
        discover();
    return true;
}
void PokeModel::gameLoaded(const QString &path)
{
    clear();
    const QUrl url(path);
    m_game = url.isLocalFile() ? url.toLocalFile() : path;
    if (!m_game.isEmpty())
        discover();
}
void PokeModel::discover()
{
    if (m_game.isEmpty())
        return;
    const QFileInfo game(m_game);
    QStringList folders{ game.absolutePath() };
    if (!m_folder.isEmpty() && !folders.contains(m_folder))
        folders.append(m_folder);
    for (const auto &folder : folders) {
        const auto files = QDir(folder).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const auto &file : files) {
            if (file.suffix().compare(u"pok"_s, Qt::CaseInsensitive) == 0
                && file.completeBaseName().compare(game.completeBaseName(), Qt::CaseInsensitive)
                        == 0) {
                loadPok(file.absoluteFilePath());
                return;
            }
        }
    }
}
bool PokeModel::loadPok(const QString &path)
{
    const QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Cannot open trainers: %1").arg(file.errorString()));
    if (file.size() > 1024 * 1024)
        return fail(tr("Trainer file is too large (maximum 1 MiB)"));
    const auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError)
        return fail(tr("Cannot read trainers"));
    // Validate before the core's permissive legacy parser can narrow integers.
    bool named = false, ended = false;
    auto text = QString::fromUtf8(bytes);
    if (text.startsWith(QChar(0xfeff)))
        text.remove(0, 1);
    QByteArray canonical;
    for (auto line : text.split(u'\n')) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;
        canonical += line.toUtf8() + '\n';
        const auto tag = line.front().toUpper();
        if (tag == u'Y') {
            ended = true;
            break;
        }
        if (tag == u'N') {
            named = true;
            continue;
        }
        if ((tag != u'M' && tag != u'Z') || !named)
            return fail(tr("Invalid POK record"));
        const auto fields = line.mid(1).simplified().split(u' ');
        if (fields.size() < 3 || fields.size() > 4)
            return fail(tr("Invalid POK patch"));
        const int limits[] = { 8, 65535, 256, 255 };
        for (int i = 0; i < fields.size(); ++i) {
            bool ok = false;
            const int value = fields[i].toInt(&ok);
            if (!ok || value < 0 || value > limits[i])
                return fail(tr("POK bank, address or byte is out of range"));
        }
    }
    const auto parsed = zxkit::PokeFinder::parsePokFile(
            std::string_view(canonical.constData(), std::size_t(canonical.size())));
    if (!ended || parsed.empty())
        return fail(tr("No complete trainers found in this POK file"));
    Trainers trainers;
    const auto machine = std::string(m_core.readLock()->machine().id());
    for (const auto &entry : parsed) {
        if (entry.name.empty() || entry.patches.empty())
            return fail(tr("Trainer has no name or patches"));
        Trainer trainer;
        trainer.name = QString::fromStdString(entry.name);
        trainer.machine = machine;
        for (const auto &patch : entry.patches)
            trainer.patches.push_back({ patch.bank, patch.address, 0, patch.value, false });
        trainers.push_back(std::move(trainer));
    }
    for (const auto &entry : m_trainers)
        if (entry.active)
            return fail(tr("Turn off active pokes before replacing the trainer list"));
    beginResetModel();
    m_trainers = std::move(trainers);
    m_source = file.fileName();
    endResetModel();
    emit sourceChanged();
    return true;
}

void PokeModel::clear()
{
    beginResetModel();
    m_trainers.clear();
    m_source.clear();
    endResetModel();
    emit sourceChanged();
}
