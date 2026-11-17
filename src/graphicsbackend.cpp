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
#include "zximage.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDir>
#include <QFontMetrics>
#include <QGamepadManager>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QPalette>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickGraphicsConfiguration>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QRasterWindow>
#include <QScreen>
#include <QSettings>
#include <QSGSimpleTextureNode>
#include <QSysInfo>
#include <rhi/qrhi.h>
#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#include <rhi/qrhi_platform.h>
#endif

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

using namespace Qt::StringLiterals;

namespace {

class GraphicsMessages
{
    Q_DECLARE_TR_FUNCTIONS(GraphicsMessages)
};

#ifndef Q_OS_ANDROID
class BenchmarkScreen : public QQuickItem
{
public:
    explicit BenchmarkScreen(QQuickItem *parent) : QQuickItem(parent)
    {
        setFlag(ItemHasContents);
        setSize(parent->size());
        m_image = QImage(640, 512, QImage::Format_RGBA8888);
    }

    void advance(int frame)
    {
        // Full-frame Next-sized uploads through the same texture used by ZxScreen.
        m_image.fill(frame % 2 ? Qt::red : Qt::blue);
        update();
    }

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override
    {
        auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);
        if (!node) {
            node = new QSGSimpleTextureNode;
            node->setTexture(new ZxTexture);
            node->setOwnsTexture(true);
        }
        static_cast<ZxTexture *>(node->texture())->setFrame({ m_image, m_image.rect(), { 1, 1 } });
        node->setRect(boundingRect());
        node->markDirty(QSGNode::DirtyMaterial);
        return node;
    }

private:
    QImage m_image;
};

GraphicsBackend::Result benchmark(GraphicsBackend::Api api, QSize pixelSize)
{
    using namespace GraphicsBackend;
    Result result{ .api = api };
    const auto fail = [&](const QString &error) {
        result.error = error;
        return result;
    };
    QQuickWindow::setGraphicsApi(api);
#if QT_CONFIG(vulkan)
    // Must outlive the render control and its device.
    QVulkanInstance vulkan;
    if (api == QSGRendererInterface::Vulkan) {
        vulkan.setExtensions(QQuickGraphicsConfiguration::preferredInstanceExtensions());
        if (!vulkan.create())
            return fail(GraphicsMessages::tr("Could not initialize Vulkan."));
    }
#endif
    QQuickRenderControl control;
    QQuickWindow window(&control);
#if QT_CONFIG(vulkan)
    if (api == QSGRendererInterface::Vulkan)
        window.setVulkanInstance(&vulkan);
#endif
    window.resize(pixelSize);
    window.contentItem()->setSize(pixelSize);
    if (!control.initialize() || !control.rhi())
        return fail(GraphicsMessages::tr("Could not initialize graphics."));
    if (window.rendererInterface()->graphicsApi() != api)
        return fail(GraphicsMessages::tr("Qt initialized a different graphics API."));
    auto *rhi = control.rhi();
    result.device = QString::fromUtf8(rhi->driverInfo().deviceName);

    std::unique_ptr<QRhiTexture> texture(
            rhi->newTexture(QRhiTexture::RGBA8, pixelSize, 1,
                            QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    std::unique_ptr<QRhiRenderBuffer> depth(
            rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize));
    if (!texture->create() || !depth->create())
        return fail(GraphicsMessages::tr("Could not allocate graphics buffers."));
    QRhiTextureRenderTargetDescription description{ QRhiColorAttachment(texture.get()) };
    description.setDepthStencilBuffer(depth.get());
    std::unique_ptr<QRhiTextureRenderTarget> target(rhi->newTextureRenderTarget(description));
    std::unique_ptr<QRhiRenderPassDescriptor> pass(target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    if (!target->create())
        return fail(GraphicsMessages::tr("Could not create a graphics render target."));
    window.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(target.get()));

    QQmlEngine engine;
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
    QDir appPath(QCoreApplication::applicationDirPath());
    appPath.cdUp();
    engine.addImportPath(appPath.absoluteFilePath(u"qml"_s));
#endif
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/Spectacol/qml/GraphicsBenchmark.qml"_s));
    std::unique_ptr<QObject> scene(component.create());
    auto *root = qobject_cast<QQuickItem *>(scene.get());
    if (!root)
        return fail(component.errorString());
    root->setSize(pixelSize);
    root->setParentItem(window.contentItem());
    auto *screenArea = root->findChild<QQuickItem *>(u"benchmarkScreen"_s);
    if (!screenArea)
        return fail(GraphicsMessages::tr("Could not load the graphics benchmark."));
    BenchmarkScreen screen(screenArea);

    const auto render = [&](int frame, bool readback) {
        root->setProperty("frame", frame);
        screen.advance(frame);
        control.polishItems();
        control.beginFrame();
        if (!rhi->isRecordingFrame())
            return false;
        control.sync();
        control.render();
        QRhiReadbackResult pixels;
        bool completed = false;
        if (readback) {
            pixels.completed = [&completed] { completed = true; };
            auto *updates = rhi->nextResourceUpdateBatch();
            updates->readBackTexture(QRhiReadbackDescription(texture.get()), &pixels);
            control.commandBuffer()->resourceUpdate(updates);
        }
        control.endFrame();
        // Include actual GPU work, not just CPU command submission. No vsync cap.
        if (rhi->finish() != QRhi::FrameOpSuccess || rhi->isDeviceLost())
            return false;
        if (!readback)
            return true;
        if (!completed || pixels.pixelSize != pixelSize || pixels.data.size() < 4)
            return false;
        // The center of the synthetic screen must contain this frame's upload.
        const auto offset =
                (pixelSize.height() / 2 * pixelSize.width() + pixelSize.width() / 2) * 4;
        if (pixels.data.size() < offset + 4)
            return false;
        const auto *rgba = reinterpret_cast<const uchar *>(pixels.data.constData() + offset);
        return frame % 2 ? rgba[0] > 240 && rgba[2] < 15 : rgba[2] > 240 && rgba[0] < 15;
    };

    // Validate changing texture uploads, then exclude pipeline/font warmup from timing.
    QElapsedTimer warmup;
    warmup.start();
    for (int frame = 0; frame < 8 && (frame < 2 || warmup.elapsed() < 600); ++frame) {
        if (!render(frame, frame < 2))
            return fail(GraphicsMessages::tr("Graphics rendering or texture upload failed."));
    }
    QElapsedTimer timer;
    timer.start();
    int frames = 0;
    do {
        if (!render(frames++, false))
            return fail(GraphicsMessages::tr("The graphics device stopped rendering."));
    } while (timer.elapsed() < 600);
    result.frameMs = double(timer.nsecsElapsed()) / 1'000'000.0 / frames;
    // The scene and its GPU nodes go away before the render target/device.
    return result;
}

#endif

// This window must remain usable when no Qt Quick graphics API can initialize.
class ErrorWindow : public QRasterWindow
{
public:
    explicit ErrorWindow(QString message) : m_message(std::move(message))
    {
        setTitle(GraphicsMessages::tr("Spectacol: graphics unavailable"));
        resize(640, 480);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const auto palette = QGuiApplication::palette();
        painter.fillRect(QRect(QPoint(), size()), palette.window());
        painter.setPen(palette.windowText().color());
        painter.setFont(QGuiApplication::font());
        const int margin = QFontMetrics(painter.font()).height();
        painter.drawText(QRect(margin, margin, width() - 2 * margin, height() - 2 * margin),
                         Qt::AlignCenter | Qt::TextWordWrap, m_message);
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Back || event->key() == Qt::Key_Escape
            || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Space)
            QCoreApplication::exit(1);
    }
    void mouseReleaseEvent(QMouseEvent *) override
    {
        QCoreApplication::exit(1);
    }
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::TouchEnd) {
            QCoreApplication::exit(1);
            return true;
        }
        return QRasterWindow::event(event);
    }

private:
    QString m_message;
};

} // namespace

namespace GraphicsBackend {

bool Result::usable() const
{
    return error.isEmpty() && std::isfinite(frameMs) && frameMs > 0;
}

QString apiName(Api api)
{
    switch (api) {
    case QSGRendererInterface::Software:
        return u"Software"_s;
    case QSGRendererInterface::OpenGL:
        return u"OpenGL"_s;
    case QSGRendererInterface::Vulkan:
        return u"Vulkan"_s;
    case QSGRendererInterface::Direct3D11:
        return u"D3D11"_s;
    case QSGRendererInterface::Direct3D12:
        return u"D3D12"_s;
    case QSGRendererInterface::Metal:
        return u"Metal"_s;
    default:
        return u"Unknown"_s;
    }
}

QList<Api> availableApis()
{
    QList<Api> apis;
#if defined(Q_OS_ANDROID)
#if QT_CONFIG(opengl)
    apis << QSGRendererInterface::OpenGL;
#endif
#if QT_CONFIG(vulkan)
    apis << QSGRendererInterface::Vulkan;
#endif
#elif defined(Q_OS_WIN)
    apis << QSGRendererInterface::Direct3D11 << QSGRendererInterface::Direct3D12;
#elif defined(Q_OS_DARWIN)
#if QT_CONFIG(metal)
    apis << QSGRendererInterface::Metal;
#endif
#if QT_CONFIG(opengl)
    apis << QSGRendererInterface::OpenGL;
#endif
#else
#if QT_CONFIG(vulkan)
    apis << QSGRendererInterface::Vulkan;
#endif
#if QT_CONFIG(opengl)
    apis << QSGRendererInterface::OpenGL;
#endif
#endif
    return apis;
}

Api selectManualApi(QSettings &settings, Api defaultApi, const QList<Api> &apis,
                    const std::function<bool(Api)> &canInitialize)
{
    Api selected = defaultApi;
    const auto saved = settings.value(u"Graphics/manualApi"_s).toString();
    for (const auto api : apis) {
        if (apiName(api) == saved)
            selected = api;
    }
    const auto unfinished = settings.value(u"Graphics/startingApi"_s).toString();
    if (selected != defaultApi) {
        bool initialized = false;
        if (unfinished != apiName(selected)) {
            // A crash inside the driver must also recover on the next launch.
            settings.setValue(u"Graphics/startingApi"_s, apiName(selected));
            settings.sync();
            if (settings.status() == QSettings::NoError)
                initialized = canInitialize(selected);
        }
        if (!initialized) {
            qWarning().noquote() << apiName(selected) << "could not start; falling back to"
                                 << apiName(defaultApi);
            selected = defaultApi;
        }
    }
    settings.setValue(u"Graphics/manualApi"_s, apiName(selected));
    settings.remove(u"Graphics/rebenchmark"_s);
    if (selected == defaultApi)
        settings.remove(u"Graphics/startingApi"_s);
    else
        settings.setValue(u"Graphics/startingApi"_s, apiName(selected));
    settings.sync();
    return selected;
}

void confirmStartup()
{
    QSettings settings;
    settings.remove(u"Graphics/startingApi"_s);
    settings.sync();
}

Selection select(QSettings &settings, const QString &fingerprint, const QList<Api> &apis,
                 const std::function<Result(Api)> &runBenchmark)
{
    settings.beginGroup(u"GraphicsBenchmark"_s);
    if (settings.value(u"fingerprint"_s).toString() != fingerprint) {
        settings.remove(QString{});
        settings.setValue(u"fingerprint"_s, fingerprint);
    }
    Selection selection;
    double bestMs = std::numeric_limits<double>::infinity();
    QStringList summary;
    for (const auto api : apis) {
        const auto name = apiName(api);
        const auto key = name + u'/';
        Result result{ .api = api };
        const bool cached = settings.contains(key + u"error"_s);
        if (cached) {
            result.frameMs = settings.value(key + u"frameMs"_s).toDouble();
            result.device = settings.value(key + u"device"_s).toString();
            result.error = settings.value(key + u"error"_s).toString();
        } else {
            // A driver crash/forced shutdown must not trap the next launch on the same API.
            settings.setValue(key + u"error"_s,
                              GraphicsMessages::tr("The previous graphics test was interrupted."));
            settings.sync();
            result = runBenchmark(api);
            result.api = api;
        }
        if (!result.usable() && result.error.isEmpty())
            result.error = GraphicsMessages::tr("Invalid graphics benchmark result.");
        if (!cached) {
            settings.setValue(key + u"frameMs"_s, result.frameMs);
            settings.setValue(key + u"device"_s, result.device);
            settings.setValue(key + u"error"_s, result.error);
            settings.sync();
        }
        selection.results.append(result);
        const auto detail = result.usable()
                ? GraphicsMessages::tr("%1 FPS").arg(1000.0 / result.frameMs, 0, 'f', 1)
                : result.error;
        summary << u"%1: %2"_s.arg(name, detail);
        qInfo().noquote() << (cached ? "Graphics benchmark (cached):" : "Graphics benchmark:")
                          << name << detail << result.device;
        if (result.usable() && result.frameMs < bestMs) {
            bestMs = result.frameMs;
            selection.api = api;
        }
    }
    settings.setValue(u"selectedApi"_s, apiName(selection.api));
    settings.endGroup();
    settings.sync();
    if (bestMs > maximumFrameMs) {
        selection.error =
                GraphicsMessages::tr("No graphics API could render fast enough for Spectacol "
                                     "(at least 20 FPS is required).\n\n%1\n\n"
                                     "Try updating your graphics driver or operating system.")
                        .arg(summary.join(u'\n'));
    }
    return selection;
}

Selection configure(bool rebenchmark)
{
    // Explicit Qt overrides are useful for diagnostics and must not overwrite auto results.
    if (!qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND")
        || !qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")
        || !qEnvironmentVariableIsEmpty("QMLSCENE_DEVICE")) {
        qInfo() << "Using the explicitly requested Qt graphics backend; benchmark bypassed.";
        return {};
    }
    const auto platform = QGuiApplication::platformName();
    if (platform == u"offscreen" || platform == u"minimal") {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        return {};
    }
    QQuickWindow::setSceneGraphBackend(u"rhi"_s);
    QSettings settings;
#ifdef Q_OS_ANDROID
    if (rebenchmark)
        qInfo() << "Graphics benchmarking is disabled on Android.";
    const auto api = selectManualApi(
            settings, QSGRendererInterface::OpenGL, availableApis(), [](Api requested) {
#if QT_CONFIG(vulkan)
                if (requested == QSGRendererInterface::Vulkan) {
                    // Native initialization only, without creating a QML scene or benchmarking.
                    QVulkanInstance instance;
                    instance.setExtensions(
                            QQuickGraphicsConfiguration::preferredInstanceExtensions());
                    if (!instance.create())
                        return false;
                    QRhiVulkanInitParams params;
                    params.inst = &instance;
                    return QRhi::probe(QRhi::Vulkan, &params);
                }
#endif
                return false;
            });
    QQuickWindow::setGraphicsApi(api);
    qInfo().noquote() << "Android graphics API:" << apiName(api);
    return { .api = api };
#else
    if (rebenchmark || settings.value(u"Graphics/rebenchmark"_s, false).toBool()) {
        settings.remove(u"GraphicsBenchmark"_s);
        settings.remove(u"Graphics/manualApi"_s);
        settings.remove(u"Graphics/rebenchmark"_s);
        settings.sync();
    }
    const auto manual = settings.value(u"Graphics/manualApi"_s).toString();
    const auto apis = availableApis();
    for (const auto api : apis) {
        if (manual != apiName(api))
            continue;
        QQuickWindow::setGraphicsApi(api);
        qInfo().noquote() << "Using manually selected graphics API:" << manual;
        return { .api = api };
    }
    const auto *screen = QGuiApplication::primaryScreen();
    const auto size = screen ? screen->size() * screen->devicePixelRatio() : QSize(960, 768);
    // Cache schema/workload version plus Qt, OS, presentation platform and output size.
    const auto fingerprint =
            u"1|%1|%2|%3|%4|%5|%6x%7"_s
                    .arg(QString::fromLatin1(qVersion()), QSysInfo::productType(),
                         QSysInfo::productVersion(), QSysInfo::kernelVersion(), platform)
                    .arg(size.width())
                    .arg(size.height());
    const auto selection =
            select(settings, fingerprint, apis, [size](Api api) { return benchmark(api, size); });
    if (selection.error.isEmpty()) {
        QQuickWindow::setGraphicsApi(selection.api);
        qInfo().noquote() << "Selected graphics API:" << apiName(selection.api);
    }
    return selection;
#endif
}

int showError(const QString &message)
{
    qCritical().noquote() << message;
    ErrorWindow window(message + u"\n\n"_s
                       + GraphicsMessages::tr("Tap, click or press Back to close."));
    QObject::connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonPressEvent,
                     &window, [](int, QGamepadManager::GamepadButton button, double) {
                         if (button == QGamepadManager::ButtonA
                             || button == QGamepadManager::ButtonB
                             || button == QGamepadManager::ButtonStart
                             || button == QGamepadManager::ButtonSelect)
                             QCoreApplication::exit(1);
                     });
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    window.showFullScreen();
#else
    window.show();
#endif
    return QGuiApplication::exec();
}

} // namespace GraphicsBackend
