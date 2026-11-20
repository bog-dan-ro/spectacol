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

#pragma once

#include <QDateTime>
#include <QString>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

// The files and folders on the host: local paths, or the documents (content:// URIs) of the
// device folders Android lets Spectacol use. Documents have no paths: a child is found by its
// name in its folder, and a new one gets the URI its provider chooses. Every call on a document
// asks its provider, which is slow: list the folders off the GUI thread.
namespace StorageFiles {

struct Entry
{
    QString name;
    QString path;
    QDateTime modified;
    qint64 size = -1; // -1 for folders
    bool dir = false;
};

bool isDocument(const QString &path);
// The folder's children, folders first, by name; empty when it can't be read
std::vector<Entry> list(const QString &folder);
// The child called name
std::optional<Entry> find(const QString &folder, const QString &name);

std::vector<uint8_t> read(const QString &path, qint64 limit, QString &error);
// Replaces an existing file's content. A local file is written whole or not at all; a local
// path that doesn't exist yet is created.
bool write(const QString &path, std::span<const uint8_t> bytes, QString &error);
// A new file called name in folder; its path, empty on failure
QString create(const QString &folder, const QString &name, std::span<const uint8_t> bytes,
               QString &error);
// A new folder called name in folder; its path, empty on failure
QString makeFolder(const QString &folder, const QString &name, QString &error);
// The path after renaming, empty on failure
QString rename(const QString &path, const QString &newName, QString &error);
// Copies a file into folder as name, replacing existing (a file there) when it isn't empty: in
// blocks, whatever its size. Its path, empty on failure.
QString copy(const QString &source, const QString &folder, const QString &name,
             const QString &existing, QString &error);
// Removes a file, or a folder with everything in it
bool remove(const QString &path, QString &error);
bool writable(const QString &folder);

} // namespace StorageFiles
