#ifndef SNAPSHOTSCREEN_H
#define SNAPSHOTSCREEN_H

#include <QImage>


QImage buff2Image(const QByteArray &buffer, const QString &fileName = QString());
QImage buff2Image(const unsigned char *buffer, size_t bufferSize, const QString &fileName = QString());

#endif // SNAPSHOTSCREEN_H
