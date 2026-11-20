/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

#include "storagelocation.h"
#include "contenturi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {
bool everything(const QString &root)
{
    return root.isEmpty() || root == u"/"_s;
}

bool within(const QString &folder, const QString &root)
{
    return everything(root) || folder == root || folder.startsWith(root + u'/');
}
} // namespace

QString StorageLocation::canonical(const QString &path)
{
    if (ContentUri::isContent(path))
        return ContentUri::canonical(path);
    QString local = path;
    const QUrl url(path);
    if (url.isLocalFile())
        local = url.toLocalFile();
    const QFileInfo info(local);
    const auto resolved = info.canonicalFilePath();
    return resolved.isEmpty() ? info.absoluteFilePath() : resolved;
}

QString StorageLocation::deviceName(const QString &tree)
{
#ifdef Q_OS_ANDROID
    // The provider's name for it: one query
    if (const auto name = QFileInfo(tree).fileName(); !name.isEmpty())
        return name;
#endif
    const auto name = ContentUri::name(tree);
    return name.isEmpty() ? QCoreApplication::translate("StorageLocation", "Device folder") : name;
}

void StorageLocation::setRoot(const QString &root)
{
    m_root = root.isEmpty() ? QString() : canonical(root);
}

void StorageLocation::setHome(const QString &home)
{
    m_home = home.isEmpty() ? QString() : canonical(home);
}

void StorageLocation::setDeviceFolders(const QStringList &trees)
{
    m_deviceFolders = trees;
    // Forgotten while we were in it
    if (inDevice() && !granted(m_folder))
        setFolder(exit());
}

bool StorageLocation::granted(const QString &folder) const
{
    return std::ranges::any_of(m_deviceFolders, [&](const QString &tree) {
        return ContentUri::sameTree(tree, folder);
    });
}

bool StorageLocation::allowed(const QString &folder) const
{
    if (ContentUri::isContent(folder))
        return granted(folder);
    const QFileInfo info(folder);
    return within(folder, m_root) && info.isDir() && info.isReadable();
}

QString StorageLocation::fallback() const
{
    if (!everything(m_root))
        return m_root;
    if (!m_home.isEmpty())
        return m_home;
    return canonical(QDir::homePath());
}

bool StorageLocation::setFolder(const QString &folder, const QString &name)
{
    auto target = folder.isEmpty() ? fallback() : canonical(folder);
    if (!allowed(target))
        target = fallback();
    if (target == m_folder)
        return false;
    enter(target, name);
    m_folder = target;
    return true;
}

// Keeps the way down a device folder, for Up and for the title
void StorageLocation::enter(const QString &folder, const QString &name)
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
    const auto folderName = name.isEmpty() ? ContentUri::name(folder) : name;
    if (!m_trail.empty() && ContentUri::sameTree(m_trail.back().path, folder)) {
        m_trail.push_back({ folder, folderName });
        return;
    }
    if (!ContentUri::isContent(m_folder))
        m_exit = m_folder;
    // Straight into a folder below the device folder (the browser remembers the last one):
    // storage document ids give the way down; opaque ones only the folder itself.
    m_trail.clear();
    for (auto step = folder; !step.isEmpty(); step = ContentUri::parentFolder(step)) {
        const auto stepName = step == folder         ? folderName
                : step == ContentUri::treeRoot(step) ? deviceName(step)
                                                     : ContentUri::name(step);
        m_trail.insert(m_trail.begin(), { step, stepName });
    }
}

bool StorageLocation::goUp()
{
    if (inDevice()) {
        if (m_trail.size() > 1)
            return setFolder(m_trail[m_trail.size() - 2].path);
        // Entered below the device folder's root: storage document ids are paths
        if (const auto parent = ContentUri::parentFolder(m_folder); !parent.isEmpty()) {
            m_trail.clear();
            return setFolder(parent);
        }
        return setFolder(exit());
    }
    if (atTop())
        return false;
    QDir dir(m_folder);
    if (!dir.cdUp() || !allowed(dir.absolutePath()))
        return false;
    return setFolder(dir.absolutePath());
}

QString StorageLocation::exit() const
{
    if (!m_exit.isEmpty() && allowed(m_exit))
        return m_exit;
    return !m_home.isEmpty() ? m_home : fallback();
}

bool StorageLocation::inDevice() const
{
    return ContentUri::isContent(m_folder);
}

bool StorageLocation::inside(const QString &folder) const
{
    if (!inDevice())
        return m_folder == folder || m_folder.startsWith(folder + u'/');
    return std::ranges::any_of(m_trail, [&](const Step &step) { return step.path == folder; });
}

bool StorageLocation::atTop() const
{
    if (inDevice())
        return false;
    return QDir(m_folder).isRoot() || (!everything(m_root) && m_folder == m_root);
}

QString StorageLocation::title() const
{
    if (!inDevice())
        return m_folder;
    QStringList names;
    for (const auto &step : m_trail)
        names.append(step.name);
    if (names.isEmpty())
        names.append(ContentUri::name(m_folder));
    return names.join(u'/');
}

QString StorageLocation::name() const
{
    if (!inDevice())
        return QFileInfo(m_folder).fileName();
    return m_trail.empty() ? ContentUri::name(m_folder) : m_trail.back().name;
}

std::vector<StorageFiles::Entry> StorageLocation::deviceEntries() const
{
    std::vector<StorageFiles::Entry> entries;
    for (const auto &tree : m_deviceFolders)
        entries.push_back({ deviceName(tree), ContentUri::treeRoot(tree), {}, -1, true });
    return entries;
}
