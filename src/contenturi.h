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

#include <QString>
#include <QStringList>

// Android document-tree URIs, as plain strings: no Android calls, so they work (and are tested)
// on every platform. A folder the user granted is a tree, "content://AUTHORITY/tree/TREE", and
// everything inside it is "content://AUTHORITY/tree/TREE/document/DOCUMENT". Qt's content file
// engine lists those folders and opens their files; it can't go up a level.
namespace ContentUri {

bool isContent(const QString &path);

// The same folder or file always has the same string: "tree/T" becomes "tree/T/document/T"
// and the ids are encoded like Android's Uri.encode(). Other paths are returned unchanged.
QString canonical(const QString &uri);

// Both are inside the same granted tree.
bool sameTree(const QString &a, const QString &b);

// The tree's root folder, canonical.
QString treeRoot(const QString &uri);

// The folder above, while still inside the tree. Only for providers whose document ids are
// paths ("primary:Games/Sub", Android's own storage); empty at the root or for opaque ids.
QString parentFolder(const QString &uri);

// The last name in the document id ("Sub" for "primary:Games/Sub"), empty for opaque ids.
QString name(const QString &uri);

// Android's external storage document for a shared-storage path, e.g. "/storage/emulated/0/Spectacol"
// is "content://com.android.externalstorage.documents/document/primary%3ASpectacol"; empty for
// other paths. The document picker can start there.
QString documentForPath(const QString &path);

// "/storage/emulated/0/Spectacol" as people know it: "Internal storage/Spectacol".
QString displayPath(const QString &path);

} // namespace ContentUri
