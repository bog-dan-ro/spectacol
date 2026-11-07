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

#include "nextuarthooks.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QThread>
#include <QUdpSocket>
#include <chrono>
#include <thread>
#include <zxkit/machines/ZxNextMachine.h>

using namespace Qt::StringLiterals;
namespace {
uint64_t uartTime(const zxkit::Emulator &em)
{
    const auto &clock = em.clock();
    return clock.frameCount() * uint64_t(clock.timings().tstatesPerFrame())
            + clock.frameBaseTstates();
}
QByteArray byteArray(const QVariantList &bytes)
{
    QByteArray result;
    result.reserve(bytes.size());
    for (const auto &byte : bytes)
        result.append(char(byte.toUInt() & 255));
    return result;
}
QVariantList byteList(const QByteArray &bytes)
{
    QVariantList result;
    result.reserve(bytes.size());
    for (unsigned char byte : bytes)
        result.append(unsigned(byte));
    return result;
}
quint64 steadyNanos()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
}
} // namespace

class NextUartRuntime
{
public:
    NextUartHooks &host;
    QJSEngine engine; // Constructed by Engine::ThreadHooks, never by the UI.
    std::array<std::unique_ptr<UartScriptApi>, 2> api;
    std::array<QJSValue, 2> modules;
    struct Methods
    {
        QJSValue reset, onByte, onNetwork, onTimer;
    };
    std::array<Methods, 2> methods;
    NextUartHooks::Configuration configuration;
    zxkit::Emulator *emulator = nullptr;
    zxkit::NextUart *uart = nullptr;
    std::thread::id owner = std::this_thread::get_id();
    uint64_t revision = ~uint64_t(0), generation = ~uint64_t(0), epoch = 0;
    std::array<std::array<uint64_t, 5>, 2> socketEpoch{};
    QMutex deferredMutex;
    QList<QPair<unsigned, uint8_t>> deferred;
    std::array<QHash<int, quint64>, 2> timers;
    QMutex evaluationMutex;
    std::atomic<quint64> deadline{ 0 };
    std::jthread watchdog; // Destroyed before engine.

    explicit NextUartRuntime(NextUartHooks &h)
        : host(h), watchdog([this](std::stop_token stop) {
              while (!stop.stop_requested()) {
                  {
                      QMutexLocker lock(&evaluationMutex);
                      const auto end = deadline.load(std::memory_order_acquire);
                      if (end && steadyNanos() > end)
                          engine.setInterrupted(true);
                  }
                  std::this_thread::sleep_for(std::chrono::milliseconds(10));
              }
          })
    {
        for (unsigned ch = 0; ch < 2; ++ch) {
            api[ch] = std::make_unique<UartScriptApi>(*this, ch);
            QJSEngine::setObjectOwnership(api[ch].get(), QJSEngine::CppOwnership);
        }
    }
    ~NextUartRuntime()
    {
        watchdog.request_stop();
        watchdog.join();
    }
    QJSValue call(unsigned ch, const QJSValue &function, const QJSValueList &args = {},
                  bool method = true)
    {
        if (!function.isCallable())
            return {};
        {
            QMutexLocker lock(&evaluationMutex);
            engine.setInterrupted(false);
            deadline.store(steadyNanos() + 100'000'000, std::memory_order_release);
        }
        auto result = method ? function.callWithInstance(modules[ch], args) : function.call(args);
        {
            QMutexLocker lock(&evaluationMutex);
            deadline.store(0, std::memory_order_release);
        }
        if (result.isError()) {
            modules[ch] = {};
            methods[ch] = {};
            host.reportError(
                    QString(u"%1 UART hook: %2"_s).arg(ch ? u"Pi"_s : u"ESP"_s, result.toString()));
        }
        return result;
    }
    void closeAll()
    {
        for (unsigned ch = 0; ch < 2; ++ch)
            api[ch]->network(u"close"_s, -1);
    }
    void reset()
    {
        for (auto &channel : timers)
            channel.clear();
        closeAll();
        ++epoch;
        for (unsigned ch = 0; ch < 2; ++ch)
            call(ch, methods[ch].reset);
    }
    void reload()
    {
        for (auto &channel : timers)
            channel.clear();
        closeAll();
        ++epoch;
        {
            QMutexLocker lock(&host.m_mutex);
            configuration = host.m_configuration;
            revision = host.m_revision.load();
        }
        for (unsigned ch = 0; ch < 2; ++ch) {
            modules[ch] = {};
            methods[ch] = {};
            if (configuration.source[ch].isEmpty())
                continue;
            {
                QMutexLocker lock(&evaluationMutex);
                engine.setInterrupted(false);
                deadline.store(steadyNanos() + 100'000'000, std::memory_order_release);
            }
            const auto factory = engine.evaluate(u"(function(host){'use strict';\n"_s
                                                         + configuration.source[ch] + u"\n})"_s,
                                                 ch ? u"pi-uart.js"_s : u"esp-uart.js"_s);
            {
                QMutexLocker lock(&evaluationMutex);
                deadline.store(0, std::memory_order_release);
            }
            if (factory.isError()) {
                host.reportError(factory.toString());
                continue;
            }
            auto module = call(ch, factory, { engine.newQObject(api[ch].get()) }, false);
            bool valid = false;
            {
                QMutexLocker lock(&evaluationMutex);
                engine.setInterrupted(false);
                deadline.store(steadyNanos() + 100'000'000, std::memory_order_release);
            }
            if (module.isObject() && module.property(u"apiVersion"_s).toInt() == 1) {
                methods[ch] = { module.property(u"reset"_s), module.property(u"onByte"_s),
                                module.property(u"onNetwork"_s), module.property(u"onTimer"_s) };
                valid = !engine.hasError();
            }
            {
                QMutexLocker lock(&evaluationMutex);
                deadline.store(0, std::memory_order_release);
            }
            if (valid)
                modules[ch] = module;
            else {
                if (engine.hasError())
                    engine.catchError();
                methods[ch] = {};
                host.reportError(u"UART script must return an object with apiVersion: 1"_s);
            }
        }
        for (unsigned ch = 0; ch < 2; ++ch)
            call(ch, methods[ch].reset);
    }
    void transmit(zxkit::Emulator &em, zxkit::NextUart &device, unsigned ch, uint8_t byte)
    {
        // Debugger single steps can run on the GUI thread. Preserve the byte for
        // the next Engine poll instead of crossing QJSEngine's thread affinity.
        if (std::this_thread::get_id() != owner) {
            QMutexLocker lock(&deferredMutex);
            if (deferred.size() < 65536)
                deferred.append({ ch, byte });
            return;
        }
        emulator = &em;
        uart = &device;
        if (generation != device.generation()) {
            generation = device.generation();
            reset();
        }
        call(ch, methods[ch].onByte, { int(byte) });
    }
    void poll(zxkit::Emulator &em)
    {
        if (em.machine().id() != "next") {
            uart = nullptr;
            emulator = nullptr;
            return;
        }
        auto &next = static_cast<zxkit::ZxNextMachine &>(em.machine());
        emulator = &em;
        uart = &next.uart();
        if (!uart->hasTransmitHandler()) {
            generation = ~uint64_t(0);
            uart->setTransmitHandler([this, &em, device = uart](unsigned ch, uint8_t byte) {
                transmit(em, *device, ch, byte);
            });
            next.rtc().setTimeSource([] {
                const auto now = QDateTime::currentDateTime();
                return now.toSecsSinceEpoch() + now.offsetFromUtc();
            });
            QByteArray data;
            {
                QMutexLocker lock(&host.m_mutex);
                data = host.m_savedNvram;
            }
            if (data.size() == 56)
                next.rtc().setNvram(std::span<const uint8_t, 56>(
                        reinterpret_cast<const uint8_t *>(data.constData()), 56));
        }
        const bool reloaded = revision != host.m_revision.load(std::memory_order_acquire);
        if (reloaded)
            reload();
        if (generation != uart->generation()) {
            generation = uart->generation();
            if (!reloaded)
                reset();
        }
        if (host.m_eventsPending.exchange(false)) {
            QList<QVariantMap> events;
            {
                QMutexLocker lock(&host.m_mutex);
                events.swap(host.m_events);
            }
            for (auto event : events) {
                const auto token = event.take(u"epoch"_s).toULongLong();
                const auto ch = event.take(u"channel"_s).toUInt();
                const auto id = event.value(u"id"_s).toInt();
                if (ch >= 2 || id < 0 || id > 4 || token != socketEpoch[ch][id])
                    continue;
                if (ch < 2)
                    call(ch, methods[ch].onNetwork, { engine.toScriptValue(event) });
            }
        }
        QList<QPair<unsigned, uint8_t>> bytes;
        {
            QMutexLocker lock(&deferredMutex);
            bytes.swap(deferred);
        }
        for (const auto &[ch, byte] : bytes)
            transmit(em, *uart, ch, byte);
        if (!timers[0].isEmpty() || !timers[1].isEmpty()) {
            const auto now = steadyNanos();
            for (unsigned ch = 0; ch < 2; ++ch) {
                const auto ids = timers[ch].keys();
                for (int id : ids)
                    if (timers[ch].contains(id) && timers[ch].value(id) <= now) {
                        timers[ch].remove(id);
                        call(ch, methods[ch].onTimer, { id });
                    }
            }
        }
        // A byte comparison is cheap, and writes only happen when battery RAM changes.
        const auto ram = next.rtc().nvram();
        const QByteArray value(reinterpret_cast<const char *>(ram.data()), 56);
        if (value != nvram) {
            nvram = value;
            QMetaObject::invokeMethod(
                    &host,
                    [h = &host, value] {
                        {
                            QMutexLocker lock(&h->m_mutex);
                            h->m_savedNvram = value;
                        }
                        QSettings{}.setValue(u"Next/rtcNvram"_s, value);
                    },
                    Qt::QueuedConnection);
        }
    }
    QByteArray nvram;
};

int UartScriptApi::receive(const QVariantList &bytes)
{
    if (!m_runtime.uart || !m_runtime.emulator || bytes.size() > 65536)
        return 0;
    const auto data = byteArray(bytes);
    return int(m_runtime.uart->receive(
            m_channel,
            std::span(reinterpret_cast<const uint8_t *>(data.constData()), size_t(data.size())),
            uartTime(*m_runtime.emulator)));
}
void UartScriptApi::network(const QString &operation, int id, const QString &address, int port,
                            const QVariantList &bytes, bool udp)
{
    if (id < -1 || id > 4 || bytes.size() > 65536 || port < 0 || port > 65535)
        return;
    if (operation == u"open"_s || operation == u"close"_s) {
        if (id < 0)
            for (auto &token : m_runtime.socketEpoch[m_channel])
                token = ++m_runtime.epoch;
        else
            m_runtime.socketEpoch[m_channel][id] = ++m_runtime.epoch;
    }
    const auto token = id < 0 ? m_runtime.epoch : m_runtime.socketEpoch[m_channel][id];
    m_runtime.host.command({ { u"operation"_s, operation },
                             { u"id"_s, id },
                             { u"host"_s, address },
                             { u"port"_s, port },
                             { u"bytes"_s, byteArray(bytes) },
                             { u"udp"_s, udp },
                             { u"channel"_s, m_channel },
                             { u"epoch"_s, qulonglong(token) } });
}
void UartScriptApi::setTimer(int id, int milliseconds)
{
    if (id < 0 || id > 1023 || milliseconds < 0 || milliseconds > 60000
        || m_runtime.timers[m_channel].size() >= 64)
        return;
    m_runtime.timers[m_channel].insert(id, steadyNanos() + quint64(milliseconds) * 1'000'000);
}
void UartScriptApi::log(const QString &message)
{
    m_runtime.host.reportError(message.left(4096));
}
void UartScriptApi::cancelTimer(int id)
{
    m_runtime.timers[m_channel].remove(id);
}
QVariantMap UartScriptApi::configuration() const
{
    return { { u"host"_s, m_runtime.configuration.piHost },
             { u"port"_s, m_runtime.configuration.piPort } };
}

NextUartHooks::NextUartHooks(QObject *parent) : QObject(parent)
{
    QSettings settings;
    m_savedNvram = settings.value(u"Next/rtcNvram"_s).toByteArray();
    m_paths = { settings.value(u"Next/espScript"_s).toString(),
                settings.value(u"Next/piScript"_s).toString() };
    m_piHost = settings.value(u"Next/piHost"_s, u"127.0.0.1"_s).toString();
    m_piPort = settings.value(u"Next/piPort"_s, 0).toInt();
    configure();
}
NextUartHooks::~NextUartHooks()
{
    Q_ASSERT(!m_runtime);
}
zxkit::EngineConfig::ThreadHooks NextUartHooks::threadHooks()
{
    return { .started =
                     [this](zxkit::Emulator &) {
                         m_threadId =
                                 quint64(reinterpret_cast<quintptr>(QThread::currentThreadId()));
                     },
             .poll =
                     [this](zxkit::Emulator &em) {
                         if (em.machine().id() != "next" && !m_runtime)
                             return;
                         if (!m_runtime)
                             m_runtime = std::make_unique<NextUartRuntime>(*this);
                         m_runtime->poll(em);
                     },
             .stopped =
                     [this](zxkit::Emulator &em) {
                         if (em.machine().id() == "next")
                             static_cast<zxkit::ZxNextMachine &>(em.machine())
                                     .uart()
                                     .setTransmitHandler({});
                         if (m_runtime)
                             m_runtime->closeAll();
                         m_runtime.reset();
                     } };
}
void NextUartHooks::reportError(const QString &error)
{
    QMetaObject::invokeMethod(
            this,
            [this, error] {
                m_error = error;
                emit statusChanged();
            },
            Qt::QueuedConnection);
}
void NextUartHooks::configure()
{
    Configuration configuration;
    configuration.piHost = m_piHost;
    configuration.piPort = m_piPort;
    for (unsigned ch = 0; ch < 2; ++ch) {
        QFile file(m_paths[ch].isEmpty() ? (ch ? u":/uart/pi.js"_s : u":/uart/esp.js"_s)
                                         : m_paths[ch]);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) {
            reportError(tr("Cannot read UART script: %1").arg(file.fileName()));
            continue;
        }
        configuration.source[ch] = QString::fromUtf8(file.readAll());
    }
    {
        QMutexLocker lock(&m_mutex);
        m_configuration = std::move(configuration);
        m_revision.fetch_add(1, std::memory_order_release);
    }
    emit configurationChanged();
}
void NextUartHooks::setEspScript(const QString &path)
{
    m_paths[0] = path;
    QSettings{}.setValue(u"Next/espScript"_s, path);
    configure();
}
void NextUartHooks::setPiScript(const QString &path)
{
    m_paths[1] = path;
    QSettings{}.setValue(u"Next/piScript"_s, path);
    configure();
}
void NextUartHooks::setPiHost(const QString &value)
{
    m_piHost = value;
    QSettings{}.setValue(u"Next/piHost"_s, value);
    configure();
}
void NextUartHooks::setPiPort(int value)
{
    m_piPort = std::clamp(value, 0, 65535);
    QSettings{}.setValue(u"Next/piPort"_s, m_piPort);
    configure();
}
void NextUartHooks::reload()
{
    m_error.clear();
    emit statusChanged();
    configure();
}
void NextUartHooks::useBundled(int ch)
{
    if (ch == 0)
        setEspScript({});
    else if (ch == 1)
        setPiScript({});
}
void NextUartHooks::downloadScript(int ch, const QUrl &url)
{
    if (ch < 0 || ch > 1 || m_downloading
        || (url.scheme() != u"https"_s
            && !(url.scheme() == u"http"_s && url.host() == u"127.0.0.1"_s)))
        return;
    m_downloading = true;
    m_error.clear();
    emit statusChanged();
    auto *reply = m_downloads.get(QNetworkRequest(url));
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] {
        *bytes += reply->readAll();
        if (bytes->size() > 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, ch, reply, bytes] {
        *bytes += reply->readAll();
        reply->deleteLater();
        m_downloading = false;
        emit statusChanged();
        if (reply->error() != QNetworkReply::NoError || bytes->size() > 1024 * 1024) {
            reportError(tr("UART script download failed: %1").arg(reply->errorString()));
            return;
        }
        const auto folder =
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/uart"_s;
        QDir().mkpath(folder);
        const auto path =
                folder + (ch ? u"/pi-"_s : u"/esp-"_s)
                + QString::fromLatin1(
                        QCryptographicHash::hash(*bytes, QCryptographicHash::Sha256).toHex())
                + u".js"_s;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(*bytes) != bytes->size()
            || !file.commit()) {
            reportError(tr("Cannot save UART script"));
            return;
        }
        ch ? setPiScript(path) : setEspScript(path);
    });
}
void NextUartHooks::command(QVariantMap command)
{
    {
        QMutexLocker lock(&m_mutex);
        if (m_commands.size() >= 1024)
            return;
        m_commands.append(std::move(command));
    }
    if (!m_commandsPending.exchange(true))
        QMetaObject::invokeMethod(this, &NextUartHooks::processCommands, Qt::QueuedConnection);
}
void NextUartHooks::networkEvent(quint64 epoch, unsigned ch, QVariantMap event)
{
    event.insert(u"epoch"_s, epoch);
    event.insert(u"channel"_s, ch);
    {
        QMutexLocker lock(&m_mutex);
        if (m_events.size() >= 1024)
            return;
        m_events.append(std::move(event));
    }
    m_eventsPending = true;
}
void NextUartHooks::processCommands()
{
    QList<QVariantMap> commands;
    {
        QMutexLocker lock(&m_mutex);
        commands.swap(m_commands);
        m_commandsPending = false;
    }
    for (const auto &cmd : commands) {
        const auto op = cmd.value(u"operation"_s).toString();
        const int id = cmd.value(u"id"_s).toInt();
        const unsigned ch = cmd.value(u"channel"_s).toUInt();
        const auto epoch = cmd.value(u"epoch"_s).toULongLong();
        const int key = int(ch) * 16 + id;
        const auto event = [this, epoch, ch, id](QString type, QVariantMap values = {}) {
            values.insert(u"type"_s, type);
            values.insert(u"id"_s, id);
            networkEvent(epoch, ch, std::move(values));
        };
        if (op == u"close"_s) {
            for (int slot = 0; slot < 5; ++slot)
                if (id == -1 || id == slot) {
                    if (auto socket = m_sockets.take(int(ch) * 16 + slot)) {
                        socket->abort();
                        socket->deleteLater();
                    }
                }
        } else if (op == u"resolve"_s) {
            QHostInfo::lookupHost(cmd.value(u"host"_s).toString(), this,
                                  [event](const QHostInfo &info) {
                                      event(u"resolved"_s,
                                            { { u"address"_s,
                                                info.addresses().isEmpty()
                                                        ? QString{}
                                                        : info.addresses().first().toString() } });
                                  });
        } else if (op == u"open"_s && id >= 0) {
            if (auto old = m_sockets.take(key)) {
                old->abort();
                old->deleteLater();
            }
            QAbstractSocket *socket = cmd.value(u"udp"_s).toBool()
                    ? static_cast<QAbstractSocket *>(new QUdpSocket(this))
                    : new QTcpSocket(this);
            socket->setReadBufferSize(256 * 1024);
            m_sockets.insert(key, socket);
            connect(socket, &QAbstractSocket::connected, this, [event] { event(u"connected"_s); });
            connect(socket, &QAbstractSocket::disconnected, this, [event] { event(u"closed"_s); });
            connect(socket, &QAbstractSocket::errorOccurred, this,
                    [event, socket](QAbstractSocket::SocketError) {
                        event(u"error"_s, { { u"message"_s, socket->errorString() } });
                    });
            connect(socket, &QIODevice::readyRead, this, [event, socket] {
                if (auto *udp = qobject_cast<QUdpSocket *>(socket)) {
                    while (udp->hasPendingDatagrams()) {
                        QByteArray data(std::min<qint64>(udp->pendingDatagramSize(), 65536),
                                        Qt::Uninitialized);
                        udp->readDatagram(data.data(), data.size());
                        event(u"data"_s, { { u"bytes"_s, byteList(data) } });
                    }
                } else {
                    while (socket->bytesAvailable())
                        event(u"data"_s, { { u"bytes"_s, byteList(socket->read(65536)) } });
                }
            });
            socket->connectToHost(cmd.value(u"host"_s).toString(),
                                  quint16(cmd.value(u"port"_s).toUInt()));
        } else if (op == u"write"_s && id >= 0) {
            auto socket = m_sockets.value(key);
            const auto bytes = cmd.value(u"bytes"_s).toByteArray();
            if (!socket || socket->state() != QAbstractSocket::ConnectedState
                || socket->bytesToWrite() > 1024 * 1024 || socket->write(bytes) != bytes.size())
                event(u"error"_s);
            else
                event(u"sent"_s);
        }
    }
}
