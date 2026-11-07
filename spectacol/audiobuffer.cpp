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
#include "audiobuffer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

AudioBuffer::AudioBuffer(uint32_t outputRate, uint32_t channels, std::shared_ptr<Control> control)
    : m_frames(Capacity),
      m_outputRate(outputRate),
      m_channels(channels),
      m_control(control ? std::move(control) : std::make_shared<Control>())
{
    if (!outputRate || (channels != 1 && channels != 2))
        throw std::invalid_argument("Audio output needs a sample rate and one or two channels");
}

void AudioBuffer::Control::setFlag(uint32_t flag, bool on) noexcept
{
    auto current = state.load(std::memory_order_relaxed);
    while (bool(current & flag) != on
           && !state.compare_exchange_weak(current, ((current + 4) & ~flag) | (on ? flag : 0),
                                           std::memory_order_release, std::memory_order_relaxed)) {
    }
}

void AudioBuffer::setActive(bool active) noexcept
{
    m_control->setPaused(!active);
}

void AudioBuffer::beginFade() noexcept
{
    m_fadeRemaining = (std::max)(1u, m_outputRate / 200);
    m_fadeStep = { m_last[0] / m_fadeRemaining, m_last[1] / m_fadeRemaining };
    m_gain = 0;
}

void AudioBuffer::push(std::span<const int16_t> stereo, uint32_t speedPercent) noexcept
{
    if (!(m_control->state.load(std::memory_order_acquire) & 3))
        push(stereo, speedPercent, std::chrono::steady_clock::now());
}

void AudioBuffer::push(std::span<const int16_t> stereo, uint32_t speedPercent,
                       std::chrono::steady_clock::time_point now) noexcept
{
    const auto control = m_control->state.load(std::memory_order_acquire);
    if ((control & 3) || stereo.size() % 2 || stereo.empty())
        return;
    const auto count = uint32_t(stereo.size() / 2);
    if (control != m_producerGeneration) {
        m_producerGeneration = control;
        m_previousFrames = 0; // a deliberate pause/mute is not slow production
    }
    double rate = speedPercent / 100.0;
    if (!speedPercent) {
        if (m_previousFrames && now > m_previousPush) {
            const auto seconds = std::chrono::duration<double>(now - m_previousPush).count();
            // Average time per input sample, not instantaneous frequency:
            // reciprocal averaging would bias duration under scheduling jitter.
            const auto period =
                    std::clamp(seconds * InputRate / m_previousFrames, 1.0 / 256, 100.0);
            m_turboRate = 1.0 / (0.8 / m_turboRate + period * 0.2);
        }
        rate = m_turboRate;
    } else {
        m_turboRate = rate;
    }
    m_previousPush = now;
    m_previousFrames = count;
    const auto encodedRate = uint32_t(std::clamp(rate, 0.01, 256.0) * RateScale);
    m_rate.store(encodedRate, std::memory_order_release);
    const auto write = m_write.load(std::memory_order_relaxed);
    const auto read = m_read.load(std::memory_order_acquire);
    if (count > Capacity - (write - read)) {
        m_overflows.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    for (uint32_t i = 0; i < count; ++i)
        m_frames[(write + i) & (Capacity - 1)] = { stereo[i * 2], stereo[i * 2 + 1], control,
                                                   encodedRate };
    m_write.store(write + count, std::memory_order_release);
}

uint32_t AudioBuffer::targetFrames(double rate) const noexcept
{
    // Two source video frames absorb bursty delivery; 60 ms covers scheduling
    // jitter at ordinary speeds. Capacity stays bounded during unbounded turbo.
    return uint32_t(std::clamp(InputRate * 0.060 * rate, 1'920.0, Capacity / 2.0));
}

void AudioBuffer::render(std::span<float> output) noexcept
{
    const auto control = m_control->state.load(std::memory_order_acquire);
    auto read = m_read.load(std::memory_order_relaxed);
    const auto write = m_write.load(std::memory_order_acquire);
    if (control != m_generation) {
        beginFade();
        m_generation = control;
        m_primed = false;
        m_fraction = 0;
        m_drift = 0;
        // Discard earlier epochs, but retain fresh samples already supplied
        // after resume. The per-frame stamp also handles a late old producer.
        while (read != write && m_frames[read & (Capacity - 1)].generation != control)
            ++read;
    }
    // Queued PCM keeps the speed at which it was generated. Applying a new
    // speed to older packets would stretch a turbo buffer into a long backlog.
    const double rate = (read != write ? m_frames[read & (Capacity - 1)].rate
                                       : m_rate.load(std::memory_order_acquire))
            / double(RateScale);
    const auto target = targetFrames(rate);
    if (control & 3) {
        read = write;
        m_primed = false;
    } else if (!m_primed && write - read >= target) {
        m_primed = true;
        m_fraction = 0;
        m_rampStart = m_last;
        m_gain = 0;
    }
    // A slow occupancy feedback corrects device-clock drift without touching
    // emulation speed. At normal speeds the pitch correction is at most 0.1%.
    const double desiredDrift =
            std::clamp((double(write - read) - target) / target * 0.01, -0.001, 0.001);
    const double duration = double(output.size() / m_channels) / m_outputRate;
    m_drift += (desiredDrift - m_drift) * (std::min)(duration, 1.0);
    const double outputRatio = double(InputRate) / m_outputRate * (1 + m_drift) / RateScale;
    const float ramp = 1.0f / (0.005f * m_outputRate);
    uint32_t renderedFrames = 0;
    for (std::size_t i = 0; i + m_channels <= output.size(); i += m_channels) {
        std::array<float, 2> value{};
        if (m_primed) {
            if (write - read < 2 || m_frames[read & (Capacity - 1)].generation != control
                || m_frames[(read + 1) & (Capacity - 1)].generation != control) {
                m_primed = false;
            } else {
                const auto &a = m_frames[read & (Capacity - 1)];
                const auto &b = m_frames[(read + 1) & (Capacity - 1)];
                value = { float((a.left + (b.left - a.left) * m_fraction) / 32768.0),
                          float((a.right + (b.right - a.right) * m_fraction) / 32768.0) };
                // Advance one output sample in time, honoring rate boundaries
                // inside that interval. A fast packet must not skip the first
                // samples of a following normal-speed packet.
                double remaining = 1;
                while (remaining > 0) {
                    if (write - read < 2) {
                        m_primed = false;
                        break;
                    }
                    const double step = m_frames[read & (Capacity - 1)].rate * outputRatio;
                    const double untilNext = (1 - m_fraction) / step;
                    if (untilNext > remaining) {
                        m_fraction += remaining * step;
                        remaining = 0;
                        if (m_fraction >= 1) {
                            ++read;
                            m_fraction = 0;
                        }
                    } else {
                        remaining -= untilNext;
                        ++read;
                        m_fraction = 0;
                    }
                }
            }
            if (!m_primed) {
                beginFade();
                // A control transition during the callback is a deliberate
                // pause/mute, not starvation of a running producer.
                if (m_control->state.load(std::memory_order_acquire) == control)
                    m_underruns.fetch_add(1, std::memory_order_relaxed);
                read = write;
                m_fraction = 0;
            }
        }

        if (m_primed) {
            ++renderedFrames;
            m_gain = (std::min)(1.0f, m_gain + ramp);
            value[0] = value[0] * m_gain + m_rampStart[0] * (1 - m_gain);
            value[1] = value[1] * m_gain + m_rampStart[1] * (1 - m_gain);
            m_last = value;
        } else {
            // Bounded fade on a deliberate pause or unavoidable source underrun.
            // Never repeat an old frame indefinitely or cut a nonzero sample off.
            if (m_fadeRemaining) {
                m_last[0] -= m_fadeStep[0];
                m_last[1] -= m_fadeStep[1];
                --m_fadeRemaining;
            } else {
                m_last = {};
            }
            value = m_last;
        }
        if (m_channels == 1) {
            output[i] = (value[0] + value[1]) * 0.5f;
        } else {
            output[i] = value[0];
            output[i + 1] = value[1];
        }
    }
    m_rendered.fetch_add(renderedFrames, std::memory_order_relaxed);
    m_read.store(read, std::memory_order_release);
}

AudioBuffer::Statistics AudioBuffer::statistics() const noexcept
{
    const auto read = m_read.load(std::memory_order_acquire);
    const auto write = m_write.load(std::memory_order_acquire);
    return { m_underruns.load(std::memory_order_relaxed),
             m_overflows.load(std::memory_order_relaxed), (std::min)(write - read, Capacity),
             m_rendered.load(std::memory_order_relaxed) };
}
