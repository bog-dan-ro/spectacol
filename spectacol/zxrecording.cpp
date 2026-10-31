/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#include <QFile>

ZxRecording::ZxRecording(ZxEmulator *parent)
    : QObject(parent)
    , m_emulator(parent)
{
}

void ZxRecording::setStopVisible(bool visible)
{
    Q_UNUSED(visible)
    Type t = Type::None;
    {
        const auto emulator = m_emulator->emulator().readLock();
        const auto &rzx = emulator->rzx();
        t = rzx.isRecording() ? Type::Recording : (rzx.isPlaying() ? Type::Playback : Type::None);
    }
    if (m_type != t) {
        m_type = t;
        emit typeChanged(m_type);
    }
}

void ZxRecording::start()
{
    const bool started = m_emulator->withEmulator([](zxkit::Emulator &emulator) {
        emulator.rzx().startRecording(emulator.saveSnapshot(".szx"), ".szx", emulator.ioBus());
    });
    if (!started)
        return;
    setStopVisible(true);
    m_emulator->showMessage(tr("Recording started"));
}

void ZxRecording::continueRecording()
{
    start();
}

void ZxRecording::rollback()
{
    try {
        m_emulator->emulator().writeLock()->rewind(50);
        m_emulator->engine().refreshFrame();
    } catch (const std::exception &) {
        m_emulator->showMessage(tr("Cannot rollback."), ZxEmulator::Warning);
    }
}

void ZxRecording::rollbackTo()
{
    rollback();
}

void ZxRecording::finalise()
{
    stop();
}

void ZxRecording::stop()
{
    std::vector<uint8_t> bytes;
    m_emulator->withEmulator([&bytes](zxkit::Emulator &emulator) {
        auto &rzx = emulator.rzx();
        if (rzx.isRecording())
            bytes = rzx.stopRecording(emulator.ioBus());
        else
            rzx.clear();
    });
    if (!bytes.empty()) {
        const QString path = m_emulator->recordingFilePath();
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QByteArrayView(bytes).data(), qint64(bytes.size()));
            m_emulator->showMessage(tr("Recording saved to %1").arg(path));
        }
    }
    setStopVisible(false);
}
