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

#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQmlEngine>

#include <atomic>
#include <thread>

class QNetworkReply;

/// The ZX Spectrum Next boots NextZXOS from an SD card image. Like jnext, the
/// image comes from the official emulator distribution on specnext.com: it is
/// downloaded once (resumable) and the 1GB image is streamed out of the zip.
class NextSdCard : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)
    Q_PROPERTY(bool available READ available NOTIFY imagePathChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)
    Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY stateChanged)
    Q_PROPERTY(QString backupPath READ backupPath NOTIFY stateChanged)

public:
    enum State { Idle, Checking, Downloading, Extracting, Ready, Failed };
    Q_ENUM(State)

    static NextSdCard *create(QQmlEngine *, QJSEngine *);
    static NextSdCard &instance();
    ~NextSdCard() override;

    /// The SD card image used by the Next: a user chosen one, or the downloaded one.
    QString imagePath() const;
    /// An empty path: the downloaded image
    void setImagePath(const QString &path);
    /// The user picked the image (it isn't the downloaded one)
    bool isCustomImage() const;
    bool available() const;

    State state() const
    {
        return m_state;
    }
    double progress() const
    {
        return m_progress;
    }
    QString errorString() const
    {
        return m_errorString;
    }
    QString downloadUrl() const;
    QString backupPath() const
    {
        return m_backupPath;
    }

    /// Asks the UI to offer the download (emits `downloadRequested`).
    void requestDownload();

public slots:
    void download();
    void updateDistro();
    void cancel();

signals:
    void imagePathChanged();
    void stateChanged();
    void progressChanged();
    void downloadRequested();
    void ready();

private:
    explicit NextSdCard(QObject *parent = nullptr);
    QString defaultImagePath() const;
    QString zipPath() const;
    void setState(State state, const QString &error = {});
    void setProgress(double progress);
    void startExtraction();
    void stopExtraction();

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QFile m_zipPart;
    qint64 m_expectedSize = -1; // the whole zip, as announced by the server
    bool m_forceDownload = false;
    QString m_backupPath;
    bool m_freshZip = false; // the zip was downloaded by this attempt
    std::jthread m_extractThread;
    std::atomic<bool> m_cancel{ false };
    State m_state = Idle;
    double m_progress = 0;
    QString m_errorString;
};
