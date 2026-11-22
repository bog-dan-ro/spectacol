#include "disks.h"

#include "zxemulator.h"
#include "zxsettings.h"
#ifdef Q_OS_ANDROID
#include "androidintegration.h"
#include "contenturi.h"
#endif

#include <QFile>
#include <QFileInfo>

#include <zxkit/core/Exception.h>

#include <span>

Disks::Disks(ZxEmulator *parent) : QObject{ parent }, m_zxEmulator(parent)
{
    connect(m_zxEmulator->settings(), &ZxSettings::currentMachineChanged, this,
            &Disks::updateCount);
    // ZxEmulator creates the engine after its members
    QMetaObject::invokeMethod(this, &Disks::updateCount, Qt::QueuedConnection);
}

int Disks::count() const
{
    return m_count;
}

void Disks::updateCount()
{
    const int count = int(m_zxEmulator->emulator().readLock()->diskDriveCount());
    if (m_count == count)
        return;
    m_count = count;
    emit countChanged();
}

void Disks::insertDisk(int idx, const QString &path)
{
    if (idx < 0 || idx >= count())
        return;
    QFile file(path);
    QString name;
    bool opened = false;
#ifdef Q_OS_ANDROID
    if (ContentUri::isContent(path)) {
        const QUrl url(path);
        name = AndroidIntegration::displayName(url);
        opened = AndroidIntegration::openDocument(url, file);
    } else
#endif
    {
        name = QFileInfo(path).fileName();
        opened = file.open(QIODevice::ReadOnly);
    }
    if (!opened) {
        m_zxEmulator->showMessage(tr("Can't open %1").arg(path), ZxEmulator::Error);
        return;
    }
    // Read the file before locking the emulator
    const auto data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        m_zxEmulator->showMessage(tr("Can't read '%1': %2").arg(name, file.errorString()),
                                  ZxEmulator::Error);
        return;
    }
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t *>(data.constData()),
                                         std::size_t(data.size()));
    try {
        {
            auto emulator = m_zxEmulator->emulator().writeLock();
            emulator->insertDisk(std::size_t(idx), bytes, QFileInfo(name).suffix().toStdString());
            if (idx == 0 && m_zxEmulator->settings()->autoLoad())
                emulator->autoBootDisk();
        }
        if (idx == 0)
            m_zxEmulator->setLoadedGame(name);
        m_zxEmulator->showMessage(tr("'%1' inserted into drive %2:").arg(name, QChar(u'A' + idx)));
    } catch (const std::exception &e) {
        m_zxEmulator->showMessage(
                tr("Can't insert '%1': %2").arg(name, QString::fromUtf8(e.what())),
                ZxEmulator::Error);
    }
}

void Disks::ejectDisk(int idx)
{
    if (idx < 0 || idx >= count())
        return;
    m_zxEmulator->emulator().writeLock()->ejectDisk(std::size_t(idx));
}
