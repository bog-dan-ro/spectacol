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

#include "androidintegration.h"
#include "contenturi.h"
#include "zxemulator.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJniArray>
#include <QJniEnvironment>
#include <QJniObject>
#include <QMetaObject>
#include <QtCore/private/qandroidextras_p.h>
#include <QtCore/private/qjnihelpers_p.h>

#include <unistd.h>

using namespace Qt::StringLiterals;
namespace AndroidIntegration {
namespace {
// android.content.Intent and android.app.Activity constants
constexpr jint grantRead = 0x1;
constexpr jint grantWrite = 0x2;
constexpr jint resultOk = -1;
constexpr int pickFolderRequest = 0x5350;

QJniObject context()
{
    return QJniObject(QNativeInterface::QAndroidApplication::context());
}

QJniObject contentResolver()
{
    return context().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
}

QJniObject parseUri(const QString &uri)
{
    return QJniObject::callStaticObjectMethod("android/net/Uri", "parse",
                                              "(Ljava/lang/String;)Landroid/net/Uri;",
                                              QJniObject::fromString(uri).object<jstring>());
}

QJniObject folderIntent()
{
    return QJniObject("android/content/Intent", "(Ljava/lang/String;)V",
                      QJniObject::fromString(u"android.intent.action.OPEN_DOCUMENT_TREE"_s)
                              .object<jstring>());
}

// The UriPermission entries Android keeps for us
template <typename Fn> void forEachGrant(Fn &&fn)
{
    const auto list =
            contentResolver().callObjectMethod("getPersistedUriPermissions", "()Ljava/util/List;");
    if (!list.isValid())
        return;
    const auto count = list.callMethod<jint>("size");
    for (jint i = 0; i < count; ++i) {
        const auto grant = list.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
        const auto uri = grant.callObjectMethod("getUri", "()Landroid/net/Uri;");
        jint flags = 0;
        if (grant.callMethod<jboolean>("isReadPermission"))
            flags |= grantRead;
        if (grant.callMethod<jboolean>("isWritePermission"))
            flags |= grantWrite;
        fn(uri, uri.toString(), flags);
    }
}
class IntentHandler final : public QObject, public QtAndroidPrivate::NewIntentListener
{
public:
    explicit IntentHandler(QObject *parent) : QObject(parent)
    {
        QtAndroidPrivate::registerNewIntentListener(this);
        handle(QJniObject(QNativeInterface::QAndroidApplication::context())
                       .callObjectMethod("getIntent", "()Landroid/content/Intent;"));
    }
    ~IntentHandler() override
    {
        QtAndroidPrivate::unregisterNewIntentListener(this);
    }
    bool handleNewIntent(JNIEnv *, jobject intent) override
    {
        return handle(QJniObject(intent));
    }

private:
    bool handle(const QJniObject &intent)
    {
        if (!intent.isValid()
            || intent.callObjectMethod("getAction", "()Ljava/lang/String;").toString()
                    != u"android.intent.action.VIEW"_s)
            return false;
        const QUrl url(intent.callObjectMethod("getDataString", "()Ljava/lang/String;").toString());
        if (url.scheme() != u"content"_s && !url.isLocalFile())
            return false;
        // Android delivers new intents on its Java UI thread, not Qt's GUI thread.
        QMetaObject::invokeMethod(
                this, [url] { ZxEmulator::instance().openExternalFile(url); },
                Qt::QueuedConnection);
        return true;
    }
};
} // namespace
void install(QObject *parent)
{
    new IntentHandler(parent);
}

QString displayName(const QUrl &url)
{
    if (url.scheme() != u"content"_s)
        return QFileInfo(url.isLocalFile() ? url.toLocalFile() : url.toString()).fileName();
    const auto uri = parseUri(url.toString(QUrl::FullyEncoded));
    const auto columns = QJniArray<jstring>::fromContainer(QStringList{ u"_display_name"_s });
    auto cursor = contentResolver().callObjectMethod(
            "query",
            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/"
            "String;)Landroid/database/Cursor;",
            uri.object(), columns.object<jobjectArray>(), nullptr, nullptr, nullptr);
    QString name;
    if (cursor.isValid()) {
        const auto columnName = QJniObject::fromString(u"_display_name"_s);
        const auto column = cursor.callMethod<jint>("getColumnIndex", "(Ljava/lang/String;)I",
                                                    columnName.object<jstring>());
        if (column >= 0 && cursor.callMethod<jboolean>("moveToFirst"))
            name = cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", column).toString();
        cursor.callMethod<void>("close");
    }
    return name.isEmpty() ? QFileInfo(url.path()).fileName() : name;
}

bool openDocument(const QUrl &url, QFile &file)
{
    if (url.isLocalFile() || url.scheme() != u"content"_s) {
        file.setFileName(url.isLocalFile() ? url.toLocalFile() : url.toString());
        return file.open(QIODevice::ReadOnly);
    }
    // The URI exactly as Android sent it: its grant is for that string
    const auto descriptor = contentResolver().callObjectMethod(
            "openFileDescriptor",
            "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
            parseUri(url.toString(QUrl::FullyEncoded)).object(),
            QJniObject::fromString(u"r"_s).object<jstring>());
    QJniEnvironment env;
    if (!env.checkAndClearExceptions(QJniEnvironment::OutputMode::Verbose)
        && descriptor.isValid()) {
        const int fd = descriptor.callMethod<jint>("detachFd");
        if (fd >= 0) {
            if (file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle))
                return true;
            ::close(fd);
        }
    }
    // Qt's own content engine handles the providers it knows
    file.setFileName(url.toString());
    return file.open(QIODevice::ReadOnly);
}

std::vector<Document> listFolder(const QString &folder)
{
    const auto uri = parseUri(ContentUri::canonical(folder));
    const auto parentId = QJniObject::callStaticObjectMethod(
            "android/provider/DocumentsContract", "getDocumentId",
            "(Landroid/net/Uri;)Ljava/lang/String;", uri.object());
    const auto children = QJniObject::callStaticObjectMethod(
            "android/provider/DocumentsContract", "buildChildDocumentsUriUsingTree",
            "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;", uri.object(),
            parentId.object<jstring>());
    const auto columns = QJniArray<jstring>::fromContainer(QStringList{
            u"document_id"_s, u"_display_name"_s, u"mime_type"_s, u"last_modified"_s, u"_size"_s });
    const auto cursor = contentResolver().callObjectMethod(
            "query",
            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/"
            "String;)Landroid/database/Cursor;",
            children.object(), columns.object<jobjectArray>(), nullptr, nullptr, nullptr);
    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Verbose) || !cursor.isValid())
        return {};

    std::vector<Document> documents;
    const auto column = [&](const QString &name) {
        return cursor.callMethod<jint>("getColumnIndex", "(Ljava/lang/String;)I",
                                       QJniObject::fromString(name).object<jstring>());
    };
    const auto idColumn = column(u"document_id"_s);
    const auto nameColumn = column(u"_display_name"_s);
    const auto typeColumn = column(u"mime_type"_s);
    const auto modifiedColumn = column(u"last_modified"_s);
    const auto sizeColumn = column(u"_size"_s);
    if (idColumn >= 0 && nameColumn >= 0 && typeColumn >= 0) {
        while (cursor.callMethod<jboolean>("moveToNext")) {
            const auto id = cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", idColumn);
            const auto name =
                    cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", nameColumn)
                            .toString();
            const auto type =
                    cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", typeColumn)
                            .toString();
            const auto document = QJniObject::callStaticObjectMethod(
                    "android/provider/DocumentsContract", "buildDocumentUriUsingTree",
                    "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;", uri.object(),
                    id.object<jstring>());
            QDateTime modified;
            if (modifiedColumn >= 0
                && !cursor.callMethod<jboolean>("isNull", "(I)Z", modifiedColumn))
                modified = QDateTime::fromMSecsSinceEpoch(
                        cursor.callMethod<jlong>("getLong", "(I)J", modifiedColumn));
            qint64 size = 0;
            if (sizeColumn >= 0 && !cursor.callMethod<jboolean>("isNull", "(I)Z", sizeColumn))
                size = cursor.callMethod<jlong>("getLong", "(I)J", sizeColumn);
            documents.push_back({ name, document.toString(), modified,
                                  type == u"vnd.android.document/directory"_s, size });
        }
    }
    cursor.callMethod<void>("close");
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Verbose))
        return {};
    return documents;
}

bool canPickFolders()
{
    const auto manager = context().callObjectMethod("getPackageManager",
                                                    "()Landroid/content/pm/PackageManager;");
    // Needs the manifest's <queries> entry for OPEN_DOCUMENT_TREE
    return folderIntent()
            .callObjectMethod(
                    "resolveActivity",
                    "(Landroid/content/pm/PackageManager;)Landroid/content/ComponentName;",
                    manager.object())
            .isValid();
}

void pickFolder(const QString &initialDocument, std::function<void(const QString &tree)> done)
{
    auto intent = folderIntent();
    if (!initialDocument.isEmpty()) {
        const auto extra = QJniObject::fromString(u"android.provider.extra.INITIAL_URI"_s);
        intent.callObjectMethod(
                "putExtra", "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;",
                extra.object<jstring>(), parseUri(initialDocument).object());
    }
    QtAndroidPrivate::startActivity(
            intent, pickFolderRequest,
            [done = std::move(done)](int, int resultCode, const QJniObject &data) {
                // Android's UI thread: keep the grant while the result's flags are at hand
                QString tree;
                const auto uri = resultCode == resultOk && data.isValid()
                        ? data.callObjectMethod("getData", "()Landroid/net/Uri;")
                        : QJniObject();
                if (uri.isValid()) {
                    const jint flags = data.callMethod<jint>("getFlags") & (grantRead | grantWrite);
                    contentResolver().callMethod<void>("takePersistableUriPermission",
                                                       "(Landroid/net/Uri;I)V", uri.object(),
                                                       flags);
                    QJniEnvironment env;
                    if (env.checkAndClearExceptions())
                        qWarning() << "Android didn't keep the access to" << uri.toString();
                    tree = uri.toString();
                }
                QMetaObject::invokeMethod(qApp, [done, tree] { done(tree); }, Qt::QueuedConnection);
            });
}

void releaseFolder(const QString &tree)
{
    const auto root = ContentUri::canonical(tree);
    forEachGrant([&](const QJniObject &uri, const QString &text, jint flags) {
        if (ContentUri::canonical(text) != root || !flags)
            return;
        contentResolver().callMethod<void>("releasePersistableUriPermission",
                                           "(Landroid/net/Uri;I)V", uri.object(), flags);
        QJniEnvironment env;
        env.checkAndClearExceptions();
    });
}

QStringList grantedFolders()
{
    QStringList folders;
    forEachGrant([&](const QJniObject &, const QString &text, jint flags) {
        if (flags & grantRead)
            folders.append(text);
    });
    return folders;
}
} // namespace AndroidIntegration
