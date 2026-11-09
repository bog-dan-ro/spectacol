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

#include "desktopinput.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGamepadManager>
#include <QGuiApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <qpa/qwindowsysteminterface.h>
#include <memory>

using namespace Qt::StringLiterals;

class DesktopInputTest : public QObject
{
    Q_OBJECT

    static void write(const QString &path, const QByteArray &data)
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size());
    }

    static void addKeyboard(const QString &directory, const QByteArray &bus = "0003",
                            const QByteArray &vendor = "046d")
    {
        const QString device = directory + u"/event0/device/"_s;
        write(device + u"id/bustype"_s, bus);
        write(device + u"id/vendor"_s, vendor);
        // Q, A, Z, Enter and Space, in the Linux input capability bitmap.
        const quint64 keys = (quint64(1) << 16) | (quint64(1) << 30) | (quint64(1) << 44)
                | (quint64(1) << 28) | (quint64(1) << 57);
        QByteArray bitmap = QByteArray::number(keys, 16);
        if constexpr (sizeof(unsigned long) == 4)
            bitmap = QByteArray::number(keys >> 32, 16) + ' '
                    + QByteArray::number(keys & 0xffffffff, 16);
        write(device + u"capabilities/key"_s, bitmap);
    }

    static void tap(QGamepadManager::GamepadButton button)
    {
        auto *manager = QGamepadManager::instance();
        manager->gamepadButtonPressEvent(4242, button, 1.0);
        manager->gamepadButtonReleaseEvent(4242, button);
        QCoreApplication::processEvents();
    }

private slots:
    void linuxKeyboardDetection()
    {
#ifndef Q_OS_LINUX
        QSKIP("Linux sysfs classification");
#else
        QTemporaryDir inputs;
        QVERIFY(inputs.isValid());
        QVERIFY(!DesktopInput::hasPhysicalKeyboard(inputs.path()));
        addKeyboard(inputs.path(), "0006"); // Steam Input's virtual keyboard.
        QVERIFY(!DesktopInput::hasPhysicalKeyboard(inputs.path()));
        addKeyboard(inputs.path(), "0003", "28de"); // Deck controller's keyboard interface.
        QVERIFY(!DesktopInput::hasPhysicalKeyboard(inputs.path()));
        addKeyboard(inputs.path());
        QVERIFY(DesktopInput::hasPhysicalKeyboard(inputs.path()));
        addKeyboard(inputs.path(), "0005"); // Bluetooth keyboards also count.
        QVERIFY(DesktopInput::hasPhysicalKeyboard(inputs.path()));
        write(inputs.path() + u"/event0/device/capabilities/key"_s, "10000000"); // Enter only.
        QVERIFY(!DesktopInput::hasPhysicalKeyboard(inputs.path()));
        write(inputs.path() + u"/event0/device/capabilities/key"_s, "invalid");
        QVERIFY(!DesktopInput::hasPhysicalKeyboard(inputs.path()));
#endif
    }

    void textCommitDismissalAndHotplug()
    {
#ifndef Q_OS_LINUX
        QSKIP("Uses simulated Linux keyboard attachment");
#else
        QTemporaryDir inputs;
        DesktopInput policy(inputs.path());
        QVERIFY(DesktopInput::supported());
        QVERIFY(!policy.physicalKeyboardPresent());
        QQmlEngine engine;
        engine.addImportPath(QCoreApplication::applicationDirPath() + u"/../qml"_s);
        QQmlComponent component(&engine);
        component.loadFromModule(u"DesktopInputTest"_s, u"DesktopInputWindow"_s);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> root(component.create());
        QVERIFY(root);
        auto *window = qobject_cast<QQuickWindow *>(root.get());
        QVERIFY(window);
        auto *editor = root->findChild<QQuickItem *>(u"editor"_s);
        auto *otherControl = root->findChild<QQuickItem *>(u"otherControl"_s);
        QVERIFY(editor && otherControl);
        window->requestActivate();
        // A headless Wayland compositor has no seat to activate the window.
        if (QGuiApplication::platformName() == u"wayland"_s)
            QWindowSystemInterface::handleFocusWindowChanged(window);
        QTRY_COMPARE(qGuiApp->focusWindow(), window);
        editor->forceActiveFocus();
        QTRY_VERIFY(policy.isVisible());

        QSignalSpy navigation(&policy, &DesktopInput::navigationKey);
        tap(QGamepadManager::ButtonRight);
        tap(QGamepadManager::ButtonA);
        QCOMPARE(navigation.size(), 4);
        QCOMPARE(navigation[0][0].toInt(), Qt::Key_Right);
        QCOMPARE(navigation[2][0].toInt(), Qt::Key_Return);
        policy.commitText(u"Dizzy"_s);
        QCOMPARE(editor->property("text").toString(), u"Dizzy"_s);
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, 5)));
        policy.commitText(u"Jet Set Willy \u00a3"_s);
        QCOMPARE(editor->property("text").toString(), u"Jet Set Willy \u00a3"_s);
        policy.sendKey(Qt::Key_Backspace);
        QCOMPARE(editor->property("text").toString(), u"Jet Set Willy "_s);
        QVERIFY(editor->hasActiveFocus());

        tap(QGamepadManager::ButtonB);
        QTRY_VERIFY(!policy.isVisible());
        QCOMPARE(root->property("escapeCount").toInt(), 0);
        tap(QGamepadManager::ButtonB);
        QCOMPARE(root->property("escapeCount").toInt(), 1);
        tap(QGamepadManager::ButtonA);
        QTRY_VERIFY(policy.isVisible());

        QSignalSpy changes(&policy, &DesktopInput::devicesChanged);
        addKeyboard(inputs.path());
        policy.refreshDevices();
        QVERIFY(policy.physicalKeyboardPresent());
        QTRY_VERIFY(!policy.isVisible());

        auto *manager = QGamepadManager::instance();
        // Simulate backend notifications so the manager updates its device list.
        QVERIFY(QMetaObject::invokeMethod(manager, "_q_forwardGamepadConnected", Q_ARG(int, 4242)));
        QTRY_VERIFY(policy.isVisible());
        QVERIFY(policy.physicalKeyboardPresent());
        QTest::keyClick(window, 'x');
        QCOMPARE(editor->property("text").toString(), u"Jet Set Willy x"_s);
        QKeyEvent enterPress(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, 36, 0, 0);
        QKeyEvent enterRelease(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier, 36, 0, 0);
        QCoreApplication::sendEvent(window, &enterPress);
        QCoreApplication::sendEvent(window, &enterRelease);
        QCOMPARE(root->property("acceptedCount").toInt(), 1);

        tap(QGamepadManager::ButtonB);
        QTRY_VERIFY(!policy.isVisible());
        tap(QGamepadManager::ButtonA);
        QTRY_VERIFY(policy.isVisible());

        QVERIFY(QMetaObject::invokeMethod(manager, "_q_forwardGamepadConnected", Q_ARG(int, 4243)));
        QVERIFY(QMetaObject::invokeMethod(manager, "_q_forwardGamepadDisconnected",
                                          Q_ARG(int, 4242)));
        QCoreApplication::processEvents();
        QVERIFY(policy.isVisible());
        QVERIFY(QMetaObject::invokeMethod(manager, "_q_forwardGamepadDisconnected",
                                          Q_ARG(int, 4243)));
        QTRY_VERIFY(!policy.isVisible());

        QVERIFY(QDir(inputs.path() + u"/event0"_s).removeRecursively());
        policy.refreshDevices();
        QVERIFY(!policy.physicalKeyboardPresent());
        QTRY_VERIFY(policy.isVisible());
        QCOMPARE(changes.size(), 2);

        otherControl->forceActiveFocus();
        QTRY_VERIFY(!policy.isVisible());
        editor->setProperty("readOnly", true);
        editor->forceActiveFocus();
        QCoreApplication::processEvents();
        QVERIFY(!policy.isVisible());
#endif
    }
};

QTEST_MAIN(DesktopInputTest)
#include "test_desktop_input.moc"
