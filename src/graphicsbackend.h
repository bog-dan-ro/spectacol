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

#pragma once

#include <QList>
#include <QSGRendererInterface>
#include <QString>

#include <functional>

class QSettings;

namespace GraphicsBackend {

using Api = QSGRendererInterface::GraphicsApi;

// Below 20 FPS even the small benchmark scene is unsuitable for interactive use.
inline constexpr double maximumFrameMs = 50.0;

struct Result
{
    Api api = QSGRendererInterface::Unknown;
    double frameMs = 0;
    QString device;
    QString error;
    bool usable() const;
};

struct Selection
{
    Api api = QSGRendererInterface::Unknown;
    QList<Result> results;
    QString error;
};

QString apiName(Api api);
QList<Api> availableApis();
Api selectManualApi(QSettings &settings, Api defaultApi, const QList<Api> &apis,
                    const std::function<bool(Api)> &canInitialize);
void confirmStartup();
// Kept independent of graphics initialization so cache/recovery policy can be tested.
Selection select(QSettings &settings, const QString &fingerprint, const QList<Api> &apis,
                 const std::function<Result(Api)> &benchmark);
Selection configure(bool rebenchmark);
int showError(const QString &message);

} // namespace GraphicsBackend
