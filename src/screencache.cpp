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

#include "screencache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryFile>
#include <QStandardPaths>

#include <algorithm>
#include <vector>

using namespace Qt::StringLiterals;

namespace {

constexpr qsizetype kScreenBytes = 6912;
// What a file takes on disk: whole blocks
constexpr qint64 kBlockBytes = 4096;
// A hit refreshes an entry's time at most this often, for the least recently used order
constexpr qint64 kTouchSeconds = 24 * 60 * 60;

qint64 diskBytes(qint64 size)
{
    return (size + kBlockBytes - 1) / kBlockBytes * kBlockBytes;
}

} // namespace

QString ScreenCover::id() const
{
    const auto encode = [](const QString &text) {
        return QString::fromLatin1(text.toUtf8().toBase64(QByteArray::Base64UrlEncoding
                                                          | QByteArray::OmitTrailingEquals));
    };
    return u"cover/%1/%2/%3/%4"_s.arg(size).arg(modified).arg(encode(name), encode(path));
}

std::optional<ScreenCover> ScreenCover::fromId(const QString &id)
{
    const auto parts = id.split(u'/');
    if (parts.size() != 5 || parts[0] != u"cover"_s)
        return std::nullopt;
    const auto decode = [](const QString &text) {
        const auto bytes = QByteArray::fromBase64Encoding(
                text.toLatin1(),
                QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        return bytes ? std::optional(QString::fromUtf8(*bytes)) : std::nullopt;
    };
    bool sizeOk = false;
    bool modifiedOk = false;
    ScreenCover cover;
    cover.size = parts[1].toLongLong(&sizeOk);
    cover.modified = parts[2].toLongLong(&modifiedOk);
    const auto name = decode(parts[3]);
    const auto path = decode(parts[4]);
    if (!sizeOk || !modifiedOk || !name || !path || path->isEmpty())
        return std::nullopt;
    cover.name = *name;
    cover.path = *path;
    return cover;
}

ScreenCache::ScreenCache(QString folder) : m_folder(std::move(folder)) { }

QString ScreenCache::defaultFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/screens"_s;
}

QString ScreenCache::entryPath(const QString &path, qint64 size, qint64 modified) const
{
    const auto key =
            path.toUtf8() + '\n' + QByteArray::number(size) + '\n' + QByteArray::number(modified);
    return m_folder + u'/'
            + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex())
            + u".scr"_s;
}

std::optional<QByteArray> ScreenCache::find(const QString &path, qint64 size, qint64 modified) const
{
    QFile file(entryPath(path, size, modified));
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    const auto compressed = file.readAll();
    QByteArray screen;
    if (!compressed.isEmpty()) {
        screen = qUncompress(compressed);
        if (screen.size() != kScreenBytes)
            return std::nullopt; // damaged: read the file again
    }
    const auto now = QDateTime::currentDateTime();
    if (file.fileTime(QFileDevice::FileModificationTime).secsTo(now) > kTouchSeconds) {
        file.close();
        if (file.open(QIODevice::ReadWrite))
            file.setFileTime(now, QFileDevice::FileModificationTime);
    }
    return screen;
}

bool ScreenCache::store(const QString &path, qint64 size, qint64 modified,
                        const QByteArray &screen) const
{
    if (!screen.isEmpty() && screen.size() != kScreenBytes)
        return false;
    if (!QDir().mkpath(m_folder))
        return false;
    // Written aside, then renamed: complete or absent. Not synced to the disk, a cache needn't
    // be, and find() doesn't trust an entry that doesn't uncompress to a screen.
    const auto entry = entryPath(path, size, modified);
    QTemporaryFile file(entry + u".XXXXXX"_s);
    if (!file.open())
        return false;
    const auto data = screen.isEmpty() ? QByteArray() : qCompress(screen, 9);
    if (file.write(data) != data.size() || !file.flush())
        return false;
    if (file.rename(entry)) {
        file.setAutoRemove(false); // it still owns the file, under its new name
        return true;
    }
    // Another thread may have kept the same screen meanwhile
    return QFile::exists(entry);
}

void ScreenCache::prune(qint64 maxBytes) const
{
    struct Entry
    {
        QString path;
        QDateTime used;
        qint64 bytes;
    };
    std::vector<Entry> entries;
    qint64 total = 0;
    // Every file: also what an interrupted store() left
    QDirIterator it(m_folder, QDir::Files);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        entries.push_back({ info.filePath(), info.lastModified(), diskBytes(info.size()) });
        total += entries.back().bytes;
    }
    if (total <= maxBytes)
        return;
    std::ranges::sort(entries, {}, &Entry::used);
    for (const auto &entry : entries) {
        if (total <= maxBytes)
            break;
        if (QFile::remove(entry.path))
            total -= entry.bytes;
    }
}
