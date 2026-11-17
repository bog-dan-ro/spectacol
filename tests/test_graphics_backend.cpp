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

#include "graphicsbackend.h"
#include "graphicssettings.h"

#include <QSettings>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWindow>

#include <limits>

using namespace Qt::StringLiterals;
using namespace GraphicsBackend;

class GraphicsBackendTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDirectory;

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(u"SpectacolTests"_s);
        QCoreApplication::setApplicationName(u"Graphics"_s);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDirectory.path());
    }

    void init()
    {
        QSettings{}.clear();
    }

    void manualSelectionPersistsAndRequiresRestart()
    {
        GraphicsSettings graphics;
        QCOMPARE(graphics.choices().size(),
                 availableApis().size() + int(graphics.benchmarkAvailable()));
        QCOMPARE(graphics.selectedIndex(), 0);
        QVERIFY(!graphics.restartRequired());
        graphics.setSelectedIndex(-1);
        graphics.setSelectedIndex(graphics.choices().size());
        QCOMPARE(graphics.selectedIndex(), 0);
        if (availableApis().isEmpty())
            return;
        graphics.setSelectedIndex(1);
        QCOMPARE(graphics.selectedIndex(), 1);
        QVERIFY(graphics.restartRequired());
        GraphicsSettings nextLaunch;
        QCOMPARE(nextLaunch.selectedIndex(), 1);
        QVERIFY(!nextLaunch.restartRequired());
        graphics.setSelectedIndex(0);
        QVERIFY(!graphics.restartRequired());
    }

    void retestRestoresAutomaticSelection()
    {
        GraphicsSettings graphics;
        if (!graphics.benchmarkAvailable()) {
            graphics.retest();
            QVERIFY(!graphics.retestPending());
            return;
        }
        if (!availableApis().isEmpty())
            graphics.setSelectedIndex(1);
        graphics.retest();
        QCOMPARE(graphics.selectedIndex(), 0);
        QVERIFY(graphics.restartRequired());
        QVERIFY(graphics.retestPending());
        QVERIFY(QSettings{}.value(u"Graphics/rebenchmark"_s).toBool());
    }

    void manualDefaultDoesNotProbeOrBenchmark()
    {
        QSettings settings;
        int calls = 0;
        const auto selected = selectManualApi(
                settings, QSGRendererInterface::OpenGL,
                { QSGRendererInterface::OpenGL, QSGRendererInterface::Vulkan }, [&](Api) {
                    ++calls;
                    return false;
                });
        QCOMPARE(selected, QSGRendererInterface::OpenGL);
        QCOMPARE(calls, 0);
        QVERIFY(!settings.contains(u"GraphicsBenchmark/fingerprint"_s));
    }

    void manualFailureFallsBackToDefault()
    {
        QSettings settings;
        settings.setValue(u"Graphics/manualApi"_s, u"Vulkan"_s);
        const auto selected =
                selectManualApi(settings, QSGRendererInterface::OpenGL,
                                { QSGRendererInterface::OpenGL, QSGRendererInterface::Vulkan },
                                [](Api) { return false; });
        QCOMPARE(selected, QSGRendererInterface::OpenGL);
        QCOMPARE(settings.value(u"Graphics/manualApi"_s).toString(), u"OpenGL"_s);
    }

    void manualStartupCrashFallsBackOnNextLaunch()
    {
        QSettings settings;
        settings.setValue(u"Graphics/manualApi"_s, u"Vulkan"_s);
        int calls = 0;
        const auto initialize = [&](Api) {
            ++calls;
            return true;
        };
        const QList<Api> apis{ QSGRendererInterface::OpenGL, QSGRendererInterface::Vulkan };
        QCOMPARE(selectManualApi(settings, QSGRendererInterface::OpenGL, apis, initialize),
                 QSGRendererInterface::Vulkan);
        QCOMPARE(calls, 1);
        QCOMPARE(settings.value(u"Graphics/startingApi"_s).toString(), u"Vulkan"_s);
        QCOMPARE(selectManualApi(settings, QSGRendererInterface::OpenGL, apis, initialize),
                 QSGRendererInterface::OpenGL);
        QCOMPARE(calls, 1);
        settings.setValue(u"Graphics/manualApi"_s, u"Vulkan"_s);
        selectManualApi(settings, QSGRendererInterface::OpenGL, apis, initialize);
        confirmStartup();
        QVERIFY(!settings.contains(u"Graphics/startingApi"_s));
        QCOMPARE(selectManualApi(settings, QSGRendererInterface::OpenGL, apis, initialize),
                 QSGRendererInterface::Vulkan);
    }

    void nativeInitializationCrashFallsBackOnNextLaunch()
    {
        QSettings settings;
        settings.setValue(u"Graphics/manualApi"_s, u"Vulkan"_s);
        const QList<Api> apis{ QSGRendererInterface::OpenGL, QSGRendererInterface::Vulkan };
        struct DriverCrash
        {
        };
        QVERIFY_EXCEPTION_THROWN(selectManualApi(settings, QSGRendererInterface::OpenGL, apis,
                                                 [](Api) -> bool { throw DriverCrash{}; }),
                                 DriverCrash);
        QSettings reopened;
        int calls = 0;
        const auto selected =
                selectManualApi(reopened, QSGRendererInterface::OpenGL, apis, [&](Api) {
                    ++calls;
                    return true;
                });
        QCOMPARE(selected, QSGRendererInterface::OpenGL);
        QCOMPARE(calls, 0);
    }

    void fastestBackendAndCachedResults()
    {
        QTemporaryDir directory;
        QSettings settings(directory.filePath(u"settings.ini"_s), QSettings::IniFormat);
        const QList<Api> apis{ QSGRendererInterface::Vulkan, QSGRendererInterface::OpenGL };
        int calls = 0;
        const auto measure = [&](Api api) {
            ++calls;
            return Result{ .api = api, .frameMs = api == apis[0] ? 80.0 : 5.0 };
        };
        auto selection = select(settings, u"qt-os-screen"_s, apis, measure);
        QCOMPARE(calls, 2);
        QCOMPARE(selection.api, apis[1]);
        QVERIFY(selection.error.isEmpty());
        QCOMPARE(selection.results[0].frameMs, 80.0);
        // Reopen the settings to verify persistence across launches.
        QSettings reopened(settings.fileName(), QSettings::IniFormat);
        selection = select(reopened, u"qt-os-screen"_s, apis, measure);
        QCOMPARE(calls, 2);
        QCOMPARE(selection.api, apis[1]);
        QCOMPARE(selection.results[1].frameMs, 5.0);
        selection = select(reopened, u"new-qt-os-screen"_s, apis, measure);
        QCOMPARE(calls, 4);
        QCOMPARE(selection.api, apis[1]);
    }

    void failuresAreCachedAndDoNotBeatSuccessfulResults()
    {
        QTemporaryDir directory;
        QSettings settings(directory.filePath(u"settings.ini"_s), QSettings::IniFormat);
        const QList<Api> apis{ QSGRendererInterface::Vulkan, QSGRendererInterface::OpenGL };
        int calls = 0;
        const auto measure = [&](Api api) {
            ++calls;
            return api == apis[0] ? Result{ .api = api, .error = u"Device failed"_s }
                                  : Result{ .api = api, .frameMs = 10.0 };
        };
        auto selection = select(settings, u"v1"_s, apis, measure);
        QCOMPARE(selection.api, apis[1]);
        QVERIFY(selection.error.isEmpty());
        selection = select(settings, u"v1"_s, apis, measure);
        QCOMPARE(calls, 2);
        QCOMPARE(selection.results[0].error, u"Device failed"_s);
    }

    void allSlowOrFailed_data()
    {
        QTest::addColumn<double>("frameMs");
        QTest::newRow("very slow") << 100.0;
        QTest::newRow("zero") << 0.0;
        QTest::newRow("negative") << -1.0;
        QTest::newRow("nan") << std::numeric_limits<double>::quiet_NaN();
        QTest::newRow("infinite") << std::numeric_limits<double>::infinity();
    }

    void allSlowOrFailed()
    {
        QFETCH(double, frameMs);
        QTemporaryDir directory;
        QSettings settings(directory.filePath(u"settings.ini"_s), QSettings::IniFormat);
        const auto selection = select(
                settings, u"v1"_s, { QSGRendererInterface::Vulkan, QSGRendererInterface::OpenGL },
                [frameMs](Api api) {
                    return api == QSGRendererInterface::Vulkan
                            ? Result{ .api = api, .error = u"Unavailable"_s }
                            : Result{ .api = api, .frameMs = frameMs };
                });
        QVERIFY(!selection.error.isEmpty());
        QVERIFY(selection.error.contains(u"20 FPS"));
    }

    void interruptedBenchmarkResumesWithNextApi()
    {
        QTemporaryDir directory;
        QSettings settings(directory.filePath(u"settings.ini"_s), QSettings::IniFormat);
        const QList<Api> apis{ QSGRendererInterface::Vulkan, QSGRendererInterface::OpenGL };
        struct DriverCrash
        {
        };
        bool checkpointSaved = false;
        QVERIFY_EXCEPTION_THROWN(
                select(settings, u"v1"_s, apis,
                       [&](Api) -> Result {
                           // The checkpoint must reach disk before entering a possibly broken driver.
                           QSettings disk(settings.fileName(), QSettings::IniFormat);
                           checkpointSaved = disk.contains(u"GraphicsBenchmark/Vulkan/error"_s);
                           throw DriverCrash{};
                       }),
                DriverCrash);
        QVERIFY(checkpointSaved);
        QSettings restarted(settings.fileName(), QSettings::IniFormat);
        int calls = 0;
        const auto selection = select(restarted, u"v1"_s, apis, [&](Api api) {
            ++calls;
            return Result{ .api = api, .frameMs = 5.0 };
        });
        QCOMPARE(calls, 1);
        QCOMPARE(selection.api, apis[1]);
        QVERIFY(!selection.results[0].error.isEmpty());
    }

    void noCompiledBackend()
    {
        QTemporaryDir directory;
        QSettings settings(directory.filePath(u"settings.ini"_s), QSettings::IniFormat);
        const auto selection = select(settings, u"v1"_s, {}, [](Api) { return Result{}; });
        QVERIFY(!selection.error.isEmpty());
        QCOMPARE(selection.api, QSGRendererInterface::Unknown);
    }

    void rasterErrorHandlesKeyboardWithoutQuick()
    {
        bool rasterWindow = false;
        QTimer watchdog;
        connect(&watchdog, &QTimer::timeout, this, [] { QCoreApplication::exit(2); });
        watchdog.start(1000);
        QTimer::singleShot(50, this, [&rasterWindow] {
            for (auto *window : QGuiApplication::allWindows()) {
                if (!window->isVisible())
                    continue;
                rasterWindow = window->surfaceType() == QSurface::RasterSurface;
                QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &escape);
            }
        });
        QTest::ignoreMessage(QtCriticalMsg, "Test graphics failure");
        QCOMPARE(GraphicsBackend::showError(u"Test graphics failure"_s), 1);
        QVERIFY(rasterWindow);
    }

    void reportsActualSoftwareRenderer()
    {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        QQuickWindow window;
        window.show();
        QTRY_VERIFY(window.isSceneGraphInitialized());
        GraphicsSettings graphics;
        QCOMPARE(graphics.activeApi(), u"Software"_s);
    }
};

QTEST_MAIN(GraphicsBackendTest)
#include "test_graphics_backend.moc"
