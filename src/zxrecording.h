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

#pragma once

#include <QQmlEngine>

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <zxkit/media/RzxFormat.h>
#include <vector>
#include <cstdint>

class ZxEmulator;
class ZxRecording : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use ZxEmulator.recording instead")

    Q_PROPERTY(Type type READ type NOTIFY typeChanged)
    Q_PROPERTY(quint64 completedFrames READ completedFrames NOTIFY progressChanged)
    Q_PROPERTY(quint64 totalFrames READ totalFrames NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)
    Q_PROPERTY(QString playbackError READ playbackError NOTIFY progressChanged)
    Q_PROPERTY(bool hasProgress READ hasProgress NOTIFY progressChanged)
    Q_PROPERTY(bool waitingForInput READ waitingForInput NOTIFY transportChanged)
    Q_PROPERTY(bool seeking READ seeking NOTIFY transportChanged)
    Q_PROPERTY(quint64 positionMs READ positionMs NOTIFY transportChanged)
    Q_PROPERTY(quint64 durationMs READ durationMs NOTIFY transportChanged)

public:
    enum class Type { None, Playback, Recording, Unsaved };
    Q_ENUM(Type)

public:
    explicit ZxRecording(ZxEmulator *parent = nullptr);
    Type type() const
    {
        return m_type;
    }
    quint64 completedFrames() const
    {
        return m_progress.completedFrames;
    }
    quint64 totalFrames() const
    {
        return m_progress.totalFrames;
    }
    QString statusText() const;
    QString playbackError() const
    {
        return QString::fromStdString(m_progress.error);
    }
    bool hasProgress() const
    {
        return m_progress.state != zxkit::RzxState::Idle;
    }
    void refresh();
    void mediaLoaded();
    bool waitingForInput() const
    {
        return m_pauseOwned;
    }
    bool seeking() const
    {
        return m_seeking;
    }
    quint64 positionMs() const
    {
        return m_positionMs;
    }
    quint64 durationMs() const
    {
        return m_durationMs;
    }
    bool resumeOnGameplayInput();

public slots:
    void start();
    void stop();
    void pause();
    void resume();
    void startPlayback();
    void seek(qint64 milliseconds);
    void rewind(qint64 milliseconds = 5000);
    void fastForward(qint64 milliseconds = 5000);

signals:
    void typeChanged(Type type);
    void progressChanged();
    void transportChanged();

private:
    bool savePendingRecording();
    void advanceSeek();
    void releasePause();
    std::vector<uint8_t> m_pendingRecording;
    QString m_pendingPath;
    ZxEmulator *m_emulator;
    Type m_type = Type::None;
    zxkit::RzxProgress m_progress;
    QTimer m_progressTimer;
    QTimer m_seekTimer;
    QElapsedTimer m_seekProgressClock;
    bool m_pauseOwned = false, m_seeking = false, m_resumeAfterSeek = false,
         m_stopRequested = false;
    std::optional<quint64> m_seekRequest = std::nullopt;
    quint64 m_positionMs = 0, m_durationMs = 0, m_generation = 0;
};
