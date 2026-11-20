/*
    Copyright (c) 2016-2026, BogDan Vatra <bogdan@kde.org>

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

#include "folderlistmodel.h"
#include "screencache.h"

#include <zxkit/media/MediaPreview.h>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QSettings>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {
bool within(const QString &folder, const QString &root)
{
    return root.isEmpty() || root == u"/"_s || folder == root || folder.startsWith(root + u'/');
}
} // namespace

FolderListModel::FolderListModel(QObject *parent) : QAbstractListModel(parent) { }

FolderListModel::~FolderListModel() { }

void FolderListModel::setFolder(QString folder)
{
    // The row we open knows the provider's name for a device folder
    QString name;
    for (const auto &entry : m_allFiles) {
        if (entry.path == folder) {
            name = entry.name;
            break;
        }
    }
    if (m_location.setFolder(folder, name))
        moved();
}

void FolderListModel::moved()
{
    if (!m_search.isEmpty()) {
        m_search.clear();
        emit searchChanged();
    }
    emit folderChanged(m_location.folder());
    refresh();
}

void FolderListModel::setRootFolder(QString rootFolder)
{
    const auto previous = m_location.root();
    m_location.setRoot(rootFolder);
    if (m_location.root() == previous)
        return;
    emit rootFolderChanged(m_location.root());
    if (folder().isEmpty() || !m_location.allowed(folder()))
        setFolder(m_location.root());
    else
        refresh();
}

void FolderListModel::setHomeFolder(QString homeFolder)
{
    const auto previous = m_location.home();
    m_location.setHome(homeFolder);
    if (m_location.home() == previous)
        return;
    const bool shown =
            !folder().isEmpty() && (folder() == previous || folder() == m_location.home());
    emit homeFolderChanged();
    if (shown)
        refresh();
}

void FolderListModel::setDeviceFolders(QStringList deviceFolders)
{
    if (m_location.deviceFolders() == deviceFolders)
        return;
    const auto previous = folder();
    m_location.setDeviceFolders(deviceFolders);
    emit deviceFoldersChanged();
    if (folder() != previous)
        moved(); // forgotten while we were in it
    else if (!folder().isEmpty() && folder() == m_location.home())
        refresh();
}

QString FolderListModel::folderTitle() const
{
    return m_location.title();
}

QString FolderListModel::folderName() const
{
    return m_location.name();
}

bool FolderListModel::inDeviceFolder() const
{
    return m_location.inDevice();
}

void FolderListModel::setSearch(const QString &search)
{
    if (m_search == search)
        return;
    m_search = search;
    emit searchChanged();
    refilter();
}
QString FolderListModel::selectionKey() const
{
    return u"FileSelections/"_s
            + QString::fromLatin1(
                    QCryptographicHash::hash(
                            (folder() + u'|' + QString::number(m_filterClass)).toUtf8(),
                            QCryptographicHash::Sha256)
                            .toHex());
}

FolderListModel::FileSortCriteria FolderListModel::sortCriteria() const
{
    return m_sortCriteria;
}

void FolderListModel::setSortCriteria(FolderListModel::FileSortCriteria sortCriteria)
{
    if (m_sortCriteria == sortCriteria)
        return;

    m_sortCriteria = sortCriteria;
    emit sortCriteriaChanged(sortCriteria);
    refresh();
}

bool FolderListModel::showDirsFirst() const
{
    return m_showDirsFirst;
}

void FolderListModel::setShowDirsFirst(bool showDirsFirst)
{
    if (m_showDirsFirst == showDirsFirst)
        return;

    m_showDirsFirst = showDirsFirst;
    emit showDirsFirstChanged(showDirsFirst);
    refresh();
}

int FolderListModel::currentIndex() const
{
    return m_currentIndex;
}

void FolderListModel::setCurrentIndex(int currentIndex)
{
    if (m_refreshing || currentIndex < 0 || std::size_t(currentIndex) >= m_files.size())
        return;
    const bool changed = m_currentIndex != currentIndex;
    m_currentIndex = currentIndex;
    if (m_search.isEmpty())
        rememberSelection(currentIndex);
    if (changed)
        emit currentIndexChanged(currentIndex);
}

void FolderListModel::rememberSelection(int index)
{
    if (m_refreshing || index < 0 || std::size_t(index) >= m_files.size())
        return;
    QSettings settings;
    const auto key = selectionKey(), selected = path(index);
    if (settings.value(key).toString() != selected)
        settings.setValue(key, selected);
}

FolderListModel::FilterType FolderListModel::filterClass() const
{
    return m_filterClass;
}

void FolderListModel::setFilterClass(FolderListModel::FilterType filterClass)
{
    if (m_filterClass == filterClass)
        return;

    m_filterClass = filterClass;
    emit filterClassChanged(filterClass);
    refresh();
}

bool FolderListModel::isDir(int index)
{
    if (size_t(index) >= m_files.size())
        return false;
    return m_files[index].dir;
}

bool FolderListModel::isDeviceFolder(int index)
{
    if (size_t(index) >= m_files.size())
        return false;
    return m_files[index].device;
}

QString FolderListModel::path(int index)
{
    if (size_t(index) >= m_files.size())
        return QString();
    return m_files[index].path;
}

void FolderListModel::cdUp()
{
    if (m_location.goUp())
        moved();
}

int FolderListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_files.size();
}

QVariant FolderListModel::data(const QModelIndex &index, int role) const
{
    if (size_t(index.row()) >= m_files.size())
        return {};

    const auto &entry = m_files[index.row()];
    switch (role) {
    case Name:
        return entry.name;
    case Path:
        return entry.path;
    case IsDir:
        return entry.dir;
    case IsDevice:
        return entry.device;
    case TypeIcon:
        if (entry.dir)
            return entry.device ? u"qrc:/images/folder-device.svg"_s : u"qrc:/images/folder.svg"_s;
        switch (zxkit::classifyMediaFile(entry.name.toStdString(), {})) {
        case zxkit::MediaClass::Disk:
            return u"qrc:/images/floppy.svg"_s;
        case zxkit::MediaClass::Tape:
        case zxkit::MediaClass::Microdrive:
            return u"qrc:/images/cassette-tape.svg"_s;
        default:
            return u"qrc:/images/binary.svg"_s;
        }
    case CoverSource:
        if (entry.dir)
            return data(index, TypeIcon);
        // What the listing knows, so the provider asks the storage nothing it doesn't cache
        return u"image://spectrum/"_s
                + ScreenCover{ entry.path, entry.name, entry.size,
                               entry.modified.isValid() ? entry.modified.toMSecsSinceEpoch() : 0 }
                          .id();
    }
    return {};
}

QHash<int, QByteArray> FolderListModel::roleNames() const
{
    static QHash<int, QByteArray> ret = { { Name, "name" },
                                          { Path, "path" },
                                          { IsDir, "isDir" },
                                          { IsDevice, "isDevice" },
                                          { CoverSource, "coverSource" },
                                          { TypeIcon, "typeIcon" } };
    return ret;
}

bool FolderListModel::accepts(const QString &fileName, std::span<const uint8_t> header) const
{
    if (m_filterClass == HostFiles)
        return true;
    const auto fileClass = zxkit::classifyMediaFile(fileName.toStdString(), header);
    if (fileClass == zxkit::MediaClass::Unknown)
        return false;

    switch (m_filterClass) {
    case Tapes:
        return fileClass == zxkit::MediaClass::Tape;
    case Disks:
        return fileClass == zxkit::MediaClass::Disk;
    case Microdrives:
        return fileClass == zxkit::MediaClass::Microdrive;
    default:
        break;
    }

    return true;
}

bool FolderListModel::canOpen(const QString &path)
{
    if (m_filterClass == HostFiles)
        return true;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const auto buffer = f.read(64);
    return accepts(path,
                   std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buffer.constData()),
                                            size_t(buffer.size())));
}

void FolderListModel::refresh()
{
    const auto generation = ++m_listing; // a listing still on its way is no longer wanted
    if (inDeviceFolder()) {
        // Every call is a round trip to the provider: off the GUI thread, by the names there
        const auto shown = folder();
        if (shown != m_listedFolder) {
            m_allFiles.clear();
            refilter();
            m_listedFolder = shown;
        }
        setBusy(true);
        QtConcurrent::run([shown] {
            return StorageFiles::list(shown);
        }).then(this, [this, generation, shown](const std::vector<StorageFiles::Entry> &files) {
            if (generation != m_listing || shown != folder())
                return;
            m_allFiles.clear();
            for (const auto &file : files) {
                if (file.dir || accepts(file.name, {}))
                    m_allFiles.push_back({ file.name, file.path, file.modified, file.dir, false,
                                           file.dir ? 0 : file.size });
            }
            sortAndRefilter();
            setBusy(false);
        });
        return;
    }
    m_listedFolder.clear();
    setBusy(false);
    m_allFiles.clear();
    if (m_filterClass != HostFiles && !m_location.home().isEmpty()
        && folder() == m_location.home()) {
        for (const auto &device : m_location.deviceEntries())
            m_allFiles.push_back({ device.name, device.path, {}, true, true });
    }
    if (!folder().isEmpty()) {
        QDirIterator it(folder(), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        while (it.hasNext()) {
            it.next();
            const auto &info = it.fileInfo();
            if (!within(info.canonicalFilePath(), m_location.root()))
                continue;
            const auto name = info.fileName();
            if (m_filterClass != HostFiles
                && QFileInfo(name).suffix().compare(u"pok"_s, Qt::CaseInsensitive) == 0)
                continue;
            const bool dir = info.isDir();
            if (!dir && !canOpen(info.absoluteFilePath()))
                continue;
            m_allFiles.push_back({ name, info.absoluteFilePath(), info.lastModified(), dir, false,
                                   dir ? 0 : info.size() });
        }
    }
    sortAndRefilter();
}

void FolderListModel::sortAndRefilter()
{
    std::stable_sort(m_allFiles.begin(), m_allFiles.end(), [this](const Entry &a, const Entry &b) {
        if (a.device != b.device)
            return a.device;
        if (m_showDirsFirst && a.dir != b.dir)
            return a.dir;
        if (m_sortCriteria == ByDateDesc && a.modified != b.modified)
            return a.modified > b.modified;
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });
    refilter();
}

void FolderListModel::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void FolderListModel::refilter()
{
    const QString selected = m_search.isEmpty() ? QSettings().value(selectionKey()).toString()
                                                : path(m_currentIndex);
    m_refreshing = true;
    beginResetModel();
    m_files.clear();
    for (const auto &entry : m_allFiles)
        if (entry.name.contains(m_search, Qt::CaseInsensitive))
            m_files.push_back(entry);
    m_currentIndex = m_files.empty() ? -1 : 0;
    for (std::size_t i = 0; i < m_files.size(); ++i)
        if (m_files[i].path == selected) {
            m_currentIndex = int(i);
            break;
        }
    endResetModel();
    m_refreshing = false;
    emit currentIndexChanged(m_currentIndex);
}
