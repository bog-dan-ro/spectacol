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
#include <QStringList>
#include <QUrl>

#include <functional>
#include <span>
#include <vector>

class QFile;
class QObject;
namespace AndroidIntegration {
void install(QObject *parent);
QString displayName(const QUrl &url);
// Read the provider's descriptor directly, including providers Qt's content engine can't open.
bool openDocument(const QUrl &url, QFile &file);

struct Document
{
    QString name;
    QString path;
    QDateTime modified;
    bool directory = false;
    qint64 size = 0;
};
// Read names, document URIs and metadata in one provider query, without opening the files.
std::vector<Document> listFolder(const QString &folder);
// Opens a document to replace its content. Not every provider truncates it: resize() it to what
// was written, before closing.
bool openDocumentForWriting(const QString &uri, QFile &file);
// Replaces a document's content
bool writeDocument(const QString &uri, std::span<const uint8_t> bytes);
// A new document in a granted folder: a file, or a folder with the mime type
// "vnd.android.document/directory". Its URI, empty when the provider refused.
QString createDocument(const QString &folder, const QString &mimeType, const QString &name);
// The document's URI after renaming it, empty when the provider refused
QString renameDocument(const QString &uri, const QString &name);
// Removes a document; a folder goes with everything in it
bool deleteDocument(const QString &uri);

// Whether this device has Android's folder picker (some TVs don't).
bool canPickFolders();
// Opens Android's folder picker, at initialDocument when it is set (a document URI). The access
// the user grants is kept across restarts. done runs on the GUI thread with the folder's tree URI,
// or with an empty string when the user cancels.
void pickFolder(const QString &initialDocument, std::function<void(const QString &tree)> done);
// Gives back the access kept for a folder.
void releaseFolder(const QString &tree);
// The folders whose access Android keeps for us.
QStringList grantedFolders();
} // namespace AndroidIntegration
