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

#include "controllerprofiles.h"
#include "controllerinput.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGamepadManager>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QRegularExpression>
#include <cmath>

using namespace Qt::StringLiterals;

namespace {
constexpr qsizetype maxDocumentBytes = 1024 * 1024;
constexpr int maxProfiles = 128;

bool validName(const QString &name)
{
    if (name.isEmpty() || name.size() > 64 || name != name.trimmed())
        return false;
    for (const auto ch : name)
        if (ch.isNull() || ch.category() == QChar::Other_Control)
            return false;
    return true;
}

bool integer(const QJsonValue &value, int low, int high)
{
    if (!value.isDouble())
        return false;
    const double number = value.toDouble();
    return std::isfinite(number) && number == std::floor(number) && number >= low && number <= high;
}

bool validButton(int button)
{
    using G = QGamepadManager;
    return (button >= G::ButtonA && button <= G::ButtonSelect) || button == G::ButtonL3
            || button == G::ButtonR3;
}

QString gameTitle(QString title)
{
    static const QRegularExpression timestamp(
            u"\\.\\d{4}-\\d{2}-\\d{2}_\\d{2}_\\d{2}_\\d{2}_\\d{3}$"_s);
    title.remove(timestamp);
    while (QStringList{ u"tap"_s, u"tzx"_s, u"pzx"_s, u"csw"_s, u"sna"_s, u"szx"_s, u"z80"_s,
                        u"nex"_s, u"rzx"_s, u"zip"_s }
                   .contains(QFileInfo(title).suffix().toLower()))
        title = QFileInfo(title).completeBaseName();
    return title.trimmed();
}
QString gameKey(const QString &title)
{
    return title.isEmpty()
            ? QString{}
            : QString::fromLatin1(
                      QCryptographicHash::hash(title.normalized(QString::NormalizationForm_C)
                                                       .toCaseFolded()
                                                       .toUtf8(),
                                               QCryptographicHash::Sha256)
                              .toHex());
}

QString localPath(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}
} // namespace

bool ControllerProfiles::validMapping(const QVariantMap &mapping)
{
    const auto json = QJsonObject::fromVariantMap(mapping);
    if (!integer(json.value(u"directionMode"_s), 0, 2) || !integer(json.value(u"joystick"_s), 0, 7)
        || !json.value(u"fastForwardToggle"_s).isBool() || !json.value(u"deadZone"_s).isDouble())
        return false;
    const double zone = json.value(u"deadZone"_s).toDouble();
    if (!std::isfinite(zone) || zone < 0 || zone > 0.95)
        return false;
    if (!json.value(u"directionKeys"_s).isArray())
        return false;
    const auto keys = json.value(u"directionKeys"_s).toArray();
    if (keys.size() != ControllerInput::DirectionCount)
        return false;
    for (const auto &key : keys)
        if (!integer(key, 0, Qt::Key_unknown) || !ControllerInput::isSpectrumKey(key.toInt()))
            return false;
    if (!json.value(u"actions"_s).isObject())
        return false;
    const auto actions = json.value(u"actions"_s).toObject();
    for (auto it = actions.begin(); it != actions.end(); ++it) {
        bool ok = false;
        const int button = it.key().toInt(&ok);
        if (!ok || QString::number(button) != it.key() || !validButton(button)
            || !integer(it.value(), 0, int(ControllerAction::FastForward)))
            return false;
        const int action = it.value().toInt();
        if (action != int(ControllerAction::Fire) && !ControllerInput::isSpectrumKey(action)
            && (action < int(ControllerAction::Keyboard)
                || action > int(ControllerAction::FastForward)))
            return false;
    }
    return true;
}

ControllerProfiles::ControllerProfiles(QObject *parent) : QObject(parent)
{
    const auto raw = QSettings().value(u"ControllerProfiles/document"_s).toByteArray();
    if (raw.isEmpty())
        return;
    if (raw.size() > maxDocumentBytes) {
        fail(tr("Saved controller profiles are too large"));
        return;
    }
    const auto document = QJsonDocument::fromJson(raw);
    const auto root = document.object();
    if (!document.isObject() || !integer(root.value(u"version"_s), 1, 1)
        || !root.value(u"profiles"_s).isObject() || !root.value(u"games"_s).isObject()
        || root.value(u"profiles"_s).toObject().size() > maxProfiles) {
        fail(tr("Saved controller profiles are invalid"));
        return;
    }
    const auto profiles = root.value(u"profiles"_s).toObject();
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        if (!validName(it.key()) || !it.value().isObject()
            || !validMapping(it.value().toObject().toVariantMap())) {
            fail(tr("Saved controller profiles are invalid"));
            return;
        }
    }
    m_profiles = profiles;
    const auto games = root.value(u"games"_s).toObject();
    for (auto it = games.begin(); it != games.end(); ++it)
        if (it.key().size() == 64 && it.value().isString()
            && m_profiles.contains(it.value().toString()))
            m_games.insert(it.key(), it.value());
}

QStringList ControllerProfiles::names() const
{
    auto result = m_profiles.keys();
    result.sort(Qt::CaseInsensitive);
    return result;
}

void ControllerProfiles::setGame(const QString &title)
{
    m_gameName = gameTitle(title);
    m_gameKey = gameKey(m_gameName);
    emit gameChanged();
}

bool ControllerProfiles::rememberGame(const QString &title, const QString &profile)
{
    const auto key = gameKey(gameTitle(title));
    if (key.isEmpty() || !m_profiles.contains(profile))
        return fail(tr("Invalid game or profile"));
    if (m_games.value(key).toString() == profile)
        return true;
    auto games = m_games;
    games.insert(key, profile);
    return store(m_profiles, games);
}

QString ControllerProfiles::gameProfile() const
{
    return m_games.value(m_gameKey).toString();
}
QVariantMap ControllerProfiles::mapping(const QString &name) const
{
    return m_profiles.value(name).toObject().toVariantMap();
}

bool ControllerProfiles::fail(const QString &message)
{
    m_error = message;
    emit errorChanged();
    emit failed(message);
    return false;
}
void ControllerProfiles::clearError()
{
    if (m_error.isEmpty())
        return;
    m_error.clear();
    emit errorChanged();
}

bool ControllerProfiles::store(const QJsonObject &profiles, const QJsonObject &games)
{
    const auto data = QJsonDocument(QJsonObject{ { u"version"_s, 1 },
                                                 { u"profiles"_s, profiles },
                                                 { u"games"_s, games } })
                              .toJson(QJsonDocument::Compact);
    if (data.size() > maxDocumentBytes)
        return fail(tr("Controller profile storage is full"));
    QSettings settings;
    const auto old = settings.value(u"ControllerProfiles/document"_s);
    settings.setValue(u"ControllerProfiles/document"_s, data);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (old.isValid())
            settings.setValue(u"ControllerProfiles/document"_s, old);
        else
            settings.remove(u"ControllerProfiles/document"_s);
        return fail(tr("Can't save controller profiles"));
    }
    m_profiles = profiles;
    m_games = games;
    clearError();
    emit profilesChanged();
    emit gameChanged();
    return true;
}

bool ControllerProfiles::save(const QString &name, const QVariantMap &mapping)
{
    if (!validName(name) || !validMapping(mapping))
        return fail(tr("Invalid profile name or controller mapping"));
    if (!m_profiles.contains(name) && m_profiles.size() >= maxProfiles)
        return fail(tr("Too many controller profiles"));
    auto profiles = m_profiles;
    profiles.insert(name, QJsonObject::fromVariantMap(mapping));
    return store(profiles, m_games);
}

bool ControllerProfiles::remove(const QString &name)
{
    if (!m_profiles.contains(name))
        return fail(tr("Controller profile does not exist"));
    const bool active = gameProfile() == name;
    auto profiles = m_profiles;
    auto games = m_games;
    profiles.remove(name);
    for (auto it = games.begin(); it != games.end();)
        if (it.value().toString() == name)
            it = games.erase(it);
        else
            ++it;
    if (!store(profiles, games))
        return false;
    if (active)
        emit activationRequested(QString{});
    return true;
}

bool ControllerProfiles::assignGame(const QString &name)
{
    if (!hasGame() || (!name.isEmpty() && !m_profiles.contains(name)))
        return fail(tr("Load a game and choose an existing profile first"));
    auto games = m_games;
    if (name.isEmpty())
        games.remove(m_gameKey);
    else
        games.insert(m_gameKey, name);
    if (!store(m_profiles, games))
        return false;
    emit activationRequested(name);
    return true;
}

QString ControllerProfiles::importProfile(const QUrl &url)
{
    QFile file(localPath(url));
    if (!file.open(QIODevice::ReadOnly)) {
        fail(tr("Can't open profile: %1").arg(file.errorString()));
        return {};
    }
    const auto raw = file.read(maxDocumentBytes + 1);
    if (file.error() != QFileDevice::NoError || raw.size() > maxDocumentBytes) {
        fail(tr("Profile is unreadable or too large"));
        return {};
    }
    const auto document = QJsonDocument::fromJson(raw);
    const auto root = document.object();
    const auto mapping = root.value(u"mapping"_s).toObject().toVariantMap();
    QString name = root.value(u"name"_s).toString();
    if (!document.isObject()
        || root.value(u"format"_s).toString() != u"spectacol-controller-profile"_s
        || !integer(root.value(u"version"_s), 1, 1) || !validName(name) || !validMapping(mapping)) {
        fail(tr("Unsupported or invalid controller profile"));
        return {};
    }
    const auto original = name;
    for (int suffix = 2; m_profiles.contains(name); ++suffix)
        name = original.left(54) + u" (%1)"_s.arg(suffix);
    return save(name, mapping) ? name : QString{};
}

bool ControllerProfiles::exportProfile(const QString &name, const QUrl &url)
{
    if (!m_profiles.contains(name))
        return fail(tr("Controller profile does not exist"));
    const auto data = QJsonDocument(QJsonObject{ { u"format"_s, u"spectacol-controller-profile"_s },
                                                 { u"version"_s, 1 },
                                                 { u"name"_s, name },
                                                 { u"mapping"_s, m_profiles.value(name) } })
                              .toJson();
    const auto path = localPath(url);
    if (!path.startsWith(u"content://"_s) && !QFileInfo(path).absoluteDir().mkpath(u"."_s))
        return fail(tr("Can't create the profile folder"));
    QSaveFile file(path);
    file.setDirectWriteFallback(path.startsWith(u"content://"_s));
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        return fail(tr("Can't export profile: %1").arg(file.errorString()));
    clearError();
    return true;
}
