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

#include "fusetape.h"

#include "fuseemulator.h"
#include "qmlui.h"

extern "C"  {
# include <fuse.h>
# include <settings.h>
# include <tape.h>
}

FuseTape::FuseTape(FuseEmulator *emulator)
    : FuseObject(emulator)
    , m_fuseEmulator(emulator)
{
    m_saveSnapshotTimer.setInterval(2000);
    m_saveSnapshotTimer.setSingleShot(true);
    connect(&m_saveSnapshotTimer, &QTimer::timeout, this, [this]{
        auto oldData = std::move(m_tapeData);
        tape_foreach(&checkProgramName, this);
        if (oldData != m_tapeData)
            write(m_fuseEmulator->saveSnapshotsFilePath(m_programName));
    });
}

void FuseTape::updateBrowseData()
{
    callFunction([this]{
        auto hasTape = tape_get_current_block() != -1;
        if (hasTape != m_hasTape) {
            m_hasTape = hasTape;
            emit hasTapeChanged(hasTape);
        }
        if (hasTape)
            m_saveSnapshotTimer.start();
    });
}

void FuseTape::setProgramName(const QString& programName)
{
    if (m_programName == programName)
        return;

    m_programName = programName;
    emit programNameChanged();
}

void FuseTape::refreshData()
{
    m_tapeData.clear();
    tape_foreach(&checkProgramName, this);
    m_saveSnapshotTimer.stop();
}

void FuseTape::open(QString filePath)
{
    pokeEvent([this, filePath]{
        m_tapeData.clear();
        fuse_emulation_pause();
        tape_open(filePath.toUtf8().constData(), 0);
        tape_foreach(&checkProgramName, this);
        fuse_emulation_unpause();
    });
}

void FuseTape::togglePlay()
{
    pokeEvent([]{
        tape_toggle_play(0);
    });
}

void FuseTape::rewind()
{
    pokeEvent([]{
        tape_rewind();
    });
}

void FuseTape::clear()
{
    pokeEvent([this]{
        m_tapeData.clear();
        tape_close();
    });
}

void FuseTape::write(QString filePath)
{
    pokeEvent([this, filePath]{
        fuse_emulation_pause();
        tape_write(filePath.toUtf8().constData());
        fuse_emulation_unpause();
        m_saveSnapshotTimer.stop();
        m_fuseEmulator->showMessage(tr("Saved %1").arg(filePath));
    });
}

void FuseTape::checkProgramName(libspectrum_tape_block *block, void *user_data)
{
    auto self = reinterpret_cast<FuseTape*>(user_data);
    QByteArray temp{256, 0};
    tape_block_details(temp.data(), temp.size(), block);
    if (temp.startsWith("Program: \""))
        self->setProgramName(temp.mid(10, 8).trimmed());
    switch (libspectrum_tape_block_type(block)) {
    case LIBSPECTRUM_TAPE_BLOCK_ROM:
    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
        self->m_tapeData.append(QByteArray(reinterpret_cast<const char *>(libspectrum_tape_block_data(block)),
                                           libspectrum_tape_block_data_length(block)));
        break;
    default:
        break;
    }
}
