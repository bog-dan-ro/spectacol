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
#include "zxemulator.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJniObject>
#include <QMetaObject>
#include <QtCore/private/qjnihelpers_p.h>

using namespace Qt::StringLiterals;
namespace AndroidIntegration {
namespace {
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
                this,
                [url] {
                    auto &emulator = ZxEmulator::instance();
                    const auto path = emulator.importFile(url);
                    if (!path.isEmpty())
                        emulator.load(path);
                },
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
    const auto uriString = QJniObject::fromString(url.toString());
    const auto uri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse",
                                                        "(Ljava/lang/String;)Landroid/net/Uri;",
                                                        uriString.object<jstring>());
    const auto resolver =
            QJniObject(QNativeInterface::QAndroidApplication::context())
                    .callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
    auto cursor = resolver.callObjectMethod(
            "query",
            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/"
            "String;)Landroid/database/Cursor;",
            uri.object(), nullptr, nullptr, nullptr, nullptr);
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
} // namespace AndroidIntegration
