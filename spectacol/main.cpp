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

#include "zxemulator.h"
#include "zxsettings.h"
#include "spectrumscreen.h"

#include <QCommandLineParser>

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
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
    parser.addOption(machineOption);
    parser.addOption(listMachinesOption);
    parser.addOption(quitAfterOption);
    parser.addOption(screenshotOption);
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

    // After the UI is up, so it can ask for e.g. the ZX Next SD card image
    if (parser.isSet(machineOption))
        ZxEmulator::instance().settings()->selectMachine(parser.value(machineOption));
    if (!parser.positionalArguments().isEmpty())
        ZxEmulator::instance().load(QFileInfo(parser.positionalArguments().first()).absoluteFilePath());
    if (parser.isSet(quitAfterOption)) {
        const QString screenshot = parser.isSet(screenshotOption) ? QFileInfo(parser.value(screenshotOption)).absoluteFilePath() : QString{};
        QTimer::singleShot(std::chrono::milliseconds(qRound(parser.value(quitAfterOption).toDouble() * 1000)), &app, [screenshot] {
            // Before quitting: the emulation stops on aboutToQuit
            if (!screenshot.isEmpty())
                ZxEmulator::instance().saveMediaFile(screenshot);
            QCoreApplication::quit();
        });
    }
#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::hideSplashScreen();
#endif
    return app.exec();
}
