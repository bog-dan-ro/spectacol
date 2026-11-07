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

#include "zxemulator.h"
#include "zxsettings.h"
#include "spectrumscreen.h"

#include <QCommandLineParser>

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QKeySequence>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QTouchEvent>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>
#include <QMap>
#include <memory>
#include <algorithm>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardPaths>
#include <QTimer>
#ifdef Q_OS_ANDROID
#include "androidintegration.h"
#endif

using namespace Qt::StringLiterals;
int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);
    app.setOrganizationName(u"Licentia"_s);
    app.setOrganizationDomain(u"licentia.eu"_s);
    app.setApplicationName(u"Spectacol"_s);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Spectacol, a ZX Spectrum emulator"_s);
    parser.addHelpOption();
    const QCommandLineOption machineOption(
            u"machine"_s, u"Start with machine <id> (see --list-machines)."_s, u"id"_s);
    const QCommandLineOption listMachinesOption(u"list-machines"_s,
                                                u"Print the machine ids and exit."_s);
    const QCommandLineOption quitAfterOption(u"quit-after"_s, u"Quit after <seconds>."_s,
                                             u"seconds"_s);
    const QCommandLineOption screenshotOption(
            u"screenshot"_s,
            u"With --quit-after, save the emulated screen to <file.png> before quitting."_s,
            u"file"_s);
    const QCommandLineOption uiScreenshotOption(
            u"ui-screenshot"_s,
            u"With --quit-after, save the whole window (the UI) to <file.png> before quitting."_s,
            u"file"_s);
    const QCommandLineOption windowSizeOption(
            u"window-size"_s, u"The window's size, e.g. 1280x800 (for --ui-screenshot)."_s,
            u"WxH"_s);
    parser.addOption(machineOption);
    parser.addOption(listMachinesOption);
    parser.addOption(quitAfterOption);
    parser.addOption(screenshotOption);
    parser.addOption(uiScreenshotOption);
    const QCommandLineOption uiKeysOption(
            u"ui-keys"_s,
            u"Press keys in the window, one every 300 ms from 1 s on, e.g. F5,Down,Return (to screenshot a page); click:X:Y clicks at a window position, click:NAME clicks a named UI item, nav:ButtonA sends a gamepad navigation tap, input:TEXT commits text through the input method, down:KEY and up:KEY only press or release a key (e.g. down:Control,0,up:Control), wait idles 300 ms."_s,
            u"keys"_s);
    parser.addOption(windowSizeOption);
    parser.addOption(uiKeysOption);
    parser.addPositionalArgument(u"file"_s, u"Snapshot, tape, disk or recording to load."_s,
                                 u"[file]"_s);
    parser.process(app);

    if (parser.isSet(listMachinesOption)) {
        for (const auto &id : ZxEmulator::instance().settings()->machineIds())
            qInfo().noquote() << id;
        return 0;
    }
    if (parser.isSet(machineOption)
        && !ZxEmulator::instance().settings()->machineIds().contains(parser.value(machineOption))) {
        qCritical().noquote() << "Unknown machine" << parser.value(machineOption)
                              << "(see --list-machines)";
        return 1;
    }

    QQmlApplicationEngine engine;
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
    QDir appPath{ QCoreApplication::applicationDirPath() };
    appPath.cdUp();
    QCoreApplication::addLibraryPath(appPath.absoluteFilePath(u"lib"_s));
    QCoreApplication::addLibraryPath(appPath.absoluteFilePath(u"plugins"_s));
    engine.addImportPath(appPath.absoluteFilePath(u"qml"_s));
#endif

    engine.addImageProvider(u"spectrum"_s, new SpectrumScreenImageProvider);
    engine.loadFromModule(u"Spectacol"_s, u"Main"_s);
    const auto window = [&engine]() -> QQuickWindow * {
        return engine.rootObjects().isEmpty()
                ? nullptr
                : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    };
    if (parser.isSet(windowSizeOption)) {
        const QStringList size = parser.value(windowSizeOption).split(u'x');
        if (auto *w = window(); w && size.size() == 2)
            w->resize(size[0].toInt(), size[1].toInt());
    }
#ifdef Q_OS_ANDROID
    // Apply the saved orientation after the Qt window exists, so its surface
    // receives the same resize as the Android activity at startup.
    if (window()) {
        auto *settings = ZxEmulator::instance().settings();
        settings->setScreenOrientation(settings->screenOrientation());
    }
#endif

    // After the UI is up, so it can ask for e.g. the ZX Next SD card image
    if (parser.isSet(machineOption))
        ZxEmulator::instance().settings()->selectMachine(parser.value(machineOption));
    if (!parser.positionalArguments().isEmpty())
        ZxEmulator::instance().load(
                QFileInfo(parser.positionalArguments().first()).absoluteFilePath());
    // Keep the scripted device alive until the window and pending touches close.
    QPointingDevice touchDevice(
            u"Scripted touchscreen"_s, 0x53504543, QInputDevice::DeviceType::TouchScreen,
            QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10, 0);
    if (parser.isSet(uiKeysOption)) {
        const QStringList keys = parser.value(uiKeysOption).split(u',', Qt::SkipEmptyParts);
        if (std::any_of(keys.cbegin(), keys.cend(),
                        [](const QString &key) { return key.startsWith(u"point:"_s); }))
            QWindowSystemInterface::registerInputDevice(&touchDevice);
        const auto touchPoints = std::make_shared<QMap<int, QPointF>>();
        // Coarse timers can merge adjacent slots in long scripts and reorder
        // modifier transitions. Keep scripted input on the requested timeline.
        for (qsizetype i = 0; i < keys.size(); ++i) {
            // wait: an idle 300 ms slot
            if (keys[i].trimmed() == u"wait"_s)
                continue;
            if (keys[i].startsWith(u"point:"_s)) {
                // Real window touch events: point:ID:down/move/up:X:Y or point:cancel.
                const auto parts = keys[i].split(u':');
                bool idOk = false, xOk = false, yOk = false;
                const int id = parts.value(1).toInt(&idOk);
                const auto operation = parts.value(2);
                const QPointF pos(parts.value(3).toDouble(&xOk), parts.value(4).toDouble(&yOk));
                const bool cancel = parts.value(1) == u"cancel"_s;
                if (!cancel
                    && (!idOk || !xOk || !yOk
                        || (operation != u"down"_s && operation != u"move"_s
                            && operation != u"up"_s))) {
                    qWarning().noquote() << "Unknown touch event" << keys[i];
                    continue;
                }
                QTimer::singleShot(
                        std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app,
                        [id, operation, pos, cancel, window, &touchDevice, touchPoints] {
                            auto *w = window();
                            if (!w)
                                return;
                            if (cancel) {
                                QWindowSystemInterface::handleTouchCancelEvent<
                                        QWindowSystemInterface::SynchronousDelivery>(w,
                                                                                     &touchDevice);
                                touchPoints->clear();
                                return;
                            }
                            const bool down = operation == u"down"_s;
                            const bool up = operation == u"up"_s;
                            if ((!down && !touchPoints->contains(id))
                                || (down && touchPoints->contains(id)))
                                return;
                            touchPoints->insert(id, pos);
                            QList<QWindowSystemInterface::TouchPoint> points;
                            for (auto it = touchPoints->cbegin(); it != touchPoints->cend(); ++it) {
                                const auto state = it.key() != id ? QEventPoint::Stationary
                                        : down                    ? QEventPoint::Pressed
                                        : up                      ? QEventPoint::Released
                                                                  : QEventPoint::Updated;
                                QWindowSystemInterface::TouchPoint point;
                                point.id = it.key();
                                point.state = state;
                                point.area = QRectF(
                                        QHighDpi::toNativePixels(w->mapToGlobal(it.value()), w),
                                        QSizeF(1, 1));
                                point.pressure = state == QEventPoint::Released ? 0 : 1;
                                points.append(point);
                            }
                            if (up)
                                touchPoints->remove(id);
                            QWindowSystemInterface::handleTouchEvent<
                                    QWindowSystemInterface::SynchronousDelivery>(w, &touchDevice,
                                                                                 points);
                        });
                continue;
            }
            if (keys[i].startsWith(u"input:"_s)) {
                const auto text = keys[i].mid(6);
                QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer,
                                   &app, [text, window] {
                                       if (auto *w = window()) {
                                           QInputMethodEvent event;
                                           event.setCommitString(text);
                                           QCoreApplication::sendEvent(w, &event);
                                       }
                                   });
                continue;
            }
            if (keys[i].startsWith(u"nav:"_s)) {
                const auto parts = keys[i].split(u':');
                const auto name = parts.value(1).toLatin1();
                const auto &meta = QGamepadManager::staticMetaObject;
                bool axisOk = false, buttonOk = false, valueOk = false;
                const int axis = meta.enumerator(meta.indexOfEnumerator("GamepadAxis"))
                                         .keyToValue(name.constData(), &axisOk);
                const int button = meta.enumerator(meta.indexOfEnumerator("GamepadButton"))
                                           .keyToValue(name.constData(), &buttonOk);
                const double value = parts.value(2).toDouble(&valueOk);
                if (!buttonOk && !(axisOk && valueOk)) {
                    qWarning() << "Unknown navigation event" << keys[i];
                    continue;
                }
                QTimer::singleShot(
                        std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app,
                        [buttonOk, button, axis, value] {
                            auto *manager = QGamepadManager::instance();
                            const int device = ZxEmulator::instance().gamepadId();
                            if (buttonOk) {
                                manager->gamepadButtonPressEvent(
                                        device, static_cast<QGamepadManager::GamepadButton>(button),
                                        1);
                                QTimer::singleShot(80, manager, [manager, device, button] {
                                    manager->gamepadButtonReleaseEvent(
                                            device,
                                            static_cast<QGamepadManager::GamepadButton>(button));
                                });
                            } else
                                manager->gamepadAxisEvent(
                                        device, static_cast<QGamepadManager::GamepadAxis>(axis),
                                        value);
                        });
                continue;
            }
            if (keys[i].startsWith(u"pad:"_s) || keys[i].startsWith(u"touch:"_s)) {
                // Exercise controller input without requiring a hardware gamepad.
                const auto parts = keys[i].split(u':');
                const bool touch = parts[0] == u"touch"_s;
                const auto name = parts.value(1).toLatin1();
                if (name == "clear") {
                    QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer,
                                       &app, [touch] {
                                           if (touch)
                                               ZxEmulator::instance().releaseTouchJoystick();
                                           else
                                               ZxEmulator::instance().releaseInputs();
                                       });
                    continue;
                }
                bool axisOk = false, buttonOk = false, numberOk = false;
                const auto &meta = QGamepadManager::staticMetaObject;
                const int axis = meta.enumerator(meta.indexOfEnumerator("GamepadAxis"))
                                         .keyToValue(name.constData(), &axisOk);
                const int button = meta.enumerator(meta.indexOfEnumerator("GamepadButton"))
                                           .keyToValue(name.constData(), &buttonOk);
                const double value = parts.value(2).toDouble(&numberOk);
                const auto operation = parts.value(2);
                if ((axisOk && numberOk)
                    || (buttonOk
                        && (operation == u"down"_s || operation == u"up"_s
                            || operation == u"cancel"_s))) {
                    QTimer::singleShot(
                            std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app,
                            [axisOk, axis, button, value, operation, touch] {
                                auto &emulator = ZxEmulator::instance();
                                if (axisOk)
                                    emulator.gamepadAxisEvent(
                                            static_cast<QGamepadManager::GamepadAxis>(axis), value,
                                            touch);
                                else if (operation == u"down"_s)
                                    emulator.gamepadButtonPressEvent(
                                            static_cast<QGamepadManager::GamepadButton>(button),
                                            touch);
                                else
                                    emulator.gamepadButtonReleaseEvent(
                                            static_cast<QGamepadManager::GamepadButton>(button),
                                            touch, operation == u"cancel"_s);
                            });
                } else {
                    qWarning().noquote() << "Unknown controller event" << keys[i];
                }
                continue;
            }
            if (keys[i].startsWith(u"click:"_s)) {
                const QStringList xy = keys[i].split(u':');
                const QPointF position(xy.value(1).toDouble(), xy.value(2).toDouble());
                const QString itemName = xy.size() == 2 ? xy[1] : QString{};
                QTimer::singleShot(
                        std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app,
                        [position, itemName, window] {
                            if (auto *w = window()) {
                                QPointF pos = position;
                                if (!itemName.isEmpty()) {
                                    const auto *item = w->findChild<QQuickItem *>(itemName);
                                    if (!item || !item->isVisible() || !item->isEnabled()) {
                                        qWarning() << "Unavailable UI item" << itemName;
                                        return;
                                    }
                                    pos = item->mapToScene(
                                            QPointF(item->width() / 2, item->height() / 2));
                                }
                                QMouseEvent press(QEvent::MouseButtonPress, pos,
                                                  w->mapToGlobal(pos), Qt::LeftButton,
                                                  Qt::LeftButton, Qt::NoModifier);
                                QCoreApplication::sendEvent(w, &press);
                                QMouseEvent release(QEvent::MouseButtonRelease, pos,
                                                    w->mapToGlobal(pos), Qt::LeftButton,
                                                    Qt::NoButton, Qt::NoModifier);
                                QCoreApplication::sendEvent(w, &release);
                            }
                        });
                continue;
            }
            if (keys[i].startsWith(u"down:"_s) || keys[i].startsWith(u"up:"_s)) {
                // down:KEY / up:KEY: only press or release a key (a Qt::Key name without "Key_"), to
                // hold a modifier around other keys, e.g. down:Control,0,up:Control
                const bool down = keys[i].startsWith(u"down:"_s);
                const QByteArray name = "Key_" + keys[i].section(u':', 1).trimmed().toLatin1();
                bool ok = false;
                const int key = QMetaEnum::fromType<Qt::Key>().keyToValue(name.constData(), &ok);
                if (!ok) {
                    qWarning().noquote() << "Unknown key" << keys[i];
                    continue;
                }
                QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer,
                                   &app, [key, down, window] {
                                       if (auto *w = window()) {
                                           QKeyEvent event(down ? QEvent::KeyPress
                                                                : QEvent::KeyRelease,
                                                           key, Qt::NoModifier);
                                           QCoreApplication::sendEvent(w, &event);
                                       }
                                   });
                continue;
            }
            const QKeySequence sequence = QKeySequence::fromString(keys[i].trimmed());
            if (sequence.isEmpty()) {
                qWarning().noquote() << "Unknown key" << keys[i];
                continue;
            }
            const QKeyCombination key = sequence[0];
            // A single character is also typed (text fields)
            const QString text = keys[i].size() == 1 ? keys[i] : QString{};
            QTimer::singleShot(
                    std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app,
                    [key, text, window] {
                        if (auto *w = window()) {
                            QKeyEvent press(QEvent::KeyPress, key.key(), key.keyboardModifiers(),
                                            text);
                            QCoreApplication::sendEvent(w, &press);
                            // Keep it down across several emulated frames, so the ROM's keyboard scan
                            // sees it too (an immediate release only reaches the QML UI).
                            QTimer::singleShot(
                                    std::chrono::milliseconds(80), Qt::PreciseTimer, w,
                                    [key, text, window] {
                                        if (auto *target = window()) {
                                            QKeyEvent release(QEvent::KeyRelease, key.key(),
                                                              key.keyboardModifiers(), text);
                                            QCoreApplication::sendEvent(target, &release);
                                        }
                                    });
                        }
                    });
        }
    }
    if (parser.isSet(quitAfterOption)) {
        const QString screenshot = parser.isSet(screenshotOption)
                ? QFileInfo(parser.value(screenshotOption)).absoluteFilePath()
                : QString{};
        const QString uiScreenshot = parser.isSet(uiScreenshotOption)
                ? QFileInfo(parser.value(uiScreenshotOption)).absoluteFilePath()
                : QString{};
        QTimer::singleShot(
                std::chrono::milliseconds(qRound(parser.value(quitAfterOption).toDouble() * 1000)),
                &app, [screenshot, uiScreenshot, window] {
                    // Before quitting: the emulation stops on aboutToQuit
                    if (!screenshot.isEmpty())
                        ZxEmulator::instance().saveMediaFile(screenshot);
                    if (auto *w = window();
                        w && !uiScreenshot.isEmpty() && !w->grabWindow().save(uiScreenshot))
                        qWarning().noquote() << "Can't save" << uiScreenshot;
                    QCoreApplication::quit();
                });
    }
#ifdef Q_OS_ANDROID
    AndroidIntegration::install(&app);
    QNativeInterface::QAndroidApplication::hideSplashScreen();
#endif
    return app.exec();
}
