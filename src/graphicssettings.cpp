/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "graphicssettings.h"
#include "graphicsbackend.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QSettings>

using namespace Qt::StringLiterals;

GraphicsSettings::GraphicsSettings(QObject *parent)
    : QObject(parent), m_startupIndex(selectedIndex())
{
}

QStringList GraphicsSettings::choices() const
{
    QStringList names;
    if (benchmarkAvailable())
        names << tr("Automatic");
    for (const auto api : GraphicsBackend::availableApis())
        names << GraphicsBackend::apiName(api);
    return names;
}

int GraphicsSettings::selectedIndex() const
{
    const auto saved = QSettings{}.value(u"Graphics/manualApi"_s).toString();
    const auto apis = GraphicsBackend::availableApis();
    for (int index = 0; index < apis.size(); ++index) {
        if (saved == GraphicsBackend::apiName(apis[index]))
            return index + int(benchmarkAvailable());
    }
    return 0;
}

void GraphicsSettings::setSelectedIndex(int index)
{
    const auto apis = GraphicsBackend::availableApis();
    const int offset = int(benchmarkAvailable());
    if (index < 0 || index >= apis.size() + offset || index == selectedIndex())
        return;
    QSettings settings;
    settings.setValue(u"Graphics/manualApi"_s,
                      index >= offset ? GraphicsBackend::apiName(apis[index - offset]) : QString{});
    settings.remove(u"Graphics/rebenchmark"_s);
    settings.sync();
    emit changed();
}

QString GraphicsSettings::activeApi() const
{
    for (auto *window : QGuiApplication::allWindows()) {
        auto *quick = qobject_cast<QQuickWindow *>(window);
        if (quick && quick->isSceneGraphInitialized())
            return GraphicsBackend::apiName(quick->rendererInterface()->graphicsApi());
    }
    return GraphicsBackend::apiName(QQuickWindow::graphicsApi());
}

QString GraphicsSettings::measurements() const
{
    QSettings settings;
    QStringList lines;
    for (const auto api : GraphicsBackend::availableApis()) {
        const auto name = GraphicsBackend::apiName(api);
        const auto key = u"GraphicsBenchmark/"_s + name + u'/';
        if (!settings.contains(key + u"error"_s))
            continue;
        const auto error = settings.value(key + u"error"_s).toString();
        const auto frameMs = settings.value(key + u"frameMs"_s).toDouble();
        lines << (error.isEmpty() && frameMs > 0
                          ? tr("%1: %2 FPS").arg(name).arg(1000.0 / frameMs, 0, 'f', 1)
                          : tr("%1: test failed").arg(name));
    }
    return lines.isEmpty() ? tr("No saved measurements.") : lines.join(u'\n');
}

bool GraphicsSettings::restartRequired() const
{
    return retestPending() || selectedIndex() != m_startupIndex;
}

bool GraphicsSettings::retestPending() const
{
    return benchmarkAvailable() && QSettings{}.value(u"Graphics/rebenchmark"_s, false).toBool();
}

bool GraphicsSettings::benchmarkAvailable() const
{
#ifdef Q_OS_ANDROID
    return false;
#else
    return true;
#endif
}

void GraphicsSettings::retest()
{
    if (!benchmarkAvailable())
        return;
    QSettings settings;
    settings.remove(u"Graphics/manualApi"_s);
    settings.setValue(u"Graphics/rebenchmark"_s, true);
    settings.sync();
    emit changed();
}
