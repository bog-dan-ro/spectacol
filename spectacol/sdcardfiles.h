/*
    Copyright (c) 2026, BogDan Vatra <bogdan@kde.org>

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

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>
#include <QVariantList>

#include <functional>

namespace zxkit {
class FatVolume;
}

class ZxEmulator;

/// The files on the ZX Next's SD card image, for the SD card page: browse its folders, copy host
/// files onto it, make folders, remove files, pick another image. NextZXOS caches the card, so
/// the Next restarts after a change.
class SdCardFiles : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use ZxEmulator.sdCard instead")
    Q_PROPERTY(bool inUse READ inUse NOTIFY changed FINAL)
    Q_PROPERTY(QString imagePath READ imagePath NOTIFY changed FINAL)
    Q_PROPERTY(bool customImage READ customImage NOTIFY changed FINAL)
    Q_PROPERTY(bool available READ available NOTIFY changed FINAL)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY changed FINAL)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY changed FINAL)
    Q_PROPERTY(qint64 freeBytes READ freeBytes NOTIFY changed FINAL)
    Q_PROPERTY(QString folder READ folder NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed FINAL)

public:
    explicit SdCardFiles(ZxEmulator *parent);

    /// The current machine uses the card (the Next)
    bool inUse() const
    {
        return m_inUse;
    }
    QString imagePath() const;
    bool customImage() const;
    /// The image has a FAT file system Spectacol can read
    bool available() const
    {
        return m_available;
    }
    bool readOnly() const
    {
        return m_readOnly;
    }
    qint64 totalBytes() const
    {
        return m_totalBytes;
    }
    qint64 freeBytes() const
    {
        return m_freeBytes;
    }
    QString folder() const
    {
        return m_folder;
    }
    /// The current folder's entries ({name, size, isDir}), folders first; ".." outside the root
    QVariantList entries() const
    {
        return m_entries;
    }

public slots:
    void refresh();
    /// A folder of the current one, or ".."
    void openFolder(const QString &name);
    /// Copies a host file (a local path or a file dialog's url) into the current folder
    void addFile(const QUrl &file);
    void makeFolder(const QString &name);
    void remove(const QString &name);
    /// Uses another image (a local path or url), or the downloaded one for an empty url
    void useImage(const QUrl &image);

signals:
    void changed();

private:
    QString childPath(const QString &name) const;
    bool withVolume(const std::function<void(zxkit::FatVolume &volume)> &fn, bool write);
    void updateInUse();

    ZxEmulator *m_zxEmulator = nullptr;
    bool m_inUse = false;
    bool m_available = false;
    bool m_readOnly = false;
    qint64 m_totalBytes = 0;
    qint64 m_freeBytes = 0;
    QString m_folder; // "/" or "/games/next"
    QVariantList m_entries;
};
