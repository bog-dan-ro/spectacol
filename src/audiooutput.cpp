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
#include "audiooutput.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QThread>
#include <algorithm>

AudioOutput::AudioOutput(const QAudioDevice &device, QObject *parent)
    : QObject(parent), m_device(device), m_control(std::make_shared<AudioBuffer::Control>())
{
    connect(&m_devices, &QMediaDevices::audioOutputsChanged, this, [this] {
        if (m_device.isNull() && QMediaDevices::defaultAudioOutput().id() != m_activeDevice)
            start();
    });
}

AudioOutput::~AudioOutput()
{
    stop();
}

void AudioOutput::start()
{
    Q_ASSERT(QThread::currentThread() == thread());
    stop();
    const auto device = m_device.isNull() ? QMediaDevices::defaultAudioOutput() : m_device;
    if (device.isNull()) {
        emit errorOccurred(tr("No audio output device is available."));
        return;
    }
    QAudioFormat format;
    // Float callbacks avoid an extra PCM copy on the common device path.
    for (const auto type : { QAudioFormat::Float, QAudioFormat::Int16 }) {
        for (const auto rate :
             { int(AudioBuffer::InputRate), device.preferredFormat().sampleRate() }) {
            for (const auto channels : { 2, 1 }) {
                format.setSampleFormat(type);
                format.setSampleRate(rate);
                format.setChannelCount(channels);
                if (device.isFormatSupported(format))
                    break;
            }
            if (device.isFormatSupported(format))
                break;
        }
        if (device.isFormatSupported(format))
            break;
    }
    if (!device.isFormatSupported(format)) {
        emit errorOccurred(tr("The audio output does not support PCM playback."));
        return;
    }
    const auto buffer =
            std::make_shared<AudioBuffer>(format.sampleRate(), format.channelCount(), m_control);
    m_sink = std::make_unique<QAudioSink>(device, format);
    m_activeDevice = device.id();
#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
    m_sink->setNativePeriodFrameCount(512);
#endif
    connect(m_sink.get(), &QAudioSink::stateChanged, this, [this](QtAudio::State state) {
        if (state == QtAudio::StoppedState && m_sink && m_sink->error() != QtAudio::NoError)
            emit errorOccurred(tr("Audio playback failed (error %1).").arg(int(m_sink->error())));
    });
    publishBuffer(buffer);
    if (format.sampleFormat() == QAudioFormat::Float) {
        m_sink->start([buffer](QSpan<float> output) {
            buffer->render({ output.data(), std::size_t(output.size()) });
        });
    } else {
        m_sink->start([buffer](QSpan<int16_t> output) {
            std::array<float, 1'024> temporary;
            for (qsizetype done = 0; done < output.size();) {
                const auto count = (std::min)(qsizetype(temporary.size()), output.size() - done);
                buffer->render(std::span(temporary).first(std::size_t(count)));
                for (qsizetype i = 0; i < count; ++i)
                    output[done + i] = int16_t(
                            std::clamp(temporary[std::size_t(i)] * 32768.0f, -32768.0f, 32767.0f));
                done += count;
            }
        });
    }
    if (m_sink->error() != QtAudio::NoError) {
        emit errorOccurred(
                tr("Can't start the audio output (error %1).").arg(int(m_sink->error())));
        stop();
    }
}

void AudioOutput::stop()
{
    Q_ASSERT(QThread::currentThread() == thread());
    publishBuffer({});
    // Destroying the sink joins/releases its callback before releasing the queue.
    m_sink.reset();
}

void AudioOutput::write(std::span<const int16_t> samples, uint32_t rate, uint8_t channels,
                        uint32_t speedPercent)
{
    if (rate != AudioBuffer::InputRate || channels != 2)
        return;
    if (const auto stream = buffer())
        stream->push(samples, speedPercent);
}

AudioBuffer::Statistics AudioOutput::statistics() const
{
    if (const auto stream = buffer())
        return stream->statistics();
    return {};
}

std::shared_ptr<AudioBuffer> AudioOutput::buffer() const
{
    const std::lock_guard lock(m_bufferMutex);
    return m_buffer;
}

void AudioOutput::publishBuffer(std::shared_ptr<AudioBuffer> next)
{
    {
        const std::lock_guard lock(m_bufferMutex);
        m_buffer.swap(next);
    }
    // The retired queue is released outside the short publication lock.
}
