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

#include "qmlui.h"

extern "C"  {
# include <fuse.h>
# include <settings.h>
# include <tape.h>
}

FuseTape::FuseTape(QObject *parent) : FuseObject(parent)
{
}

void FuseTape::updateBrowseData()
{
    callFunction([this]{
        auto hasTape = tape_get_current_block() != -1;
        if (hasTape != m_hasTape) {
            m_hasTape = hasTape;
            emit hasTapeChanged(hasTape);
        }
    });
}

void FuseTape::open(QString filePath)
{
    pokeEvent([filePath]{
        fuse_emulation_pause();
        tape_open(filePath.toUtf8().constData(), 0);
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
    pokeEvent([]{
        tape_close();
    });
}

void FuseTape::write(QString filePath)
{
    pokeEvent([filePath]{
        fuse_emulation_pause();
        tape_write(filePath.toUtf8().constData());
        fuse_emulation_unpause();
    });
}

