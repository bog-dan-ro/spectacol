/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "audiobuffer.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QObject>
#include <memory>
#include <mutex>

class QAudioSink;

class AudioOutput final : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(const QAudioDevice &device = {}, QObject *parent = nullptr);
    ~AudioOutput() override;
    // Device lifecycle stays on the owner thread; PCM never passes through it.
    void start();
    void stop();
    void setPaused(bool paused) noexcept
    {
        m_control->setPaused(paused);
    }
    void setMuted(bool muted) noexcept
    {
        m_control->setMuted(muted);
    }
    void write(std::span<const int16_t> samples, uint32_t rate, uint8_t channels,
               uint32_t speedPercent);
    AudioBuffer::Statistics statistics() const;
Q_SIGNALS:
    void errorOccurred(const QString &message);

private:
    Q_DISABLE_COPY_MOVE(AudioOutput)
    std::shared_ptr<AudioBuffer> buffer() const;
    void publishBuffer(std::shared_ptr<AudioBuffer> next);
    QMediaDevices m_devices;
    const QAudioDevice m_device;
    QByteArray m_activeDevice;
    std::shared_ptr<AudioBuffer::Control> m_control;
    // Used only to take/publish a producer snapshot. Never held across a Qt
    // call, destruction, or sample processing; never used by the audio callback.
    mutable std::mutex m_bufferMutex;
    std::shared_ptr<AudioBuffer> m_buffer;
    std::unique_ptr<QAudioSink> m_sink;
};
