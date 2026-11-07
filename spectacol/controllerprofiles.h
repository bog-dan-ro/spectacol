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

#include <QObject>
#include <QQmlEngine>
#include <QJsonObject>
#include <QUrl>
#include <QVariantMap>

class ControllerProfiles : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QStringList names READ names NOTIFY profilesChanged FINAL)
    Q_PROPERTY(QString gameName READ gameName NOTIFY gameChanged FINAL)
    Q_PROPERTY(QString gameProfile READ gameProfile WRITE assignGame NOTIFY gameChanged FINAL)
    Q_PROPERTY(bool hasGame READ hasGame NOTIFY gameChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged FINAL)
public:
    explicit ControllerProfiles(QObject *parent = nullptr);
    QStringList names() const;
    QString gameName() const
    {
        return m_gameName;
    }
    QString gameProfile() const;
    bool hasGame() const
    {
        return !m_gameKey.isEmpty();
    }
    QString lastError() const
    {
        return m_error;
    }
    void setGame(const QString &title);
    bool rememberGame(const QString &title, const QString &profile);
    QVariantMap mapping(const QString &name) const;
    bool save(const QString &name, const QVariantMap &mapping);
    Q_INVOKABLE bool remove(const QString &name);
    Q_INVOKABLE bool assignGame(const QString &name);
    Q_INVOKABLE QString importProfile(const QUrl &url);
    Q_INVOKABLE bool exportProfile(const QString &name, const QUrl &url);
    static bool validMapping(const QVariantMap &mapping);
signals:
    void profilesChanged();
    void gameChanged();
    void errorChanged();
    void failed(const QString &message);
    void activationRequested(const QString &name);

private:
    bool store(const QJsonObject &profiles, const QJsonObject &games);
    bool fail(const QString &message);
    void clearError();
    QJsonObject m_profiles;
    QJsonObject m_games;
    QString m_gameName;
    QString m_gameKey;
    QString m_error;
};
