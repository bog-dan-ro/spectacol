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
#include <QKeySequence>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

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
    const QCommandLineOption machineOption(u"machine"_s, u"Start with machine <id> (see --list-machines)."_s, u"id"_s);
    const QCommandLineOption listMachinesOption(u"list-machines"_s, u"Print the machine ids and exit."_s);
    const QCommandLineOption quitAfterOption(u"quit-after"_s, u"Quit after <seconds>."_s, u"seconds"_s);
    const QCommandLineOption screenshotOption(u"screenshot"_s, u"With --quit-after, save the emulated screen to <file.png> before quitting."_s, u"file"_s);
    const QCommandLineOption uiScreenshotOption(u"ui-screenshot"_s, u"With --quit-after, save the whole window (the UI) to <file.png> before quitting."_s, u"file"_s);
    const QCommandLineOption windowSizeOption(u"window-size"_s, u"The window's size, e.g. 1280x800 (for --ui-screenshot)."_s, u"WxH"_s);
    parser.addOption(machineOption);
    parser.addOption(listMachinesOption);
    parser.addOption(quitAfterOption);
    parser.addOption(screenshotOption);
    parser.addOption(uiScreenshotOption);
    const QCommandLineOption uiKeysOption(u"ui-keys"_s, u"Press keys in the window, one every 300 ms from 1 s on, e.g. F5,Down,Return (to screenshot a page); click:X:Y clicks at a window position, down:KEY and up:KEY only press or release a key (e.g. down:Control,0,up:Control), wait idles 300 ms."_s, u"keys"_s);
    parser.addOption(windowSizeOption);
    parser.addOption(uiKeysOption);
    parser.addPositionalArgument(u"file"_s, u"Snapshot, tape, disk or recording to load."_s, u"[file]"_s);
    parser.process(app);

    if (parser.isSet(listMachinesOption)) {
        for (const auto &id : ZxEmulator::instance().settings()->machineIds())
            qInfo().noquote() << id;
        return 0;
    }
    if (parser.isSet(machineOption) && !ZxEmulator::instance().settings()->machineIds().contains(parser.value(machineOption))) {
        qCritical().noquote() << "Unknown machine" << parser.value(machineOption) << "(see --list-machines)";
        return 1;
    }

    QQmlApplicationEngine engine;
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
    QDir appPath{QCoreApplication::applicationDirPath()};
    appPath.cdUp();
    QCoreApplication::addLibraryPath(appPath.absoluteFilePath(u"lib"_s));
    QCoreApplication::addLibraryPath(appPath.absoluteFilePath(u"plugins"_s));
    engine.addImportPath(appPath.absoluteFilePath(u"qml"_s));
#endif

    engine.addImageProvider(u"spectrum"_s, new SpectrumScreenImageProvider);
    engine.loadFromModule(u"Spectacol"_s, u"Main"_s);
    const auto window = [&engine]() -> QQuickWindow * {
        return engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    };
    if (parser.isSet(windowSizeOption)) {
        const QStringList size = parser.value(windowSizeOption).split(u'x');
        if (auto *w = window(); w && size.size() == 2)
            w->resize(size[0].toInt(), size[1].toInt());
    }

    // After the UI is up, so it can ask for e.g. the ZX Next SD card image
    if (parser.isSet(machineOption))
        ZxEmulator::instance().settings()->selectMachine(parser.value(machineOption));
    if (!parser.positionalArguments().isEmpty())
        ZxEmulator::instance().load(QFileInfo(parser.positionalArguments().first()).absoluteFilePath());
    if (parser.isSet(uiKeysOption)) {
        const QStringList keys = parser.value(uiKeysOption).split(u',', Qt::SkipEmptyParts);
        // Coarse timers can merge adjacent slots in long scripts and reorder
        // modifier transitions. Keep scripted input on the requested timeline.
        for (qsizetype i = 0; i < keys.size(); ++i) {
            // wait: an idle 300 ms slot
            if (keys[i].trimmed() == u"wait"_s)
                continue;
            if (keys[i].startsWith(u"click:"_s)) {
                const QStringList xy = keys[i].split(u':');
                const QPointF pos(xy.value(1).toDouble(), xy.value(2).toDouble());
                QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app, [pos, window] {
                    if (auto *w = window()) {
                        QMouseEvent press(QEvent::MouseButtonPress, pos, w->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QCoreApplication::sendEvent(w, &press);
                        QMouseEvent release(QEvent::MouseButtonRelease, pos, w->mapToGlobal(pos), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
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
                QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app, [key, down, window] {
                    if (auto *w = window()) {
                        QKeyEvent event(down ? QEvent::KeyPress : QEvent::KeyRelease, key, Qt::NoModifier);
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
            QTimer::singleShot(std::chrono::milliseconds(1000 + 300 * i), Qt::PreciseTimer, &app, [key, text, window] {
                if (auto *w = window()) {
                    QKeyEvent press(QEvent::KeyPress, key.key(), key.keyboardModifiers(), text);
                    QCoreApplication::sendEvent(w, &press);
                    // Keep it down across several emulated frames, so the ROM's keyboard scan
                    // sees it too (an immediate release only reaches the QML UI).
                    QTimer::singleShot(std::chrono::milliseconds(80), Qt::PreciseTimer, w, [key, text, window] {
                        if (auto *target = window()) {
                            QKeyEvent release(QEvent::KeyRelease, key.key(), key.keyboardModifiers(), text);
                            QCoreApplication::sendEvent(target, &release);
                        }
                    });
                }
            });
        }
    }
    if (parser.isSet(quitAfterOption)) {
        const QString screenshot = parser.isSet(screenshotOption) ? QFileInfo(parser.value(screenshotOption)).absoluteFilePath() : QString{};
        const QString uiScreenshot = parser.isSet(uiScreenshotOption) ? QFileInfo(parser.value(uiScreenshotOption)).absoluteFilePath() : QString{};
        QTimer::singleShot(std::chrono::milliseconds(qRound(parser.value(quitAfterOption).toDouble() * 1000)), &app, [screenshot, uiScreenshot, window] {
            // Before quitting: the emulation stops on aboutToQuit
            if (!screenshot.isEmpty())
                ZxEmulator::instance().saveMediaFile(screenshot);
            if (auto *w = window(); w && !uiScreenshot.isEmpty() && !w->grabWindow().save(uiScreenshot))
                qWarning().noquote() << "Can't save" << uiScreenshot;
            QCoreApplication::quit();
        });
    }
#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::hideSplashScreen();
#endif
    return app.exec();
}
