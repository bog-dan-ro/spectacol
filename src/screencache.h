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

#include <QByteArray>
#include <QString>

#include <optional>

// A file's cover in image://spectrum/: "cover/SIZE/MODIFIED/NAME/PATH", the name and the path in
// base64url, so URLs keep content:// paths intact. The folder listings know the name, the size
// and the modification time (ms since the epoch): the image provider doesn't have to ask a
// storage provider for them.
struct ScreenCover
{
    QString path;
    QString name;
    qint64 size = 0;
    qint64 modified = 0;

    QString id() const;
    static std::optional<ScreenCover> fromId(const QString &id);
};

// The screens media files show, kept on disk as compressed 6912-byte .scr data, so a file that
// a slow storage provider serves is read once. An entry is found by the file's path, size and
// modification time: a changed file is read again. A file without a screen is kept too, as an
// empty entry. Entries are separate files: several threads can use one folder.
class ScreenCache
{
public:
    explicit ScreenCache(QString folder);

    // The "screens" folder in the application's cache location
    static QString defaultFolder();

    // The screen: nullopt when the file isn't known, an empty array when it has no screen
    std::optional<QByteArray> find(const QString &path, qint64 size, qint64 modified) const;
    // Keeps screen (6912 bytes, or empty for none); false when it couldn't be written
    bool store(const QString &path, qint64 size, qint64 modified, const QByteArray &screen) const;
    // Removes the entries used least recently until the rest fit in maxBytes of disk
    void prune(qint64 maxBytes) const;

private:
    QString entryPath(const QString &path, qint64 size, qint64 modified) const;

    QString m_folder;
};
