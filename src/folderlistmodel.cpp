/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

#include <zxkit/media/MediaPreview.h>

#include <QDateTime>
#include <QDirIterator>
#include <QSet>
#include <QTimer>
#include <QSettings>
#include <QCryptographicHash>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace {
QString canonicalFolder(QString path)
{
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

void FolderListModel::setFolder(QString folder)
{
    folder = canonicalFolder(folder);
    if (!within(folder, m_rootFolder) || !QFileInfo(folder).isDir())
        folder = m_rootFolder;
    if (m_folder == folder)
        return;
    m_folder = folder;
    if (!m_search.isEmpty()) {
        m_search.clear();
        emit searchChanged();
    }
    emit folderChanged(m_folder);
    refresh();
}
void FolderListModel::setRootFolder(QString rootFolder)
{
    rootFolder = canonicalFolder(rootFolder);
    if (m_rootFolder == rootFolder)
        return;
    m_rootFolder = rootFolder;
    emit rootFolderChanged(m_rootFolder);
    if (!within(m_folder, m_rootFolder))
        setFolder(m_rootFolder);
    else
        refresh();
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
    return m_files[index].isDir();
}

QString FolderListModel::path(int index)
{
    if (size_t(index) >= m_files.size())
        return QString();
    return m_files[index].absoluteFilePath();
}

void FolderListModel::cdUp()
{
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

    switch (role) {
    case Name:
        return m_files[index.row()].fileName();
    case Path:
        return m_files[index.row()].absoluteFilePath();
    case IsDir:
        return m_files[index.row()].isDir();
    }
    return {};
}

QHash<int, QByteArray> FolderListModel::roleNames() const
{
    static QHash<int, QByteArray> ret = { { Name, "name" }, { Path, "path" }, { IsDir, "isDir" } };
    return ret;
}

bool FolderListModel::canOpen(const QString &path)
{
    if (m_filterClass == HostFiles)
        return true;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const auto buffer = f.read(64);
    const std::span<const uint8_t> span(reinterpret_cast<const uint8_t *>(buffer.constData()),
                                        size_t(buffer.size()));
    const auto fileClass = zxkit::classifyMediaFile(path.toStdString(), span);
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

void FolderListModel::refresh()
{
    m_allFiles.clear();
    QDirIterator it(m_folder, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    while (it.hasNext()) {
        it.next();
        const auto &info = it.fileInfo();
        if (!within(info.canonicalFilePath(), m_rootFolder))
            continue;
        if (m_filterClass != HostFiles && info.suffix().compare(u"pok"_s, Qt::CaseInsensitive) == 0)
            continue;
        if (info.isDir() || canOpen(info.absoluteFilePath()))
            m_allFiles.push_back(info);
    }
    std::sort(m_allFiles.begin(), m_allFiles.end(), [this](const QFileInfo &a, const QFileInfo &b) {
        if (m_showDirsFirst && a.isDir() != b.isDir())
            return a.isDir();
        if (m_sortCriteria == ByDateDesc && a.lastModified() != b.lastModified())
            return a.lastModified() > b.lastModified();
        return QString::compare(a.fileName(), b.fileName(), Qt::CaseInsensitive) < 0;
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
    for (const auto &info : m_allFiles)
        if (info.fileName().contains(m_search, Qt::CaseInsensitive))
            m_files.push_back(info);
    m_currentIndex = m_files.empty() ? -1 : 0;
    for (std::size_t i = 0; i < m_files.size(); ++i)
        if (m_files[i].absoluteFilePath() == selected) {
            m_currentIndex = int(i);
            break;
        }
    endResetModel();
    m_refreshing = false;
    emit currentIndexChanged(m_currentIndex);
}
