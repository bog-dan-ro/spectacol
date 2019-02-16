#include "fuserecording.h"
#include "fuseemulator.h"

#include "qmlui.h"

extern "C"  {
# include <fuse.h>
# include <rzx.h>
}

FuseRecording::FuseRecording(FuseEmulator *parent)
    : FuseObject(parent)
    , m_emulator(parent)
{
}

void FuseRecording::setStopVisible(bool visible)
{
    callFunction([this] {
        if (rzx_recording) {
            m_type = Recording;
        } else {
            if (rzx_playback)
                m_type = Playback;
            else
                m_type = None;
        }
        emit typeChanged(m_type);
    });
}

void FuseRecording::start()
{
    pokeEvent([this]{
        fuse_emulation_pause();
        auto path = m_emulator->recordingFilePath();
        if (rzx_start_recording(path.toUtf8().constData(), 1) == LIBSPECTRUM_ERROR_NONE)
            m_emulator->showMessage(tr("Recording starting to %1").arg(path));
        else
            m_emulator->showMessage(tr("Can't start recording to %1").arg(path), FuseEmulator::Warning);
        fuse_emulation_unpause();
    });
}

void FuseRecording::continueRecording()
{
    pokeEvent([this]{
        if (rzx_playback || rzx_recording)
            return;

        fuse_emulation_pause();
        char *rzx_filename = m_emulator->uiOpenFilename("Continue Recording", m_emulator->recordingsPath());
        if (!rzx_filename) {
            fuse_emulation_unpause();
            return;
        }

        if (rzx_continue_recording(rzx_filename) != LIBSPECTRUM_ERROR_NONE)
            m_emulator->showMessage(tr("RZX file cannot be continued"), FuseEmulator::Warning);

        libspectrum_free(rzx_filename);
        fuse_emulation_unpause();
    });
}

void FuseRecording::rollback()
{
    pokeEvent([this]{
        fuse_emulation_pause();
        if (rzx_rollback() != LIBSPECTRUM_ERROR_NONE)
            m_emulator->showMessage(tr("Cannot rollback."), FuseEmulator::Warning);
        fuse_emulation_unpause();

    });
}

void FuseRecording::rollbackTo()
{
    pokeEvent([this]{
        fuse_emulation_pause();
        if (rzx_rollback_to() != LIBSPECTRUM_ERROR_NONE)
            m_emulator->showMessage(tr("Cannot rollback."), FuseEmulator::Warning);
        fuse_emulation_unpause();
    });
}

void FuseRecording::finalise()
{
    pokeEvent([this]{
        if (rzx_playback || rzx_recording)
            return;

        fuse_emulation_pause();
        char *rzx_filename = m_emulator->uiOpenFilename("Finalise Recording", m_emulator->recordingsPath());
        if (!rzx_filename) {
            fuse_emulation_unpause();
            return;
        }

        if (rzx_finalise_recording(rzx_filename) == LIBSPECTRUM_ERROR_NONE)
            m_emulator->showMessage(tr("RZX file finalised"));
        else
            m_emulator->showMessage(tr("RZX file cannot be finalised"), FuseEmulator::Warning);

        libspectrum_free(rzx_filename);
        fuse_emulation_unpause();
    });
}

void FuseRecording::stop()
{
    pokeEvent([]{
        if (rzx_recording)
            rzx_stop_recording();
        if (rzx_playback)
            rzx_stop_playback(1);
    });
}
