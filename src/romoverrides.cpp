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

#include "romoverrides.h"

#include <zxkit/core/Emulator.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <string_view>

using namespace Qt::StringLiterals;
using namespace std::literals;

RomOverrides::RomOverrides(QObject *parent) : QObject(parent)
{
    const zxkit::MachineRegistry registry;
    for (const auto &id : registry.availableIds()) {
        const auto machine = registry.create(id);
        const auto qid = QString::fromStdString(id);
        m_ids.append(qid);
        m_names.append(QString::fromUtf8(machine->name()));
        if (id == "48k"sv)
            m_index = int(m_ids.size() - 1);
        // Ask the core itself for names/sizes; never duplicate its ROM catalogue.
        zxkit::EmulatorConfig config;
        config.machineId = id;
        config.romResolver = [&](std::string_view name, std::size_t size) {
            const auto qname = QString::fromUtf8(name);
            m_slots[qid].append({ qname, qsizetype(size) });
            QFile bundled(u":/roms/"_s + qname);
            if (bundled.open(QIODevice::ReadOnly))
                m_bundled[qname] = bundled.readAll();
            return std::vector<uint8_t>(size, 0);
        };
        zxkit::Emulator probe(std::move(config));
    }
    // Peripheral ROMs are not requested by the machine-only catalogue probes.
    const QDir resources(u":/roms"_s);
    for (const auto &name : resources.entryList(QDir::Files)) {
        QFile file(resources.filePath(name));
        if (file.open(QIODevice::ReadOnly))
            m_bundled[name] = file.readAll();
    }
    QSettings settings;
    for (auto it = m_slots.cbegin(); it != m_slots.cend(); ++it) {
        for (const auto &slot : it.value()) {
            const auto k = key(it.key(), slot.name);
            const auto label = settings.value(u"CustomROMs/"_s + k).toString();
            if (label.isEmpty())
                continue;
            QFile file(storedPath(it.key(), slot.name));
            if (file.open(QIODevice::ReadOnly) && file.size() == slot.size) {
                const auto bytes = file.readAll();
                if (bytes.size() == slot.size && file.error() == QFileDevice::NoError)
                    m_overrides.insert(k, { bytes, label });
                else
                    m_error = tr("Cannot read saved ROM %1; using the default").arg(slot.name);
            } else
                m_error = tr("Missing or invalid saved ROM %1; using the default").arg(slot.name);
        }
    }
}
QString RomOverrides::key(const QString &machine, const QString &name) const
{
    return machine + u'/' + name;
}
QString RomOverrides::storedPath(const QString &machine, const QString &name) const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(u"ROMs/"_s + key(machine, name));
}
void RomOverrides::setMachineIndex(int index)
{
    if (index < 0 || index >= m_ids.size() || index == m_index)
        return;
    m_index = index;
    m_error.clear();
    emit errorChanged();
    emit changed();
}
QVariantList RomOverrides::romSlots() const
{
    QVariantList result;
    const auto machine = m_ids[m_index];
    const std::lock_guard guard(m_mutex);
    for (const auto &slot : m_slots[machine]) {
        const auto custom = m_overrides.constFind(key(machine, slot.name));
        result.append(QVariantMap{
                { u"name"_s, slot.name },
                { u"size"_s, slot.size },
                { u"custom"_s, custom != m_overrides.cend() },
                { u"source"_s,
                  custom != m_overrides.cend() ? custom->label : tr("Default ROM") } });
    }
    return result;
}
bool RomOverrides::fail(const QString &message)
{
    m_error = message;
    emit errorChanged();
    return false;
}
bool RomOverrides::useFile(int index, const QString &path)
{
    const auto machine = m_ids[m_index];
    const auto list = m_slots.value(machine);
    if (index < 0 || index >= list.size())
        return fail(tr("Choose a ROM slot"));
    const auto &slot = list[index];
    const QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Cannot open ROM: %1").arg(file.errorString()));
    if (file.size() != slot.size)
        return fail(tr("%1 requires exactly %2 bytes; this file has %3")
                            .arg(slot.name)
                            .arg(slot.size)
                            .arg(file.size()));
    const auto bytes = file.readAll();
    if (bytes.size() != slot.size || file.error() != QFileDevice::NoError)
        return fail(tr("Cannot read the complete ROM"));
    const auto target = storedPath(machine, slot.name);
    if (!QDir().mkpath(QFileInfo(target).absolutePath()))
        return fail(tr("Cannot create the ROM folder"));
    QSaveFile output(target);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()
        || !output.commit())
        return fail(tr("Cannot store ROM: %1").arg(output.errorString()));
    const auto label = QFileInfo(path).fileName();
    {
        const std::lock_guard guard(m_mutex);
        m_overrides[key(machine, slot.name)] = { bytes, label };
    }
    QSettings().setValue(u"CustomROMs/"_s + key(machine, slot.name), label);
    m_error.clear();
    emit errorChanged();
    emit changed();
    return true;
}
bool RomOverrides::restore(int index)
{
    const auto machine = m_ids[m_index];
    const auto list = m_slots.value(machine);
    if (index < 0 || index >= list.size())
        return false;
    const auto &slot = list[index];
    // Remove the preference first; stale files are harmless if removal fails.
    QSettings().remove(u"CustomROMs/"_s + key(machine, slot.name));
    {
        const std::lock_guard guard(m_mutex);
        m_overrides.remove(key(machine, slot.name));
    }
    QFile::remove(storedPath(machine, slot.name));
    m_error.clear();
    emit errorChanged();
    emit changed();
    return true;
}
bool RomOverrides::restoreMachine()
{
    for (int i = 0, count = m_slots.value(m_ids[m_index]).size(); i < count; ++i)
        restore(i);
    return true;
}
std::vector<uint8_t> RomOverrides::resolve(std::string_view machine, std::string_view name,
                                           std::size_t size) const
{
    const std::lock_guard guard(m_mutex);
    const auto qname = QString::fromUtf8(name);
    const auto custom = m_overrides.constFind(key(QString::fromUtf8(machine), qname));
    const auto bytes = custom != m_overrides.cend() ? custom->bytes : m_bundled.value(qname);
    if (size && std::size_t(bytes.size()) != size)
        return {};
    return { bytes.begin(), bytes.end() };
}
