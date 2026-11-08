/*
    Copyright (c) 2026, BogDan Vatra <bogdan@kde.org>

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

#include "sdcardfiles.h"

#include "nextsdcard.h"
#include "zxemulator.h"
#include "zxsettings.h"

#include <QFile>
#include <QFileInfo>

#include <zxkit/core/Exception.h>
#include <zxkit/peripherals/storage/FatVolume.h>

#include <algorithm>
#include <ranges>

using namespace Qt::StringLiterals;

namespace {

QString localPath(const QUrl &url)
{
    // Android's file dialogs give content:// urls, which QFile opens as they are
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

} // namespace

SdCardFiles::SdCardFiles(ZxEmulator *parent)
    : QObject{ parent }, m_zxEmulator(parent), m_folder(u"/"_s)
{
    connect(m_zxEmulator->settings(), &ZxSettings::currentMachineChanged, this,
            &SdCardFiles::updateInUse);
    connect(&NextSdCard::instance(), &NextSdCard::imagePathChanged, this, [this] {
        m_folder = u"/"_s;
        refresh();
    });
    connect(&NextSdCard::instance(), &NextSdCard::ready, this, [this] {
        useImage({});
        refresh();
    });
    // ZxEmulator creates the engine after its members
    QMetaObject::invokeMethod(this, &SdCardFiles::updateInUse, Qt::QueuedConnection);
}

QString SdCardFiles::imagePath() const
{
    return NextSdCard::instance().imagePath();
}

bool SdCardFiles::customImage() const
{
    return NextSdCard::instance().isCustomImage();
}

void SdCardFiles::updateInUse()
{
    const bool inUse = m_zxEmulator->emulator().readLock()->machine().id() == "next";
    if (inUse == m_inUse)
        return;
    m_inUse = inUse;
    emit changed();
}

QString SdCardFiles::childPath(const QString &name) const
{
    return m_folder.endsWith(u'/') ? m_folder + name : m_folder + u'/' + name;
}

bool SdCardFiles::withVolume(const std::function<void(zxkit::FatVolume &volume)> &fn, bool write)
{
    const std::string image = imagePath().toStdString();
    try {
        auto emulator = m_zxEmulator->emulator().writeLock();
        // The other machines don't use the card: it is inserted to get to its files
        if (!emulator->sdCard().isInserted() || emulator->sdCard().imagePath() != image)
            emulator->insertSdCard(image);
        auto volume = emulator->sdCardVolume();
        fn(volume);
        // NextZXOS caches the file system: it must read it again
        if (write && emulator->machine().id() == "next")
            emulator->reset(zxkit::ResetKind::Hard);
        return true;
    } catch (const zxkit::Exception &e) {
        m_zxEmulator->showMessage(QString::fromUtf8(e.rawMessage()), ZxEmulator::Error);
    } catch (const std::exception &e) {
        m_zxEmulator->showMessage(QString::fromUtf8(e.what()), ZxEmulator::Error);
    }
    return false;
}

void SdCardFiles::refresh()
{
    m_available = false;
    m_entries.clear();
    if (NextSdCard::instance().available()) {
        withVolume(
                [this](zxkit::FatVolume &volume) {
                    auto entries = volume.list(m_folder.toStdString());
                    if (!entries) {
                        m_folder = u"/"_s;
                        entries = volume.list("/");
                    }
                    std::ranges::stable_sort(*entries, [](const auto &a, const auto &b) {
                        return a.isDirectory > b.isDirectory;
                    });
                    if (m_folder != u"/"_s)
                        m_entries.append(QVariantMap{
                                { u"name"_s, u".."_s }, { u"size"_s, 0 }, { u"isDir"_s, true } });
                    for (const auto &e : *entries) {
                        m_entries.append(QVariantMap{ { u"name"_s, QString::fromStdString(e.name) },
                                                      { u"size"_s, qint64(e.size) },
                                                      { u"isDir"_s, e.isDirectory } });
                    }
                    m_readOnly = !volume.isWritable();
                    m_totalBytes = qint64(volume.totalBytes());
                    m_freeBytes = qint64(volume.freeBytes());
                    m_available = true;
                },
                false);
    }
    emit changed();
}

void SdCardFiles::openFolder(const QString &name)
{
    if (name == u".."_s) {
        m_folder = m_folder.left(m_folder.lastIndexOf(u'/'));
        if (m_folder.isEmpty())
            m_folder = u"/"_s;
    } else {
        m_folder = childPath(name);
    }
    refresh();
}

void SdCardFiles::addFile(const QUrl &file)
{
    const QString path = localPath(file);
    QFile f{ path };
    if (!f.open(QIODeviceBase::ReadOnly)) {
        m_zxEmulator->showMessage(tr("Can't open %1").arg(path), ZxEmulator::Error);
        return;
    }
    // Read the file before locking the emulator
    const QByteArray bytes = f.readAll();
    const QString name = QFileInfo{ f.fileName() }.fileName();
    const auto data =
            std::span(reinterpret_cast<const uint8_t *>(bytes.constData()), size_t(bytes.size()));
    if (withVolume(
                [&](zxkit::FatVolume &volume) {
                    volume.writeFile(childPath(name).toStdString(), data);
                },
                true)) {
        m_zxEmulator->showMessage(
                m_inUse ? tr("'%1' copied to %2, the Next restarts").arg(name, m_folder)
                        : tr("'%1' copied to %2").arg(name, m_folder));
    }
    refresh();
}

void SdCardFiles::makeFolder(const QString &name)
{
    if (name.trimmed().isEmpty())
        return;
    withVolume(
            [&](zxkit::FatVolume &volume) {
                volume.makeDirectory(childPath(name.trimmed()).toStdString());
            },
            true);
    refresh();
}

void SdCardFiles::remove(const QString &name)
{
    if (name.isEmpty() || name == u".."_s)
        return;
    if (withVolume([&](zxkit::FatVolume &volume) { volume.remove(childPath(name).toStdString()); },
                   true))
        m_zxEmulator->showMessage(tr("'%1' removed").arg(name));
    refresh();
}

void SdCardFiles::useImage(const QUrl &image)
{
    auto &sdCard = NextSdCard::instance();
    sdCard.setImagePath(image.isEmpty() ? QString{} : localPath(image));
    if (!m_inUse)
        return;
    // The Next restarts with the new card
    try {
        auto emulator = m_zxEmulator->emulator().writeLock();
        emulator->insertSdCard(sdCard.imagePath().toStdString());
        emulator->reset(zxkit::ResetKind::Hard);
    } catch (const zxkit::Exception &e) {
        m_zxEmulator->showMessage(QString::fromUtf8(e.rawMessage()), ZxEmulator::Error);
    }
}
