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

#include "storagefiles.h"
#include "contenturi.h"
#ifdef Q_OS_ANDROID
#include "androidintegration.h"
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUrl>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace StorageFiles {
namespace {

class Messages
{
    Q_DECLARE_TR_FUNCTIONS(StorageFiles)
};

#ifdef Q_OS_ANDROID
constexpr QStringView kFolderType = u"vnd.android.document/directory";
constexpr QStringView kFileType = u"application/octet-stream";
#endif

} // namespace

bool isDocument(const QString &path)
{
    return ContentUri::isContent(path);
}

std::vector<Entry> list(const QString &folder)
{
    std::vector<Entry> entries;
    if (isDocument(folder)) {
#ifdef Q_OS_ANDROID
        for (const auto &document : AndroidIntegration::listFolder(folder))
            entries.push_back({ document.name, ContentUri::canonical(document.path),
                                document.modified, document.directory ? -1 : document.size,
                                document.directory });
        // As QDir sorts the local ones
        std::ranges::sort(entries, [](const Entry &a, const Entry &b) {
            if (a.dir != b.dir)
                return a.dir;
            return a.name.localeAwareCompare(b.name) < 0;
        });
#endif
        return entries;
    }
    const auto infos = QDir(folder).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                  QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
                                                          | QDir::LocaleAware);
    for (const auto &info : infos)
        entries.push_back({ info.fileName(), info.absoluteFilePath(), info.lastModified(),
                            info.isDir() ? -1 : info.size(), info.isDir() });
    return entries;
}

std::optional<Entry> find(const QString &folder, const QString &name)
{
    if (!isDocument(folder)) {
        const QFileInfo info(QDir(folder).filePath(name));
        if (!info.exists())
            return std::nullopt;
        return Entry{ info.fileName(), info.absoluteFilePath(), info.lastModified(),
                      info.isDir() ? -1 : info.size(), info.isDir() };
    }
    for (auto &entry : list(folder))
        if (entry.name == name)
            return std::move(entry);
    return std::nullopt;
}

std::vector<uint8_t> read(const QString &path, qint64 limit, QString &error)
{
    QFile file;
    bool opened = false;
#ifdef Q_OS_ANDROID
    if (isDocument(path))
        opened = AndroidIntegration::openDocument(QUrl(path), file);
    else
#endif
    {
        file.setFileName(path);
        opened = file.open(QIODevice::ReadOnly);
    }
    if (!opened) {
        error = Messages::tr("Can't open '%1': %2").arg(path, file.errorString());
        return {};
    }
    const auto bytes = file.read(limit);
    return { bytes.begin(), bytes.end() };
}

bool write(const QString &path, std::span<const uint8_t> bytes, QString &error)
{
    if (isDocument(path)) {
#ifdef Q_OS_ANDROID
        if (AndroidIntegration::writeDocument(path, bytes))
            return true;
#endif
        error = Messages::tr("Can't save '%1'.").arg(path);
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(reinterpret_cast<const char *>(bytes.data()), qint64(bytes.size()))
                != qint64(bytes.size())
        || !file.commit()) {
        error = Messages::tr("Can't save '%1': %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

QString create(const QString &folder, const QString &name, std::span<const uint8_t> bytes,
               QString &error)
{
    if (!isDocument(folder)) {
        const auto path = QDir(folder).filePath(name);
        return write(path, bytes, error) ? path : QString();
    }
#ifdef Q_OS_ANDROID
    const auto document = AndroidIntegration::createDocument(folder, kFileType.toString(), name);
    if (!document.isEmpty()) {
        if (AndroidIntegration::writeDocument(document, bytes))
            return document;
        AndroidIntegration::deleteDocument(document);
    }
#endif
    error = Messages::tr("Can't save '%1'.").arg(name);
    return {};
}

QString makeFolder(const QString &folder, const QString &name, QString &error)
{
    if (!isDocument(folder)) {
        if (QDir(folder).mkdir(name))
            return QDir(folder).filePath(name);
    } else {
#ifdef Q_OS_ANDROID
        const auto document =
                AndroidIntegration::createDocument(folder, kFolderType.toString(), name);
        if (!document.isEmpty())
            return document;
#endif
    }
    error = Messages::tr("Can't create folder '%1'.").arg(name);
    return {};
}

QString rename(const QString &path, const QString &newName, QString &error)
{
    if (isDocument(path) && ContentUri::canonical(path) == ContentUri::treeRoot(path)) {
        error = Messages::tr("Can't rename the device folder itself.");
        return {};
    }
    if (!isDocument(path)) {
        const QFileInfo info(path);
        if (info.dir().rename(info.fileName(), newName))
            return info.dir().filePath(newName);
    } else {
#ifdef Q_OS_ANDROID
        if (const auto document = AndroidIntegration::renameDocument(path, newName);
            !document.isEmpty())
            return document;
#endif
    }
    error = Messages::tr("Can't rename '%1' to '%2'.").arg(path, newName);
    return {};
}

QString copy(const QString &source, const QString &folder, const QString &name,
             const QString &existing, QString &error)
{
    QFile input;
    bool opened = false;
#ifdef Q_OS_ANDROID
    if (isDocument(source))
        opened = AndroidIntegration::openDocument(QUrl(source), input);
    else
#endif
    {
        input.setFileName(source);
        opened = input.open(QIODevice::ReadOnly);
    }
    if (!opened) {
        error = Messages::tr("Can't open '%1': %2").arg(source, input.errorString());
        return {};
    }
    constexpr qint64 kBlock = 1024 * 1024;
    const auto pump = [&](QIODevice &output) {
        qint64 written = 0;
        while (true) {
            const auto block = input.read(kBlock);
            if (block.isEmpty())
                return input.error() == QFileDevice::NoError ? written : -1;
            if (output.write(block) != block.size())
                return qint64(-1);
            written += block.size();
        }
    };
    if (!isDocument(folder)) {
        const auto target = existing.isEmpty() ? QDir(folder).filePath(name) : existing;
        QSaveFile output(target);
        if (output.open(QIODevice::WriteOnly) && pump(output) >= 0 && output.commit())
            return target;
        error = Messages::tr("Can't save '%1': %2").arg(target, output.errorString());
        return {};
    }
#ifdef Q_OS_ANDROID
    auto target = existing;
    if (target.isEmpty())
        target = AndroidIntegration::createDocument(folder, kFileType.toString(), name);
    QFile output;
    if (!target.isEmpty() && AndroidIntegration::openDocumentForWriting(target, output)) {
        const auto written = pump(output);
        if (written >= 0 && output.resize(written) && output.flush())
            return target;
    }
    // Not half a new file
    if (existing.isEmpty() && !target.isEmpty())
        AndroidIntegration::deleteDocument(target);
#endif
    error = Messages::tr("Can't save '%1'.").arg(name);
    return {};
}

bool remove(const QString &path, QString &error)
{
    // The folder the user granted is theirs: never removed from here
    if (isDocument(path) && ContentUri::canonical(path) == ContentUri::treeRoot(path)) {
        error = Messages::tr("Can't remove the device folder itself.");
        return false;
    }
    bool removed = false;
    if (!isDocument(path)) {
        const QFileInfo info(path);
        removed = info.isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
    } else {
#ifdef Q_OS_ANDROID
        removed = AndroidIntegration::deleteDocument(path);
#endif
    }
    if (!removed)
        error = Messages::tr("Can't remove '%1'.").arg(path);
    return removed;
}

bool writable(const QString &folder)
{
    // A device folder is granted for writing too; its provider refuses what it can't do
    return isDocument(folder) || QFileInfo(folder).isWritable();
}

} // namespace StorageFiles
