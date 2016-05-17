#include "fusetape.h"

#include <fuse.h>
#include <settings.h>
#include <tape.h>

#include "qmlui.h"

FuseTape::FuseTape(QObject *parent) : FuseObject(parent)
{
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

