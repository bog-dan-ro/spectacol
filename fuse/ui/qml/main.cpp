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

#include "fusescreen.h"
#include "spectrumscreen.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick>

class SpectrumScreenImageProvider : public QQuickImageProvider
{
public:
    SpectrumScreenImageProvider()
        : QQuickImageProvider(Image, ForceAsynchronousImageLoading)
    {}

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

QImage SpectrumScreenImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(size)
    Q_UNUSED(requestedSize)
    if (id == QLatin1Literal(".."))
        return QImage(":/images/folder_up.png");
    QFileInfo inf(id);
    if (inf.isDir())
        return QImage(":/images/folder.svg");
    if (!inf.isFile())
        return QImage(":/images/zx_broken_cassette.png");

    QFile file(id);
    if (!file.open(QIODevice::ReadOnly))
        return QImage(":/images/zx_broken_cassette.png");

    QImage ret = buff2Image(file.readAll(), inf.fileName());
    if (ret.isNull())
        return QImage(":/images/zx_cassette_unknown.png");
    return ret;
}


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<FuseScreen>("Fuse", 1, 0, "FuseScreen");

    QQmlApplicationEngine engine;
    engine.addImageProvider("spectrum", new SpectrumScreenImageProvider);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
