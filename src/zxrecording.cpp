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

#include "zxrecording.h"
#include "zxemulator.h"
#include "zxsettings.h"

#include <algorithm>
#include <utility>

ZxRecording::ZxRecording(ZxEmulator *parent) : QObject(parent), m_emulator(parent)
{
    m_progressTimer.setInterval(200);
    connect(&m_progressTimer, &QTimer::timeout, this, &ZxRecording::refresh);
    connect(&m_seekTimer, &QTimer::timeout, this, &ZxRecording::advanceSeek);
}

void ZxRecording::refresh()
{
    zxkit::RzxProgress progress;
    std::string machine;
    Type t;
    quint64 position, duration;
    {
        const auto emulator = m_emulator->emulator().readLock();
        const auto &rzx = emulator->rzx();
        progress = rzx.progress();
        machine = emulator->machine().id();
        position = emulator->rzxPositionMilliseconds();
        duration = emulator->rzxDurationMilliseconds();
        t = !m_pendingRecording.empty() ? Type::Unsaved
                : (rzx.isRecording() || rzx.isReviewing())
                ? Type::Recording
                : (rzx.isPlaying() ? Type::Playback : Type::None);
    }
    if (m_progress.state != progress.state && m_pauseOwned
        && (progress.state == zxkit::RzxState::Idle
            || progress.state == zxkit::RzxState::Stopped)) {
        m_seekRequest.reset();
        m_seekTimer.stop();
        m_seeking = false;
        m_resumeAfterSeek = m_stopRequested = false;
        releasePause();
    }
    // Signals can acquire the core lock, so notify only after releasing it.
    m_emulator->settings()->syncCurrentMachine(machine);
    if (m_progress != progress) {
        m_progress = std::move(progress);
        emit progressChanged();
    }
    if (m_type != t) {
        m_type = t;
        emit typeChanged(m_type);
    }
    if (position != m_positionMs || duration != m_durationMs) {
        m_positionMs = position;
        m_durationMs = duration;
        emit transportChanged();
    }
    const bool active = !m_pauseOwned && (t == Type::Playback || t == Type::Recording);
    if (active && !m_progressTimer.isActive())
        m_progressTimer.start();
    else if (!active)
        m_progressTimer.stop();
}

void ZxRecording::mediaLoaded()
{
    if (m_type == Type::Recording || m_type == Type::Unsaved)
        return;
    m_seekRequest.reset();
    m_seekTimer.stop();
    m_seeking = false;
    m_resumeAfterSeek = m_stopRequested = false;
    releasePause();
    refresh();
}

QString ZxRecording::statusText() const
{
    using zxkit::RzxState;
    if (m_progress.state == RzxState::Idle)
        return {};
    if (m_progress.state == RzxState::Recording)
        return tr("RZX recording: %1 frames").arg(completedFrames());
    if (m_progress.state == RzxState::Reviewing)
        return tr("RZX recording: %1 / %2 frames").arg(completedFrames()).arg(totalFrames());
    const auto percent = totalFrames() ? 100 * completedFrames() / totalFrames() : 0;
    const auto frames =
            tr("%1 / %2 frames (%3%)").arg(completedFrames()).arg(totalFrames()).arg(percent);
    switch (m_progress.state) {
    case RzxState::Playing:
        return tr("RZX: %1").arg(frames);
    case RzxState::Completed:
        return tr("RZX complete: %1").arg(frames);
    case RzxState::Stopped:
        return tr("RZX stopped: %1").arg(frames);
    case RzxState::Error:
        return tr("RZX error: %1").arg(frames);
    default:
        return {};
    }
}

void ZxRecording::start()
{
    if (m_seeking)
        return;
    if (!savePendingRecording())
        return;
    const bool wasPaused = m_pauseOwned;
    if (m_emulator->emulationQuiescent() && !m_pauseOwned)
        pause();
    const bool started = m_emulator->withEmulator(
            [](zxkit::Emulator &emulator) { emulator.startRzxRecording(); });
    if (!started) {
        if (!wasPaused)
            releasePause();
        refresh();
        return;
    }
    releasePause();
    refresh();
    m_emulator->showMessage(tr("Recording started"));
}

void ZxRecording::stop()
{
    m_resumeAfterSeek = false;
    if (m_seeking) {
        m_seekTimer.stop();
        m_seekRequest.reset();
        m_seeking = false;
        m_emulator->withEmulator([](zxkit::Emulator &e) { e.cancelRzxSeek(); });
        emit transportChanged();
    }
    if (!m_emulator->emulationQuiescent()) {
        pause();
        m_stopRequested = true;
        m_seekTimer.start(10);
        return;
    }
    if (m_pendingRecording.empty()) {
        if (!m_emulator->withEmulator([this](zxkit::Emulator &emulator) {
                auto &rzx = emulator.rzx();
                if (rzx.isRecording() || rzx.isReviewing())
                    m_pendingRecording = emulator.stopRzxRecording();
                else
                    emulator.stopRzxPlayback();
            }))
            return;
        if (!m_pendingRecording.empty())
            m_pendingPath = m_emulator->recordingFilePath();
    }
    savePendingRecording();
    releasePause();
    refresh();
}

void ZxRecording::pause()
{
    ++m_generation;
    if (!m_pauseOwned) {
        m_pauseOwned = true;
        m_emulator->setPaused(true);
    }
    refresh();
    emit transportChanged();
}

void ZxRecording::releasePause()
{
    ++m_generation;
    if (m_pauseOwned) {
        m_pauseOwned = false;
        m_emulator->setPaused(false);
        emit transportChanged();
    }
}

void ZxRecording::resume()
{
    if (m_seeking || !m_pauseOwned)
        return;
    if (!m_emulator->emulationQuiescent()) {
        m_resumeAfterSeek = true;
        m_seekTimer.start(10);
        return;
    }
    if (!m_emulator->withEmulator([](zxkit::Emulator &e) {
            if (e.rzx().isReviewing())
                e.resumeRzxRecording();
        }))
        return;
    const auto generation = m_generation;
    // The input which resumed us must reach the guest before its next instruction.
    QTimer::singleShot(0, this, [this, generation] {
        if (generation == m_generation && !m_seeking) {
            releasePause();
            refresh();
        }
    });
}

bool ZxRecording::resumeOnGameplayInput()
{
    if (!m_pauseOwned || m_seeking || !m_emulator->canResumeRzxInput()
        || (m_type != Type::Recording && m_type != Type::Playback))
        return false;
    resume();
    return true;
}

void ZxRecording::seek(qint64 milliseconds)
{
    if (m_type == Type::Unsaved || !hasProgress() || !playbackError().isEmpty())
        return;
    pause();
    m_seekRequest = static_cast<quint64>(std::max<qint64>(0, milliseconds));
    m_seeking = true;
    m_resumeAfterSeek = false;
    m_stopRequested = false;
    m_seekProgressClock.start();
    m_seekTimer.start(10);
    emit transportChanged();
}

void ZxRecording::rewind(qint64 milliseconds)
{
    if (milliseconds < 0)
        return;
    refresh();
    seek(static_cast<qint64>(m_positionMs) - std::min<quint64>(m_positionMs, milliseconds));
}

void ZxRecording::fastForward(qint64 milliseconds)
{
    if (milliseconds < -1)
        return;
    refresh();
    seek(milliseconds == -1
                 ? m_durationMs
                 : m_positionMs + std::min<quint64>(milliseconds, m_durationMs - m_positionMs));
}

void ZxRecording::startPlayback()
{
    seek(0);
    m_resumeAfterSeek = m_seeking;
}

void ZxRecording::advanceSeek()
{
    if (!m_emulator->emulationQuiescent())
        return;
    if (m_stopRequested) {
        m_stopRequested = false;
        m_seekTimer.stop();
        stop();
        return;
    }
    m_seekTimer.setInterval(0);
    bool done = true;
    if (!m_emulator->withEmulator([this, &done](zxkit::Emulator &e) {
            if (m_seekRequest) {
                const auto request = std::exchange(m_seekRequest, std::nullopt);
                e.beginRzxSeek(*request);
            }
            done = e.advanceRzxSeek(2);
        })) {
        m_seekRequest.reset();
        m_seeking = false;
        m_resumeAfterSeek = false;
        m_seekTimer.stop();
        refresh();
        emit transportChanged();
        return;
    }
    if (!done) {
        if (m_seekProgressClock.elapsed() >= 200) {
            refresh();
            m_emulator->engine().refreshFrame();
            m_seekProgressClock.restart();
        }
        return;
    }
    m_seekTimer.stop();
    m_seeking = false;
    m_emulator->engine().refreshFrame();
    refresh();
    emit transportChanged();
    if (std::exchange(m_resumeAfterSeek, false))
        resume();
}

bool ZxRecording::savePendingRecording()
{
    if (m_pendingRecording.empty())
        return true;
    if (!m_emulator->writeFileBytes(m_pendingPath, m_pendingRecording))
        return false;
    m_emulator->showMessage(tr("Recording saved to %1").arg(m_pendingPath));
    m_pendingRecording.clear();
    m_pendingPath.clear();
    return true;
}
