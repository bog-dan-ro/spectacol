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

#include "fuseemulator.h"
#include "fusescreen.h"
#include "spectrumscreen.h"
#include "z80assembler.h"

#include <QQmlApplicationEngine>

#ifdef QT_WIDGETS_LIB
#include <QApplication>
#else
#include <QGuiApplication>
#endif

#ifdef QT_WIDGETS_LIB
#define QtQuickControlsApplication QApplication
#else
#define QtQuickControlsApplication QGuiApplication
#endif

int main(int argc, char *argv[])
{
    qputenv("QT_LABS_CONTROLS_STYLE", "default");
    QtQuickControlsApplication app(argc, argv);
    app.setOrganizationName("Fuse Emulator");
    app.setOrganizationDomain("licentia.eu");
    app.setApplicationName("Fuse Emulator");

    qmlRegisterType<FuseScreen>("Fuse", 1, 0, "FuseScreen");
    qmlRegisterType<Z80Assembler>("Fuse", 1, 0, "Z80Assembler");
    qmlRegisterUncreatableType<BreakpointsModel>("Fuse", 1, 0, "BreakpointsModel", QLatin1String("use breakpointsModel context property instead"));

    QQmlApplicationEngine engine;
    FuseEmulator fuse(engine.rootContext());
    engine.addImageProvider("spectrum", new SpectrumScreenImageProvider);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
