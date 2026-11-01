/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

using namespace Qt::StringLiterals;

ZxTape::ZxTape(ZxEmulator *emulator)
    : QObject(emulator)
    , m_zxEmulator(emulator)
{
    m_saveSnapshotTimer.setInterval(2000);
    m_saveSnapshotTimer.setSingleShot(true);
    connect(&m_saveSnapshotTimer, &QTimer::timeout, this, [this] {
        write(m_zxEmulator->saveSnapshotsFilePath(m_programName));
    });
}

void ZxTape::setHasTape(bool hasTape)
{
    if (hasTape == m_hasTape)
        return;
    m_hasTape = hasTape;
    emit hasTapeChanged(hasTape);
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

void ZxTape::refreshData()
{
    QString progName;
    bool hasTape = false;
    {
        const auto emulator = m_zxEmulator->emulator().readLock();
        const auto &blocks = emulator->tapeDeck().blocks();
        for (const auto &blk : blocks) {
            const QString desc = QString::fromStdString(blk.description);
            if (desc.startsWith(u"Program: "_s)) {
                progName = desc.mid(9).trimmed();
                break;
            }
        }
        hasTape = !blocks.empty();
    }
    if (!progName.isEmpty())
        setProgramName(progName);
    setHasTape(hasTape);
    m_saveSnapshotTimer.stop();
}

void ZxTape::open(QString filePath)
{
    if (m_zxEmulator->loadMediaFile(filePath))
        refreshData();
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
    m_zxEmulator->withEmulator([](zxkit::Emulator &emulator) {
        emulator.tapeDeck().rewind();
    });
}

void ZxTape::clear()
{
    m_zxEmulator->withEmulator([](zxkit::Emulator &emulator) {
        emulator.tapeDeck().clear();
    });
    setHasTape(false);
}

void ZxTape::write(QString filePath)
{
    if (m_zxEmulator->saveMediaFile(filePath))
        m_zxEmulator->showMessage(tr("Saved %1").arg(filePath));
}
