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

#include "devicestorage.h"
#include "contenturi.h"
#ifdef Q_OS_ANDROID
#include "androidintegration.h"
#endif

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSaveFile>
#include <QSettings>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace {
const auto foldersKey = u"Android/deviceFolders"_s;
const auto legacyPathKey = u"Android/legacyDataPath"_s;
const auto legacyImportKey = u"Android/legacyImport"_s;
// ZxEmulator::ErrorLevel
enum Level { Info, Warning };
constexpr qint64 chunkSize = 256 * 1024;

bool sameContents(const QString &first, const QString &second)
{
    QFile a(first);
    QFile b(second);
    if (!a.open(QIODevice::ReadOnly) || !b.open(QIODevice::ReadOnly))
        return false;
    for (;;) {
        const auto left = a.read(chunkSize);
        const auto right = b.read(chunkSize);
        if (left != right)
            return false;
        if (left.isEmpty())
            return a.error() == QFileDevice::NoError && b.error() == QFileDevice::NoError;
    }
}

bool sameFile(const QString &source, qint64 size, const QString &target)
{
    const QFileInfo existing(target);
    return existing.isFile() && existing.size() == size && sameContents(source, target);
}

// A free numbered name for a file whose name is taken, or nothing when one of the numbered
// names already has the same file (copied by an earlier run)
std::optional<QString> numbered(const QString &path, const QString &source, qint64 size)
{
    const QFileInfo info(path);
    const auto folder = info.dir();
    const auto base = info.completeBaseName();
    const auto suffix = info.suffix().isEmpty() ? QString{} : u'.' + info.suffix();
    for (int n = 1;; ++n) {
        const auto candidate = folder.filePath(base + u'_' + QString::number(n) + suffix);
        if (!QFileInfo::exists(candidate))
            return candidate;
        if (sameFile(source, size, candidate))
            return std::nullopt;
    }
}

bool copyFile(const QString &source, const QFileInfo &info, const QString &target)
{
    QFile input(source);
    if (!input.open(QIODevice::ReadOnly))
        return false;
    QSaveFile output(target);
    if (!output.open(QIODevice::WriteOnly))
        return false;
    for (;;) {
        const auto bytes = input.read(chunkSize);
        if (input.error() != QFileDevice::NoError)
            return false; // QSaveFile discards what it wrote
        if (bytes.isEmpty())
            break;
        if (output.write(bytes) != bytes.size())
            return false;
    }
    if (!output.commit())
        return false;
    // Quick Load and the snapshot folders go by these times
    if (const auto modified = info.lastModified(); modified.isValid()) {
        QFile written(target);
        if (written.open(QIODevice::Append))
            written.setFileTime(modified, QFileDevice::FileModificationTime);
    }
    return true;
}

void copyFolder(const QString &source, const QString &destination, const QString &relative,
                const std::stop_token &stop, const DeviceStorage::Progress &progress,
                DeviceStorage::CopyResult &result)
{
    if (!QDir().mkpath(destination)) {
        result.failed.append(relative.isEmpty() ? destination : relative);
        return;
    }
    // A content:// folder's entries are documents: their paths come from the iterator, not
    // from joining names, and their parents can't be computed.
    QDirIterator it(source, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    while (it.hasNext()) {
        if (stop.stop_requested()) {
            result.stopped = true;
            return;
        }
        it.next();
        const auto name = it.fileName();
        if (name.isEmpty() || name.contains(u'/') || name == u"."_s || name == u".."_s)
            continue;
        const auto info = it.fileInfo();
        const auto shown = relative.isEmpty() ? name : relative + u'/' + name;
        auto target = QDir(destination).filePath(name);
        if (info.isDir()) {
            copyFolder(it.filePath(), target, shown, stop, progress, result);
            if (result.stopped)
                return;
            continue;
        }
        bool renamed = false;
        if (QFileInfo::exists(target)) {
            const auto free = sameFile(it.filePath(), info.size(), target)
                    ? std::nullopt
                    : numbered(target, it.filePath(), info.size());
            if (!free) {
                ++result.skipped;
                continue;
            }
            target = *free;
            renamed = true;
        }
        if (!copyFile(it.filePath(), info, target)) {
            result.failed.append(shown);
            continue;
        }
        ++result.copied;
        if (renamed)
            ++result.renamed;
        if (progress)
            progress(result.copied, shown);
    }
}
} // namespace

DeviceStorage::CopyResult DeviceStorage::copyTree(const QString &source, const QString &destination,
                                                  std::stop_token stop, const Progress &progress)
{
    CopyResult result;
    if (!QFileInfo(source).isDir()) {
        result.failed.append(source);
        return result;
    }
    copyFolder(source, destination, {}, stop, progress, result);
    return result;
}

DeviceStorage::DeviceStorage(QString dataPath, QObject *parent)
    : QObject(parent), m_dataPath(std::move(dataPath))
{
    QSettings settings;
    m_legacyPath = settings.value(legacyPathKey).toString();
    m_folders = settings.value(foldersKey).toStringList();
#ifdef Q_OS_ANDROID
    // Android drops a grant when its folder goes away or the user revokes it; a tree that
    // is no device folder is a leftover of an interrupted import.
    QStringList granted;
    for (const auto &tree : AndroidIntegration::grantedFolders()) {
        const auto root = ContentUri::canonical(tree);
        const bool kept = std::ranges::any_of(m_folders, [&](const QString &folder) {
            return ContentUri::canonical(folder) == root;
        });
        if (kept)
            granted.append(root);
        else if (tree.contains(u"/tree/"_s) && !tree.contains(u"/document/"_s))
            AndroidIntegration::releaseFolder(tree);
    }
    const auto count = m_folders.size();
    m_folders.removeIf([&](const QString &folder) {
        return !granted.contains(ContentUri::canonical(folder));
    });
    if (m_folders.size() != count)
        settings.setValue(foldersKey, m_folders);
#endif
    const auto state = settings.value(legacyImportKey).toString();
    if (!m_legacyPath.isEmpty() && state != u"done"_s && state != u"never"_s)
        QMetaObject::invokeMethod(this, &DeviceStorage::offerLegacyImport, Qt::QueuedConnection);
}

DeviceStorage::~DeviceStorage()
{
    m_worker.request_stop();
    if (m_worker.joinable())
        m_worker.join();
}

bool DeviceStorage::available() const
{
#ifdef Q_OS_ANDROID
    static const bool picker = AndroidIntegration::canPickFolders();
    return picker;
#else
    return false;
#endif
}

QStringList DeviceStorage::folders() const
{
    return m_folders;
}

QString DeviceStorage::legacyFolder() const
{
    return m_legacyPath.isEmpty() ? QString{} : ContentUri::displayPath(m_legacyPath);
}

bool DeviceStorage::importing() const
{
    return m_importing;
}

int DeviceStorage::importedFiles() const
{
    return m_importedFiles;
}

QString DeviceStorage::importSummary() const
{
    return m_importSummary;
}

void DeviceStorage::addFolder()
{
#ifdef Q_OS_ANDROID
    if (!available()) {
        emit message(tr("This device has no folder picker."), Warning);
        return;
    }
    AndroidIntegration::pickFolder({}, [self = QPointer(this)](const QString &tree) {
        if (!self || tree.isEmpty())
            return;
        auto folders = self->m_folders;
        const auto root = ContentUri::canonical(tree);
        if (!std::ranges::any_of(folders, [&](const QString &folder) {
                return ContentUri::canonical(folder) == root;
            })) {
            folders.append(tree);
        }
        self->setFolders(folders);
        emit self->folderAdded(root);
    });
#else
    emit message(tr("Spectacol browses this computer's folders directly."), Info);
#endif
}

void DeviceStorage::forgetFolder(const QString &path)
{
    const auto root = ContentUri::treeRoot(path);
    auto folders = m_folders;
    folders.removeIf([&](const QString &folder) {
        if (ContentUri::canonical(folder) != root)
            return false;
#ifdef Q_OS_ANDROID
        AndroidIntegration::releaseFolder(folder);
#endif
        return true;
    });
    setFolders(folders);
}

void DeviceStorage::importLegacyFolder()
{
    if (m_importing || m_legacyPath.isEmpty())
        return;
#ifdef Q_OS_ANDROID
    if (!available()) {
        emit message(tr("This device has no folder picker."), Warning);
        return;
    }
    // Android shows the old folder; the user still has to choose it
    AndroidIntegration::pickFolder(ContentUri::documentForPath(m_legacyPath),
                                   [self = QPointer(this)](const QString &tree) {
                                       if (self && !tree.isEmpty())
                                           self->startImport(tree);
                                   });
#else
    startImport(m_legacyPath);
#endif
}

void DeviceStorage::postponeLegacyImport(bool permanently)
{
    if (permanently)
        QSettings().setValue(legacyImportKey, u"never"_s);
}

void DeviceStorage::setFolders(QStringList folders)
{
    if (m_folders == folders)
        return;
    m_folders = std::move(folders);
    QSettings().setValue(foldersKey, m_folders);
    emit foldersChanged();
}

void DeviceStorage::startImport(const QString &tree)
{
    if (m_importing)
        return;
    m_importing = true;
    m_importedFiles = 0;
    m_importSummary.clear();
    emit importingChanged();
    emit importProgressChanged();
    m_worker = std::jthread([this, tree, destination = m_dataPath](std::stop_token stop) {
        QElapsedTimer shown;
        shown.start();
        const auto result = copyTree(tree, destination, stop, [&](int copied, const QString &) {
            m_importedFiles = copied;
            if (shown.elapsed() >= 100) {
                shown.restart();
                QMetaObject::invokeMethod(this, &DeviceStorage::importProgressChanged,
                                          Qt::QueuedConnection);
            }
        });
        QMetaObject::invokeMethod(
                this, [this, result, tree] { finishImport(result, tree); }, Qt::QueuedConnection);
    });
}

void DeviceStorage::finishImport(const CopyResult &result, const QString &tree)
{
    if (m_worker.joinable())
        m_worker.join();
    m_importing = false;
    m_importedFiles = result.copied;
    QStringList summary;
    if (result.stopped) {
        summary.append(tr("Copying stopped after %n file(s).", "", result.copied));
    } else {
        summary.append(tr("Copied %n file(s) into the Spectacol folder.", "", result.copied));
        QSettings().setValue(legacyImportKey, u"done"_s);
    }
    if (result.skipped)
        summary.append(tr("%n file(s) were already there.", "", result.skipped));
    if (result.renamed)
        summary.append(tr("%n file(s) got a numbered name, a different file had theirs.", "",
                          result.renamed));
    if (!result.failed.isEmpty()) {
        auto failed = result.failed.mid(0, 5).join(u", "_s);
        if (result.failed.size() > 5)
            failed += u", …"_s;
        summary.append(tr("%n file(s) could not be copied: %1", "", int(result.failed.size()))
                               .arg(failed));
    }
    m_importSummary = summary.join(u' ');
#ifdef Q_OS_ANDROID
    // The copy is done with the folder, unless it is also a device folder
    const auto root = ContentUri::canonical(tree);
    if (!std::ranges::any_of(m_folders, [&](const QString &folder) {
            return ContentUri::canonical(folder) == root;
        })) {
        AndroidIntegration::releaseFolder(tree);
    }
#else
    Q_UNUSED(tree);
#endif
    emit importingChanged();
    emit importProgressChanged();
    emit message(m_importSummary, result.failed.isEmpty() ? Info : Warning);
}
