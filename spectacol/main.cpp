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

using namespace Qt::StringLiterals;
int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);
    app.setOrganizationName(u"Licentia"_s);
    app.setOrganizationDomain(u"licentia.eu"_s);
    app.setApplicationName(u"Spectacol"_s);

    QString newSettings = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + u"/.fuserc"_s;
#ifdef Q_OS_ANDROID
    QString oldSettings = QDir::homePath() + u"/.fuserc"_s;
#else
    QString oldSettings = QDir::homePath() + u"/.spectacolrc"_s;
#endif
    QDir::home().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!QFile::exists(newSettings) && QFile::exists(oldSettings))
        QFile::rename(oldSettings, newSettings);

#if defined(Q_OS_WIN)
#elif defined(Q_OS_ANDROID)
#else
    QDir appPath{QCoreApplication::applicationDirPath()};
    appPath.cdUp();
    QString libsPath = appPath.absoluteFilePath(u"lib"_s);
    QString pluginsPath = appPath.absoluteFilePath(u"plugins"_s);
    QString qmlPath = appPath.absoluteFilePath(u"qml"_s);
#endif
    QCoreApplication::addLibraryPath(libsPath);
    QCoreApplication::addLibraryPath(pluginsPath);

    QQmlApplicationEngine engine;
    engine.addImportPath(qmlPath);

    engine.addImageProvider(u"spectrum"_s, new SpectrumScreenImageProvider);
    engine.loadFromModule(u"Spectacol"_s, u"Main"_s);
#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::hideSplashScreen();
#endif
    return app.exec();
}
