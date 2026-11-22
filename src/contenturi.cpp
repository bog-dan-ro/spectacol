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

#include "contenturi.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>
#include <array>
#include <optional>

using namespace Qt::StringLiterals;

namespace ContentUri {
namespace {
const auto scheme = u"content://"_s;
const auto externalStorage = u"com.android.externalstorage.documents"_s;

struct Parts
{
    QString authority;
    QString tree;
    QString document;
};

QString decode(QStringView text)
{
    return QUrl::fromPercentEncoding(text.toUtf8());
}

// Android's Uri.encode() leaves letters, digits and "_-!.~'()*" alone
QString encode(const QString &id)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(id, "!'()*"));
}

std::optional<Parts> parse(const QString &uri)
{
    if (!isContent(uri))
        return {};
    const auto rest = QStringView(uri).mid(scheme.size());
    const auto slash = rest.indexOf(u'/');
    if (slash <= 0)
        return {};
    auto segments = rest.mid(slash + 1).split(u'/');
    if (!segments.isEmpty() && segments.last().isEmpty())
        segments.removeLast();
    if (segments.size() < 2 || segments[0] != u"tree"_s || segments[1].isEmpty())
        return {};
    Parts parts{ rest.left(slash).toString(), decode(segments[1]), {} };
    if (segments.size() == 2)
        parts.document = parts.tree;
    else if (segments.size() == 4 && segments[2] == u"document"_s && !segments[3].isEmpty())
        parts.document = decode(segments[3]);
    else
        return {}; // e.g. "tree/T/Sub/file" from QDir::filePath(): not a document
    return parts;
}

QString build(const Parts &parts)
{
    return scheme + parts.authority + u"/tree/"_s + encode(parts.tree) + u"/document/"_s
            + encode(parts.document);
}

// The names below the tree's root, when the ids are paths; nullopt when they aren't.
std::optional<QStringList> relative(const Parts &parts)
{
    if (parts.document == parts.tree)
        return QStringList{};
    if (!parts.document.startsWith(parts.tree))
        return {};
    auto rest = QStringView(parts.document).mid(parts.tree.size());
    // "primary:" is a whole volume, "primary:Games" one of its folders
    if (!parts.tree.endsWith(u':')) {
        if (!rest.startsWith(u'/'))
            return {};
        rest = rest.mid(1);
    }
    if (rest.isEmpty() || rest.endsWith(u'/'))
        return {};
    return rest.toString().split(u'/');
}

struct Volume
{
    QString id;
    QString rest;
};

std::optional<Volume> volumeOf(const QString &path)
{
    if (path.isEmpty() || !QDir::isAbsolutePath(path))
        return {};
    auto clean = QDir::cleanPath(path);
    static const std::array primaryRoots{ u"/sdcard"_s, u"/mnt/sdcard"_s, u"/storage/sdcard0"_s,
                                          u"/storage/self/primary"_s };
    for (const auto &root : primaryRoots) {
        if (clean == root || clean.startsWith(root + u'/'))
            return Volume{ u"primary"_s, clean.mid(root.size() + 1) };
    }
    static const QRegularExpression storage(
            u"^/storage/(emulated/\\d+|[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4})(/.*)?$"_s);
    const auto match = storage.match(clean);
    if (!match.hasMatch())
        return {};
    const auto volume = match.captured(1);
    return Volume{ volume.startsWith(u"emulated/"_s) ? u"primary"_s : volume.toUpper(),
                   match.captured(2).mid(1) };
}
} // namespace

bool isContent(const QString &path)
{
    return path.startsWith(scheme);
}

QString mediaFileName(const QString &name)
{
    static const QRegularExpression duplicate(
            uR"(^(.*)(\.(?:szx|sna|z80|nex|tap|tzx))\s+(\([0-9]+\))$)"_s,
            QRegularExpression::CaseInsensitiveOption);
    const auto match = duplicate.match(name);
    if (!match.hasMatch())
        return name;
    return match.captured(1) + u' ' + match.captured(3) + match.captured(2);
}

QString canonical(const QString &uri)
{
    const auto parts = parse(uri);
    return parts ? build(*parts) : uri;
}

bool sameTree(const QString &a, const QString &b)
{
    const auto first = parse(a);
    const auto second = parse(b);
    return first && second && first->authority == second->authority && first->tree == second->tree;
}

QString treeRoot(const QString &uri)
{
    auto parts = parse(uri);
    if (!parts)
        return {};
    parts->document = parts->tree;
    return build(*parts);
}

QString parentFolder(const QString &uri)
{
    auto parts = parse(uri);
    if (!parts)
        return {};
    const auto names = relative(*parts);
    if (!names || names->isEmpty())
        return {};
    if (names->size() == 1) {
        parts->document = parts->tree;
    } else {
        parts->document.truncate(parts->document.size() - names->last().size() - 1);
    }
    return build(*parts);
}

QString name(const QString &uri)
{
    const auto parts = parse(uri);
    if (!parts)
        return {};
    const auto &id = parts->document;
    const auto separator = std::max(id.lastIndexOf(u'/'), id.lastIndexOf(u':'));
    return id.mid(separator + 1);
}

QString documentForPath(const QString &path)
{
    const auto volume = volumeOf(path);
    if (!volume)
        return {};
    return scheme + externalStorage + u"/document/"_s + encode(volume->id + u':' + volume->rest);
}

QString displayPath(const QString &path)
{
    const auto volume = volumeOf(path);
    if (!volume)
        return QDir::cleanPath(path);
    const auto root = volume->id == u"primary"_s
            ? QCoreApplication::translate("ContentUri", "Internal storage")
            : volume->id;
    return volume->rest.isEmpty() ? root : root + u'/' + volume->rest;
}

} // namespace ContentUri
