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
#include "audiooutput.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <thread>

using namespace Qt::StringLiterals;

class TestAudioOutput : public QObject
{
    Q_OBJECT
private slots:
    void continuousClock_data()
    {
        QTest::addColumn<uint32_t>("rate");
        QTest::addColumn<uint32_t>("speed");
        QTest::addColumn<int>("clockPpm");
        for (uint32_t rate : {44'100u, 48'000u, 96'000u})
            for (uint32_t speed : {50u, 100u, 200u})
                for (int drift : {-300, 0, 300})
                    QTest::newRow(qPrintable(u"%1-%2-%3"_s.arg(rate).arg(speed).arg(drift))) << rate << speed << drift;
    }
    void continuousClock()
    {
        QFETCH(uint32_t, rate);
        QFETCH(uint32_t, speed);
        QFETCH(int, clockPpm);
        AudioBuffer buffer(rate);
        buffer.setActive(true);
        std::array<int16_t, 1'920> source;
        source.fill(8'192);
        // Deterministic device and producer clocks, with frame-sized delivery
        // bursts and a 300 ppm hardware clock mismatch. No sleeps or hardware.
        constexpr uint32_t callback = 256;
        std::array<float, callback * 2> output;
        const double secondsPerCallback = double(callback) / (rate * (1 + clockPpm / 1'000'000.0));
        double nextFrame = 0;
        double now = 0;
        bool playing = false;
        for (unsigned i = 0; now < ((clockPpm && rate == 48'000 && speed == 100) ? 600 : 12); ++i, now += secondsPerCallback) {
            while (nextFrame <= now) {
                buffer.push(source, speed);
                nextFrame += 0.020 * 100 / speed;
            }
            buffer.render(output);
            if (now > 0.5) {
                for (float value : output)
                    QVERIFY2(std::abs(value - .25f) < .0001f, "Gap or amplitude discontinuity in continuous PCM");
                playing = true;
            }
        }
        QVERIFY(playing);
        QCOMPARE(buffer.statistics().underruns, 0u);
        QCOMPARE(buffer.statistics().overflows, 0u);
        QVERIFY(buffer.statistics().renderedFrames > rate * 10);
    }
    void speedChangesKeepLatencyBounded()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        std::array<int16_t, 1'920> source;
        source.fill(8'192);
        std::array<float, 512> output;
        double budget = 0;
        constexpr double period = 256.0 / 48'000;
        for (double now = 0; now < 4; now += period) {
            const uint32_t speed = now < 2 ? 400 : 100;
            budget += period * 48'000 * speed / 100;
            while (budget >= 960) {
                buffer.push(source, speed);
                budget -= 960;
            }
            buffer.render(output);
            if (now > 0.2)
                QVERIFY(std::ranges::all_of(output, [](float s) { return std::abs(s - .25f) < .0001f; }));
            // A 60 ms 4x buffer must remain 60 ms after returning to 100%,
            // rather than stretching into 240 ms of delayed game sound.
            if (now > 2.5)
                QVERIFY(buffer.statistics().queuedFrames < 5'000);
        }
        QCOMPARE(buffer.statistics().underruns, 0u);
        QCOMPARE(buffer.statistics().overflows, 0u);
    }
    void speedBoundaryPreservesWaveformPhase()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        constexpr unsigned boundary = 40'003;
        std::vector<int16_t> fast(boundary * 2), normal(12'000 * 2);
        const auto sample = [](double position) {
            return 8'000 * std::sin(2 * std::numbers::pi * position / 500);
        };
        for (unsigned i = 0; i < boundary; ++i)
            fast[i * 2] = fast[i * 2 + 1] = int16_t(sample(i));
        for (unsigned i = 0; i < 12'000; ++i)
            normal[i * 2] = normal[i * 2 + 1] = int16_t(sample(boundary + i));
        buffer.push(fast, 4'000);
        buffer.push(normal, 100);
        std::array<float, 512> output;
        unsigned frame = 0;
        for (unsigned block = 0; block < 6; ++block) {
            buffer.render(output);
            for (unsigned i = 0; i < 256; ++i, ++frame) {
                if (frame < 900) continue; // priming fade
                const double edge = boundary / 40.0;
                const double position = frame < edge ? frame * 40.0 : boundary + frame - edge;
                // Small occupancy-drift correction is allowed; skipping dozens
                // of input samples at a packet-rate boundary is not.
                QVERIFY(std::abs(output[i * 2] * 32'768 - sample(position)) < 150);
            }
        }
        QCOMPARE(buffer.statistics().underruns, 0u);
    }
    void turboResumeIgnoresPausedTime()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        std::array<int16_t, 1'920> source;
        source.fill(8'192);
        auto now = std::chrono::steady_clock::time_point{};
        for (int i = 0; i < 30; ++i) {
            now += std::chrono::milliseconds(5);
            buffer.push(source, 0, now);
        }
        buffer.setActive(false);
        now += std::chrono::seconds(10);
        buffer.setActive(true);
        for (int i = 0; i < 15; ++i) {
            now += std::chrono::milliseconds(5);
            buffer.push(source, 0, now);
        }
        buffer.render({}); // consume control epochs before measuring sample advance
        const auto queued = buffer.statistics().queuedFrames;
        std::array<float, 1'024> output;
        buffer.render(output);
        QVERIFY(queued - buffer.statistics().queuedFrames > 1'500);
        QCOMPARE(buffer.statistics().underruns, 0u);
    }
    void turboJitterKeepsDuration()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        std::array<int16_t, 1'920> source;
        source.fill(8'192);
        std::array<float, 512> output;
        double nextFrame = 0;
        unsigned frames = 0;
        constexpr double period = 256.0 / 48'000;
        for (double now = 0; now < 60; now += period) {
            while (nextFrame <= now) {
                const auto timestamp = std::chrono::steady_clock::time_point{}
                    + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                        std::chrono::duration<double>(nextFrame));
                buffer.push(source, 0, timestamp);
                nextFrame += (++frames & 1) ? .005 : .015; // 2x mean speed, bursty arrival
            }
            buffer.render(output);
            if (now > 1)
                QVERIFY(std::ranges::all_of(output, [](float s) { return std::abs(s - .25f) < .0001f; }));
        }
        QCOMPARE(buffer.statistics().underruns, 0u);
        QCOMPARE(buffer.statistics().overflows, 0u);
    }
    void pauseAndMuteGeneration()
    {
        const auto control = std::make_shared<AudioBuffer::Control>();
        AudioBuffer buffer(48'000, 2, control);
        std::vector<int16_t> source(8'000, 16'384);
        std::array<float, 960> output;
        control->setPaused(false);
        buffer.push(source);
        buffer.render(output);
        QVERIFY(output.back() > .49f);
        control->setPaused(true);
        buffer.render(output);
        QVERIFY(std::abs(output.back()) < .0001f);
        buffer.render(output);
        QVERIFY(std::ranges::all_of(output, [](float s) { return s == 0; }));
        // Resume and mute can happen between callbacks: stale samples must not
        // escape even when the consumer never saw the intermediate state.
        control->setPaused(false);
        buffer.push(source);
        control->setMuted(true);
        control->setMuted(false);
        std::ranges::fill(source, -16'384);
        buffer.push(source);
        buffer.render(output);
        QVERIFY(output.back() < -.49f);
        QCOMPARE(buffer.statistics().underruns, 0u);
    }
    void boundedAndInvalidInput()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        std::vector<int16_t> source((AudioBuffer::Capacity + 1) * 2, 0);
        buffer.push(source);
        QCOMPARE(buffer.statistics().overflows, 1u);
        QCOMPARE(buffer.statistics().queuedFrames, 0u);
        buffer.push(std::span(source).first(3));
        QCOMPARE(buffer.statistics().queuedFrames, 0u);
        QVERIFY_EXCEPTION_THROWN(AudioBuffer(0), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(AudioBuffer(48'000, 0), std::invalid_argument);
    }
    void realProducerAndConsumerThreads()
    {
        AudioBuffer buffer;
        buffer.setActive(true);
        std::array<int16_t, 1'920> source;
        source.fill(4'096);
        std::atomic_bool complete = false;
        std::jthread producer([&] {
            for (int n = 0; n < 2'000; ++n) {
                while (buffer.statistics().queuedFrames > 8'000)
                    std::this_thread::yield();
                buffer.push(source);
            }
            complete = true;
        });
        std::array<float, 512> output;
        while (!complete.load() || buffer.statistics().queuedFrames > 4'000) {
            if (buffer.statistics().queuedFrames < 4'000) {
                std::this_thread::yield();
                continue;
            }
            buffer.render(output);
            for (float value : output)
                QVERIFY(std::isfinite(value) && value >= 0 && value <= .126f);
        }
        producer.join();
        QCOMPARE(buffer.statistics().underruns, 0u);
        QCOMPARE(buffer.statistics().overflows, 0u);
    }
};

// Opt-in physical-backend check, separate from the hardware-independent CTest
// suite. Select an explicitly named disposable/null device; never change the
// system default output. The GUI/event loop deliberately stalls for 500 ms.
int liveOutput(QCoreApplication &app, const QString &deviceName)
{
    const auto devices = QMediaDevices::audioOutputs();
    const auto found = std::ranges::find_if(devices, [&](const QAudioDevice &device) {
        return device.description() == deviceName || device.id() == deviceName.toUtf8();
    });
    if (found == devices.end()) {
        qCritical() << "Audio test device not found:" << deviceName;
        return 2;
    }
    AudioOutput audio(*found);
    bool failed = false;
    QObject::connect(&audio, &AudioOutput::errorOccurred, &app, [&](const QString &message) {
        qCritical().noquote() << message;
        failed = true;
    });
    audio.start();
    audio.setPaused(false);
    std::jthread producer([&](std::stop_token token) {
        auto deadline = std::chrono::steady_clock::now();
        uint64_t position = 0;
        const auto started = deadline;
        while (!token.stop_requested()) {
            const auto elapsed = std::chrono::steady_clock::now() - started;
            const uint32_t speed = elapsed < std::chrono::seconds(4) ? 100
                                   : elapsed < std::chrono::seconds(8) ? 200
                                   : elapsed < std::chrono::seconds(12) ? 50 : 400;
            std::array<int16_t, 1'920> pcm;
            for (unsigned n = 0; n < 960; ++n, ++position)
                pcm[n * 2] = pcm[n * 2 + 1] = int16_t(12'000 * std::sin(2 * std::numbers::pi * 440 * position / 48'000.0));
            audio.write(pcm, 48'000, 2, speed == 400 ? 0 : speed);
            deadline += std::chrono::microseconds(2'000'000 / speed);
            std::this_thread::sleep_until(deadline);
        }
    });
    QTimer stall;
    QObject::connect(&stall, &QTimer::timeout, &app, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    });
    stall.start(1'000);
    QTimer::singleShot(16'000, &app, [&] {
        producer.request_stop();
        producer.join();
        const auto stats = audio.statistics();
        qInfo().noquote() << QJsonDocument(QJsonObject{
            {u"underruns"_s, int(stats.underruns)}, {u"overflows"_s, int(stats.overflows)},
            {u"renderedFrames"_s, int(stats.renderedFrames)}}).toJson(QJsonDocument::Compact);
        audio.stop();
        app.exit(failed || stats.underruns || stats.overflows || stats.renderedFrames < 650'000 ? 1 : 0);
    });
    return app.exec();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == u"--live-device"_s)
        return liveOutput(app, app.arguments()[2]);
    TestAudioOutput tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "test_audio_output.moc"
