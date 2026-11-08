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
#include <QAbstractSocket>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QVariantMap>
#include <array>
#include <atomic>
#include <memory>
#include <zxkit/core/Engine.h>

class NextUartRuntime;
class UartScriptApi;

/// The QObject and sockets live on the UI thread; the runtime and every QJSValue
/// are created, called and destroyed exclusively on Engine's emulation thread.
class NextUartHooks : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ZxEmulator.uartHooks")
    Q_PROPERTY(QString espScript READ espScript WRITE setEspScript NOTIFY configurationChanged)
    Q_PROPERTY(QString piScript READ piScript WRITE setPiScript NOTIFY configurationChanged)
    Q_PROPERTY(QString piHost READ piHost WRITE setPiHost NOTIFY configurationChanged)
    Q_PROPERTY(int piPort READ piPort WRITE setPiPort NOTIFY configurationChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY statusChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY statusChanged)
public:
    explicit NextUartHooks(QObject *parent = nullptr);
    ~NextUartHooks() override;
    zxkit::EngineConfig::ThreadHooks threadHooks();
    QString espScript() const
    {
        return m_paths[0];
    }
    QString piScript() const
    {
        return m_paths[1];
    }
    QString piHost() const
    {
        return m_piHost;
    }
    int piPort() const
    {
        return m_piPort;
    }
    QString errorString() const
    {
        return m_error;
    }
    bool downloading() const
    {
        return m_downloading;
    }
    void setEspScript(const QString &path);
    void setPiScript(const QString &path);
    void setPiHost(const QString &host);
    void setPiPort(int port);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void downloadScript(int channel, const QUrl &url);
    Q_INVOKABLE void useBundled(int channel);
signals:
    void configurationChanged();
    void statusChanged();

public:
    // Used by the runtime, never accesses JS from the QObject's thread.
    void command(QVariantMap command);
    void reportError(const QString &error);
    quint64 runtimeThreadId() const
    {
        return m_threadId.load();
    }

private:
    friend class NextUartRuntime;
    friend class UartScriptApi;
    void configure();
    void processCommands();
    void networkEvent(quint64 epoch, unsigned channel, QVariantMap event);
    QStringList m_paths;
    QString m_piHost;
    int m_piPort = 0;
    QByteArray m_savedNvram;
    QString m_error;
    bool m_downloading = false;
    QNetworkAccessManager m_downloads;
    QHash<int, QPointer<QAbstractSocket>> m_sockets;
    QMutex m_mutex;
    struct Configuration
    {
        std::array<QString, 2> source;
        QString piHost;
        int piPort = 0;
    } m_configuration;
    QList<QVariantMap> m_commands, m_events;
    std::atomic<uint64_t> m_revision{ 0 };
    std::atomic<bool> m_eventsPending{ false }, m_commandsPending{ false };
    std::atomic<quint64> m_threadId{ 0 };
    std::unique_ptr<NextUartRuntime> m_runtime;
};

class UartScriptApi : public QObject
{
    Q_OBJECT
public:
    UartScriptApi(NextUartRuntime &runtime, unsigned channel)
        : m_runtime(runtime), m_channel(channel)
    {
    }
    Q_INVOKABLE int receive(const QVariantList &bytes);
    Q_INVOKABLE void network(const QString &operation, int id, const QString &host = {},
                             int port = 0, const QVariantList &bytes = {}, bool udp = false);
    Q_INVOKABLE QVariantMap configuration() const;
    Q_INVOKABLE void setTimer(int id, int milliseconds);
    Q_INVOKABLE void cancelTimer(int id);
    Q_INVOKABLE void log(const QString &message);

private:
    NextUartRuntime &m_runtime;
    unsigned m_channel;
};
