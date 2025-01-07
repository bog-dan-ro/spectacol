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

#include "folderlistmodel.h"
#include "fuseemulator.h"
#include "fuserecording.h"
#include "zxscreen.h"
#include "fusetapedata.h"
#include "pokemodel.h"
#include "spectrumscreen.h"
#include "z80assembler.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
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

#if defined(Q_OS_WIN)
#elif defined(Q_OS_ANDROID)
#else
    QDir appPath{QCoreApplication::applicationDirPath()};
    appPath.cdUp();
    QString libsPath = appPath.absoluteFilePath("lib");
    QString pluginsPath = appPath.absoluteFilePath("plugins");
    QString qmlPath = appPath.absoluteFilePath("qml");
#endif
    QCoreApplication::addLibraryPath(libsPath);
    QCoreApplication::addLibraryPath(pluginsPath);

    QQmlApplicationEngine engine;
    engine.addImportPath(qmlPath);

    engine.addImageProvider("spectrum", new SpectrumScreenImageProvider);
    engine.loadFromModule("Spectacol", "Main");
#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::hideSplashScreen();
#endif
    return app.exec();
}
