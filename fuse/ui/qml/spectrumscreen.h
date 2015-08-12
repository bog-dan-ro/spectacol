#ifndef SNAPSHOTSCREEN_H
#define SNAPSHOTSCREEN_H

#include <QImage>
#include <QQuickImageProvider>


QImage buff2Image(const QByteArray &buffer, const QString &fileName = QString());
QImage buff2Image(const unsigned char *buffer, size_t bufferSize, const QString &fileName = QString());


class SpectrumScreenImageProvider : public QQuickImageProvider
{
public:
    SpectrumScreenImageProvider()
        : QQuickImageProvider(Image, ForceAsynchronousImageLoading)
    {}

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

#endif // SNAPSHOTSCREEN_H
