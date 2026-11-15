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
#include "contenturi.h"

#include <zxkit/media/MediaPreview.h>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {
QString canonicalFolder(QString path)
{
    if (ContentUri::isContent(path))
        return ContentUri::canonical(path);
    const QUrl url(path);
    if (url.isLocalFile())
        path = url.toLocalFile();
    const QFileInfo info(path);
    const auto canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}
bool within(const QString &folder, const QString &root)
{
    return root.isEmpty() || root == u"/"_s || folder == root || folder.startsWith(root + u'/');
}
} // namespace

FolderListModel::FolderListModel(QObject *parent) : QAbstractListModel(parent) { }

FolderListModel::~FolderListModel() { }

bool FolderListModel::isGranted(const QString &folder) const
{
    return std::ranges::any_of(m_deviceFolders, [&](const QString &tree) {
        return ContentUri::sameTree(tree, folder);
    });
}

bool FolderListModel::allowed(const QString &folder) const
{
    if (ContentUri::isContent(folder))
        return isGranted(folder);
    return within(folder, m_rootFolder) && QFileInfo(folder).isDir();
}

void FolderListModel::setFolder(QString folder)
{
    folder = canonicalFolder(folder);
    if (!allowed(folder))
        folder = m_rootFolder;
    if (m_folder == folder)
        return;
    enter(folder);
    m_folder = folder;
    if (!m_search.isEmpty()) {
        m_search.clear();
        emit searchChanged();
    }
    emit folderChanged(m_folder);
    refresh();
}

// Keeps the way down a device folder, for Up and for the title
void FolderListModel::enter(const QString &folder)
{
    if (!ContentUri::isContent(folder)) {
        m_trail.clear();
        return;
    }
    for (std::size_t i = 0; i < m_trail.size(); ++i) {
        if (m_trail[i].path == folder) {
            m_trail.resize(i + 1);
            return;
        }
    }
    // The row we open knows the provider's name for it
    QString name;
    for (const auto &entry : m_allFiles) {
        if (entry.path == folder) {
            name = entry.name;
            break;
        }
    }
    if (name.isEmpty())
        name = ContentUri::name(folder);
    if (!m_trail.empty() && ContentUri::sameTree(m_trail.back().path, folder)) {
        m_trail.push_back({ folder, name });
        return;
    }
    if (!ContentUri::isContent(m_folder))
        m_trailExit = m_folder;
    // Straight into a folder below the device folder (Quick Open remembers the last one):
    // storage document ids give the way down; opaque ones only the folder itself.
    m_trail.clear();
    for (auto step = folder; !step.isEmpty(); step = ContentUri::parentFolder(step)) {
        const auto stepName = step == folder         ? name
                : step == ContentUri::treeRoot(step) ? deviceName(step)
                                                     : ContentUri::name(step);
        m_trail.insert(m_trail.begin(), { step, stepName });
    }
}

void FolderListModel::setRootFolder(QString rootFolder)
{
    rootFolder = canonicalFolder(rootFolder);
    if (m_rootFolder == rootFolder)
        return;
    m_rootFolder = rootFolder;
    emit rootFolderChanged(m_rootFolder);
    if (m_folder.isEmpty() || !allowed(m_folder))
        setFolder(m_rootFolder);
    else
        refresh();
}

void FolderListModel::setHomeFolder(QString homeFolder)
{
    if (!homeFolder.isEmpty())
        homeFolder = canonicalFolder(homeFolder);
    if (m_homeFolder == homeFolder)
        return;
    const bool shown = !m_folder.isEmpty() && (m_folder == m_homeFolder || m_folder == homeFolder);
    m_homeFolder = homeFolder;
    emit homeFolderChanged();
    if (shown)
        refresh();
}

void FolderListModel::setDeviceFolders(QStringList deviceFolders)
{
    if (m_deviceFolders == deviceFolders)
        return;
    m_deviceFolders = std::move(deviceFolders);
    emit deviceFoldersChanged();
    if (inDeviceFolder() && !isGranted(m_folder)) {
        // Forgotten while we were in it
        setFolder(!m_trailExit.isEmpty()            ? m_trailExit
                          : !m_homeFolder.isEmpty() ? m_homeFolder
                                                    : m_rootFolder);
    } else if (!m_folder.isEmpty() && m_folder == m_homeFolder) {
        refresh();
    }
}

QString FolderListModel::folderTitle() const
{
    if (!inDeviceFolder())
        return m_folder;
    QStringList names;
    for (const auto &step : m_trail)
        names.append(step.name);
    if (names.isEmpty())
        names.append(ContentUri::name(m_folder));
    return names.join(u'/');
}

QString FolderListModel::folderName() const
{
    if (!inDeviceFolder())
        return QFileInfo(m_folder).fileName();
    return m_trail.empty() ? ContentUri::name(m_folder) : m_trail.back().name;
}

bool FolderListModel::inDeviceFolder() const
{
    return ContentUri::isContent(m_folder);
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
                            (m_folder + u'|' + QString::number(m_filterClass)).toUtf8(),
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
    if (inDeviceFolder()) {
        if (m_trail.size() > 1) {
            setFolder(m_trail[m_trail.size() - 2].path);
            return;
        }
        // Entered below the device folder's root: storage document ids are paths
        if (const auto parent = ContentUri::parentFolder(m_folder); !parent.isEmpty()) {
            m_trail.clear();
            setFolder(parent);
            return;
        }
        setFolder(!m_trailExit.isEmpty()            ? m_trailExit
                          : !m_homeFolder.isEmpty() ? m_homeFolder
                                                    : m_rootFolder);
        return;
    }
    QDir d(m_folder);
    d.cdUp();
    setFolder(d.absolutePath());
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
    }
    return {};
}

QHash<int, QByteArray> FolderListModel::roleNames() const
{
    static QHash<int, QByteArray> ret = {
        { Name, "name" }, { Path, "path" }, { IsDir, "isDir" }, { IsDevice, "isDevice" }
    };
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

QString FolderListModel::deviceName(const QString &tree) const
{
#ifdef Q_OS_ANDROID
    // The provider's name for it: one query
    if (const auto name = QFileInfo(tree).fileName(); !name.isEmpty())
        return name;
#endif
    const auto name = ContentUri::name(tree);
    return name.isEmpty() ? tr("Device folder") : name;
}

void FolderListModel::refresh()
{
    m_allFiles.clear();
    const bool content = inDeviceFolder();
    if (!content && m_filterClass != HostFiles && !m_homeFolder.isEmpty()
        && m_folder == m_homeFolder) {
        for (const auto &tree : m_deviceFolders)
            m_allFiles.push_back({ deviceName(tree), ContentUri::treeRoot(tree), {}, true, true });
    }
    if (!m_folder.isEmpty()) {
        QDirIterator it(m_folder, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        while (it.hasNext()) {
            it.next();
            const auto &info = it.fileInfo();
            if (!content && !within(info.canonicalFilePath(), m_rootFolder))
                continue;
            const auto name = content ? it.fileName() : info.fileName();
            if (m_filterClass != HostFiles
                && QFileInfo(name).suffix().compare(u"pok"_s, Qt::CaseInsensitive) == 0)
                continue;
            const bool dir = info.isDir();
            // Every document read is a round trip to its provider: go by the names there
            if (!dir && !(content ? accepts(name, {}) : canOpen(info.absoluteFilePath())))
                continue;
            m_allFiles.push_back(
                    { name,
                      content ? ContentUri::canonical(it.filePath()) : info.absoluteFilePath(),
                      m_sortCriteria == ByDateDesc ? info.lastModified() : QDateTime{}, dir,
                      false });
        }
    }
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
