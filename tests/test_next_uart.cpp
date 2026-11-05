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
#include <QFile>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUdpSocket>
#include <deque>
#include <mutex>
#include <zxkit/machines/ZxNextMachine.h>

using namespace Qt::StringLiterals;
class NextUartTest : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    std::unique_ptr<NextUartHooks> host;
    std::unique_ptr<zxkit::Engine> engine;
    std::mutex mutex;
    std::deque<std::pair<unsigned, uint8_t>> tx;
    std::array<QByteArray, 2> rx;
    void send(unsigned ch, const QByteArray &bytes)
    {
        std::lock_guard lock(mutex);
        rx[ch].clear();
        for (unsigned char byte : bytes)
            tx.emplace_back(ch, byte);
    }
    QByteArray received(unsigned ch = 0)
    {
        std::lock_guard lock(mutex);
        return rx[ch];
    }
    QString script(const QByteArray &source)
    {
        const auto path = directory.filePath(u"external.js"_s);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        file.write(source);
        return path;
    }
  private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName(u"SpectacolTests"_s);
        QCoreApplication::setApplicationName(u"NextUart"_s);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        qputenv("XDG_DATA_HOME", directory.path().toUtf8());
    }
    void init()
    {
        QSettings{}.clear();
        tx.clear();
        rx = {};
        host = std::make_unique<NextUartHooks>();
        auto em = zxkit::makeShared<zxkit::Emulator>(zxkit::EmulatorConfig{.machineId = "next"});
        {
            auto lock = em.writeLock();
            lock->cpu().registers().PC = 0x8000;
            const std::array<uint8_t, 4> loop{0xF3, 0xC3, 1, 0x80};
            for (unsigned i = 0; i < loop.size(); ++i)
                lock->mmu().write(0x8000 + i, loop[i]);
        }
        zxkit::EngineConfig config;
        config.threadHooks = host->threadHooks();
        const auto poll = config.threadHooks.poll;
        config.threadHooks.poll = [this, poll](zxkit::Emulator &emulator) {
            poll(emulator);
            auto &next = static_cast<zxkit::ZxNextMachine &>(emulator.machine());
            const auto &clock = emulator.clock();
            const auto now =
                clock.frameCount() * uint64_t(clock.timings().tstatesPerFrame()) + clock.frameBaseTstates();
            auto &uart = next.uart();
            std::lock_guard lock(mutex);
            for (unsigned ch = 0; ch < 2; ++ch) {
                uart.write(0x153B, ch ? 0x40 : 0, now);
                for (unsigned i = 0; i < 1024 && (uart.read(0x133B, now) & 1); ++i)
                    rx[ch].append(char(uart.read(0x143B, now)));
            }
            for (unsigned i = 0; i < 32 && !tx.empty(); ++i) {
                const auto [ch, byte] = tx.front();
                uart.write(0x153B, ch ? 0x40 : 0, now);
                if (uart.read(0x133B, now) & 2)
                    break;
                uart.write(0x133B, byte, now);
                tx.pop_front();
            }
        };
        engine = std::make_unique<zxkit::Engine>(em, std::move(config));
        QTRY_VERIFY_WITH_TIMEOUT(received().contains("ready"), 3000);
        QVERIFY(host->runtimeThreadId() != quint64(reinterpret_cast<quintptr>(QThread::currentThreadId())));
    }
    void cleanup()
    {
        engine->stop();
        engine.reset();
        host.reset();
        QCoreApplication::processEvents();
    }
    void atAndReset()
    {
        send(0, "ATE0\r\n");
        QTRY_VERIFY(received().contains("OK"));
        send(0, "AT+GMR\r\n");
        QTRY_VERIFY(received().contains("Spectacol UART API 1"));
        QVERIFY(host->errorString().isEmpty());
        send(0, "AT+UNKNOWN\r\n");
        QTRY_VERIFY(received().contains("ERROR"));
        send(0, "AT+RST\r\n");
        QTRY_VERIFY(received().contains("ready"));
    }
    void wifiSetup()
    {
        // Both Wi-Fi wizards shipped with NextZXOS query station IP and DNS.
        for (const QByteArray &command : {QByteArray("ATE0"), QByteArray("AT+CWMODE=1"), QByteArray("AT+GMR"),
                                         QByteArray("AT+CWJAP?"), QByteArray("AT+CIPSTA?"),
                                         QByteArray("AT+CIPDNS_CUR?"), QByteArray("AT+CIPDNS?")}) {
            send(0, command + "\r\n");
            QTRY_VERIFY(received().endsWith("\r\nOK\r\n"));
            QVERIFY(!received().contains("ERROR"));
        }
        QVERIFY(received().count("+CIPDNS:") == 2);
        send(0, "AT+CIPSTA?\r\n");
        QTRY_VERIFY(received().endsWith("\r\nOK\r\n"));
        QVERIFY(received().contains("+CIPSTA:ip:\"127.0.0.1\""));
        QVERIFY(received().contains("+CIPSTA:gateway:"));
        QVERIFY(received().contains("+CIPSTA:netmask:"));

        for (const QByteArray &command : {QByteArray("AT+CWDHCP=0,1"), QByteArray("AT+CIPSTA=\"192.168.1.42\""),
                                         QByteArray("AT+CIPDNS_DEF=1,1.1.1.1,9.9.9.9")}) {
            send(0, command + "\r\n");
            QTRY_VERIFY(received().endsWith("\r\nOK\r\n"));
        }
        send(0, "AT+CIPSTA?\r\n");
        QTRY_VERIFY(received().contains("+CIPSTA:ip:\"192.168.1.42\""));
        send(0, "AT+CIPDNS_CUR?\r\n");
        QTRY_VERIFY(received().endsWith("\r\nOK\r\n"));
        QVERIFY(received().contains("+CIPDNS_CUR:1.1.1.1"));
        QVERIFY(received().contains("+CIPDNS_CUR:9.9.9.9"));
        send(0, "AT+CIPSTA=\"999.1.1.1\"\r\n");
        QTRY_VERIFY(received().contains("ERROR"));
        send(0, "AT+CIPSTA?\r\n");
        QTRY_VERIFY(received().contains("192.168.1.42"));
        for (const QByteArray &command : {QByteArray("AT+CWDHCP=1,1"), QByteArray("AT+CIPDNS_DEF=0")}) {
            send(0, command + "\r\n");
            QTRY_VERIFY(received().endsWith("\r\nOK\r\n"));
        }
        send(0, "AT+CIPSTA?\r\n");
        QTRY_VERIFY(received().contains("+CIPSTA:ip:\"127.0.0.1\""));
        send(0, "AT+CWQAP\r\n");
        QTRY_VERIFY(received().contains("OK"));
        send(0, "AT+CIPSTA?\r\n");
        QTRY_VERIFY(received().contains("+CIPSTA:ip:\"0.0.0.0\""));
        QVERIFY(host->errorString().isEmpty());
    }
    void tcpBinary()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket] { socket->write(socket->readAll()); });
        });
        send(0, "ATE0\r\n");
        QTRY_VERIFY(received().contains("OK"));
        send(0, "AT+CIPSTART=\"TCP\",\"127.0.0.1\"," + QByteArray::number(server.serverPort()) + "\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(received().contains("CONNECT"), 5000);
        send(0, "AT+CIPSEND=3\r\n");
        QTRY_VERIFY(received().contains("> "));
        send(0, QByteArray::fromHex("00ff58"));
        QTRY_VERIFY_WITH_TIMEOUT(received().contains(QByteArray("+IPD,3:") + QByteArray::fromHex("00ff58")), 5000);
        QVERIFY(received().contains("SEND OK"));
        QVERIFY(host->errorString().isEmpty());
        send(0, "AT+CIPCLOSE\r\n");
        QTRY_VERIFY(received().contains("CLOSED"));
    }
    void piBridge()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket] { socket->write(socket->readAll()); });
        });
        host->setPiPort(server.serverPort());
        QTest::qWait(100);
        send(1, QByteArray::fromHex("0001ff"));
        QTRY_COMPARE_WITH_TIMEOUT(received(1), QByteArray::fromHex("0001ff"), 5000);
        QVERIFY(host->errorString().isEmpty());
    }
    void udpBinary()
    {
        QUdpSocket server;
        QVERIFY(server.bind(QHostAddress::LocalHost, 0));
        connect(&server, &QUdpSocket::readyRead, this, [&] {
            while (server.hasPendingDatagrams()) {
                QByteArray bytes(server.pendingDatagramSize(), Qt::Uninitialized);
                QHostAddress address;
                quint16 port = 0;
                server.readDatagram(bytes.data(), bytes.size(), &address, &port);
                server.writeDatagram(bytes, address, port);
            }
        });
        send(0, "ATE0\r\n");
        QTRY_VERIFY(received().contains("OK"));
        send(0, "AT+CIPSTART=\"UDP\",\"127.0.0.1\"," + QByteArray::number(server.localPort()) + "\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(received().contains("CONNECT"), 5000);
        send(0, "AT+CIPSEND=3\r\n");
        QTRY_VERIFY(received().contains("> "));
        send(0, QByteArray::fromHex("00ff58"));
        QTRY_VERIFY_WITH_TIMEOUT(received().contains(QByteArray("+IPD,3:") + QByteArray::fromHex("00ff58")), 5000);
        QVERIFY(host->errorString().isEmpty());
    }
    void moduleTimersAndThis()
    {
        const auto path = script(
            "return {apiVersion:1,value:0,onByte:function(b){this.value=b;host.setTimer(3,0);},"
            "onTimer:function(id){if(id===3)host.receive([this.value]);}};");
        host->setEspScript(path);
        QTest::qWait(100);
        send(0, QByteArray::fromHex("ff"));
        QTRY_COMPARE(received(), QByteArray::fromHex("ff"));
        QVERIFY(host->errorString().isEmpty());
    }
    void externalReloadAndWatchdog()
    {
        auto path = script("return {apiVersion:1,reset:function(){},onByte:function(b){host.receive([b^255]);}};");
        QVERIFY(!path.isEmpty());
        host->setEspScript(path);
        QTest::qWait(100);
        send(0, QByteArray::fromHex("00ff"));
        QTRY_COMPARE(received(), QByteArray::fromHex("ff00"));
        script("return {apiVersion:1,onByte:function(){while(true){}}};");
        host->reload();
        QTest::qWait(100);
        send(0, "X");
        QTRY_VERIFY_WITH_TIMEOUT(!host->errorString().isEmpty(), 3000);
        host->useBundled(0);
        host->reload();
        QTRY_VERIFY_WITH_TIMEOUT(received().contains("ready"), 3000);
        send(0, "AT\r\n");
        QTRY_VERIFY(received().contains("OK"));
    }
    void downloadedHook()
    {
        const QByteArray body = "return {apiVersion:1,reset:function(){},onByte:function(b){host.receive([b]);}};";
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, body] {
                socket->readAll();
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) +
                              "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
        host->downloadScript(0, QUrl(u"http://127.0.0.1:%1/uart.js"_s.arg(server.serverPort())));
        QTRY_VERIFY_WITH_TIMEOUT(!host->downloading(), 3000);
        QVERIFY(!host->espScript().isEmpty());
        QTest::qWait(100);
        send(0, QByteArray::fromHex("00ff41"));
        QTRY_COMPARE(received(), QByteArray::fromHex("00ff41"));
    }
};
QTEST_GUILESS_MAIN(NextUartTest)
#include "test_next_uart.moc"
