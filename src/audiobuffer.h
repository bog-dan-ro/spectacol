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

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <span>
#include <memory>
#include <vector>

// Single Engine producer / Qt audio-thread consumer. Controls may be changed by
// another thread. No allocations, mutexes, Qt objects or I/O in push()/render().
class AudioBuffer final
{
public:
    static constexpr uint32_t InputRate = 48'000;
    static constexpr uint32_t Capacity = 65'536; // bounded, including very fast playback
    static constexpr uint32_t RateScale = 65'536;
    struct Statistics
    {
        uint32_t underruns = 0;
        uint32_t overflows = 0;
        uint32_t queuedFrames = 0;
        uint32_t renderedFrames = 0;
    };

    struct Control
    {
        void setPaused(bool paused) noexcept
        {
            setFlag(1, paused);
        }
        void setMuted(bool muted) noexcept
        {
            setFlag(2, muted);
        }

    private:
        friend class AudioBuffer;
        void setFlag(uint32_t flag, bool on) noexcept;
        std::atomic<uint32_t> state{ 1 }; // low two bits: paused, muted
    };
    explicit AudioBuffer(uint32_t outputRate = InputRate, uint32_t channels = 2,
                         std::shared_ptr<Control> control = {});
    void setActive(bool active) noexcept;
    // speedPercent == 0 measures the producer's throughput for unbounded turbo.
    void push(std::span<const int16_t> stereo, uint32_t speedPercent = 100) noexcept;
    // Explicit producer timestamps also make throughput/jitter tests deterministic.
    void push(std::span<const int16_t> stereo, uint32_t speedPercent,
              std::chrono::steady_clock::time_point now) noexcept;
    void render(std::span<float> output) noexcept;
    Statistics statistics() const noexcept;

private:
    struct Frame
    {
        int16_t left = 0, right = 0;
        uint32_t generation = 0;
        uint32_t rate = RateScale;
    };
    uint32_t targetFrames(double rate) const noexcept;
    void beginFade() noexcept;

    std::vector<Frame> m_frames;
    const uint32_t m_outputRate;
    const uint32_t m_channels;
    alignas(64) std::atomic<uint32_t> m_write{ 0 };
    alignas(64) std::atomic<uint32_t> m_read{ 0 };
    const std::shared_ptr<Control> m_control;
    std::atomic<uint32_t> m_rate{ RateScale };
    std::atomic<uint32_t> m_underruns{ 0 }, m_overflows{ 0 }, m_rendered{ 0 };
    // Producer-only rate estimation.
    std::chrono::steady_clock::time_point m_previousPush = {};
    uint32_t m_previousFrames = 0;
    uint32_t m_producerGeneration = 0;
    double m_turboRate = 1;
    // Consumer-only interpolation, priming and click-free transitions.
    uint32_t m_generation = 0;
    bool m_primed = false;
    double m_fraction = 0;
    double m_drift = 0;
    float m_gain = 0;
    std::array<float, 2> m_last{}, m_rampStart{}, m_fadeStep{};
    uint32_t m_fadeRemaining = 0;
};
