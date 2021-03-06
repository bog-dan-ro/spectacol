/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

#include "folderlistmodel.h"
#include "fuseemulator.h"
#include "fuserecording.h"
#include "fusescreen.h"
#include "fusetapedata.h"
#include "pokemodel.h"
#include "spectrumscreen.h"
#include "z80assembler.h"

#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QDir>
#include <QStandardPaths>
#ifdef Q_OS_ANDROID
# include <QtAndroid>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Licentia");
    app.setOrganizationDomain("licentia.eu");
    app.setApplicationName("Spectacol");

    QString newSettings = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/.fuserc";
#ifdef Q_OS_ANDROID
    QString oldSettings = QDir::homePath() + "/.fuserc";
#else
    QString oldSettings = QDir::homePath() + "/.spectacolrc";
#endif
    QDir::home().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!QFile::exists(newSettings) && QFile::exists(oldSettings))
        QFile::rename(oldSettings, newSettings);

    qmlRegisterType<FuseScreen>("Fuse", 1, 0, "FuseScreen");
    qmlRegisterType<Z80Assembler>("Fuse", 1, 0, "Z80Assembler");
    qmlRegisterType<FolderListModel>("Fuse", 1, 0, "FolderListModel");
    qmlRegisterType<PokeModel>("Fuse", 1, 0, "PokeModel");
    qmlRegisterType<FuseTapeData>("Fuse", 1, 0, "FuseTapeData");

    qmlRegisterUncreatableType<BreakpointsModel>("Fuse", 1, 0, "BreakpointsModel", QLatin1String("use breakpointsModel context property instead"));
    qmlRegisterUncreatableType<FuseEmulator>("Fuse", 1, 0, "FuseEmulator", QLatin1String("use fuse instead"));
    qmlRegisterUncreatableType<FuseTape>("Fuse", 1, 0, "FuseTape", QLatin1String("use fuse.tape instead"));
    qmlRegisterUncreatableType<FuseSettings>("Fuse", 1, 0, "FuseSettings", QLatin1String("use fuseSettings instead"));
    qmlRegisterUncreatableType<FuseRecording>("Fuse", 1, 0, "FuseRecording", QLatin1String("use fuse.recording instead"));

    QQmlApplicationEngine engine;
    FuseEmulator fuse(engine.rootContext());
    engine.addImageProvider("spectrum", new SpectrumScreenImageProvider);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
#ifdef Q_OS_ANDROID
    QtAndroid::hideSplashScreen();
#endif
    return app.exec();
}
