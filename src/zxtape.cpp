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

#include "zxtape.h"

#include "zxemulator.h"
#include "contenturi.h"

#include <QFileInfo>

#include <utility>

using namespace Qt::StringLiterals;

ZxTape::ZxTape(ZxEmulator *emulator) : QObject(emulator), m_zxEmulator(emulator)
{
    connect(emulator, &ZxEmulator::pausedChanged, this, &ZxTape::checkPendingChange);
}

QString ZxTape::suggestedFileName() const
{
    return m_zxEmulator->tapeFileName(m_programName);
}

void ZxTape::setHasTape(bool hasTape)
{
    if (hasTape == m_hasTape)
        return;
    m_hasTape = hasTape;
    emit hasTapeChanged(hasTape);
    emit insertedChanged();
}

void ZxTape::updateBrowseData()
{
    setHasTape(!m_zxEmulator->emulator().readLock()->tapeDeck().blocks().empty());
}

void ZxTape::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
}

void ZxTape::setProgramName(const QString &programName)
{
    if (m_programName == programName)
        return;

    m_programName = programName;
    emit programNameChanged();
}

bool ZxTape::canSaveTap() const
{
    const auto emulator = m_zxEmulator->emulator().readLock();
    return std::ranges::all_of(emulator->tapeDeck().blocks(), [](const auto &b) {
        return b.isMetadata() || (b.isRomBlock() && b.data.size() <= 65535);
    });
}

void ZxTape::refreshData()
{
    QString progName;
    bool hasTape = false;
    {
        const auto emulator = m_zxEmulator->emulator().readLock();
        const auto &blocks = emulator->tapeDeck().blocks();
        for (const auto &blk : blocks) {
            const QString desc = QString::fromStdString(blk.description);
            const int prefix = desc.startsWith(u"Program: "_s) ? 9
                    : desc.startsWith(u"PZX Program: "_s)      ? 13
                                                               : 0;
            if (prefix) {
                progName = desc.mid(prefix).trimmed();
                break;
            }
        }
        hasTape = !blocks.empty();
    }
    setProgramName(progName);
    setHasTape(hasTape);
}

bool ZxTape::canSave() const
{
    if (m_sourcePath.isEmpty())
        return false;
    if (ContentUri::isContent(m_sourcePath))
        return true;
    const QFileInfo source(m_sourcePath);
    return source.isFile() && source.isWritable();
}

bool ZxTape::inserted() const
{
    return m_blank || m_hasTape || !m_sourcePath.isEmpty();
}

void ZxTape::setSource(const QString &path, const QString &format)
{
    m_blank = false;
    m_savedBlocks = m_zxEmulator->emulator().readLock()->tapeDeck().blocks().size();
    m_sourcePath = path;
    m_sourceFormat = format;
    emit sourceChanged();
    emit insertedChanged();
}

bool ZxTape::changePending() const
{
    return bool(m_pendingChange);
}

void ZxTape::beforeReplacing(std::function<void()> action)
{
    if (changePending())
        return;
    m_pendingChange = std::move(action);
    m_waitingForPause = true;
    m_zxEmulator->setPaused(true);
}

void ZxTape::checkPendingChange()
{
    // The Engine finishes its current frame before acknowledging the pause.
    if (!m_waitingForPause || !m_zxEmulator->emulationQuiescent())
        return;
    m_waitingForPause = false;
    if (m_zxEmulator->emulator().readLock()->tapeDeck().blocks().size() != m_savedBlocks) {
        emit unsavedChanges();
        return;
    }
    resolveChange(true);
}

void ZxTape::resolveChange(bool proceed)
{
    auto action = std::exchange(m_pendingChange, {});
    if (!action)
        return;
    m_waitingForPause = false;
    if (proceed)
        action();
    m_zxEmulator->setPaused(false);
}

void ZxTape::togglePlay()
{
    m_zxEmulator->withEmulator([](zxkit::Emulator &emulator) {
        auto &deck = emulator.tapeDeck();
        deck.setPlaying(!deck.isPlaying());
    });
}

void ZxTape::rewind()
{
    m_zxEmulator->withEmulator([](zxkit::Emulator &emulator) { emulator.tapeDeck().rewind(); });
}

void ZxTape::clear()
{
    beforeReplacing([this] { resetDeck(false); });
}

void ZxTape::newBlank()
{
    beforeReplacing([this] { resetDeck(true); });
}

void ZxTape::resetDeck(bool blank)
{
    ++m_zxEmulator->m_gameLoadRequest;
    m_zxEmulator->withEmulator([](zxkit::Emulator &emulator) { emulator.tapeDeck().clear(); });
    setSource({}, {});
    m_blank = blank;
    refreshData();
    setPlaying(false);
    emit insertedChanged();
}

bool ZxTape::save()
{
    return canSave() && write(m_sourcePath, m_sourceFormat, false);
}

bool ZxTape::saveAs(const QString &filePath, const QString &format)
{
    return write(filePath, format, true);
}

bool ZxTape::write(const QString &filePath, const QString &format, bool newExport)
{
    std::size_t blocks = 0;
    std::vector<uint8_t> bytes;
    if (!m_zxEmulator->withEmulator([&](zxkit::Emulator &emulator) {
            const auto &deck = emulator.tapeDeck();
            blocks = deck.blocks().size();
            if (blocks)
                bytes = emulator.saveMedia(format.toStdString());
        }))
        return false;
    if (!blocks) {
        m_zxEmulator->showMessage(tr("The tape is empty. Enter SAVE in BASIC before exporting it."),
                                  ZxEmulator::Warning);
        return false;
    }
    const auto destination = m_zxEmulator->writeMediaBytes(filePath, bytes, newExport);
    if (destination.isEmpty())
        return false;
    m_savedBlocks = blocks;
    m_sourcePath = destination;
    m_sourceFormat = format;
    emit sourceChanged();
    m_zxEmulator->showMessage(tr("Saved %1")
                                      .arg(ContentUri::isContent(destination)
                                                   ? m_zxEmulator->fileName(destination)
                                                   : destination));
    return true;
}
