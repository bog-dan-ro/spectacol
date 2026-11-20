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

#include "storagefiles.h"

#include <QString>
#include <QStringList>

#include <vector>

// Where a file browser is, and where it may go: the folders inside the root (all of them for
// an empty root or "/"), and the device folders Android lets Spectacol use (content:// trees),
// which the home folder lists. Android's document ids don't always name their parent: Up in a
// device folder follows the way down, and leaves the device folder where it was entered from.
// Shared by the file browser and the file manager.
class StorageLocation
{
public:
    // The same folder always has the same string: canonical local paths, canonical URIs
    static QString canonical(const QString &path);
    // The device folder's name, as its provider calls it
    static QString deviceName(const QString &tree);

    const QString &folder() const
    {
        return m_folder;
    }
    const QString &root() const
    {
        return m_root;
    }
    void setRoot(const QString &root);
    const QString &home() const
    {
        return m_home;
    }
    void setHome(const QString &home);
    const QStringList &deviceFolders() const
    {
        return m_deviceFolders;
    }
    // Leaves a device folder that is no longer granted
    void setDeviceFolders(const QStringList &trees);

    bool granted(const QString &folder) const;
    bool allowed(const QString &folder) const;
    // Where a folder that isn't allowed goes: the root, else the home, else the user's home
    QString fallback() const;
    // Goes to folder, or to the fallback when it isn't allowed; name is the provider's name for
    // a device folder, when the listing knows it. False when it stays where it is.
    bool setFolder(const QString &folder, const QString &name = {});
    // To the folder above; false at the top, or when the one above can't be read
    bool goUp();
    // Where Up from a device folder's root goes
    QString exit() const;

    bool inDevice() const;
    // The folder shown is folder, or inside it
    bool inside(const QString &folder) const;
    bool atTop() const;
    // The way down a device folder ("Games/Sub"), else the path
    QString title() const;
    QString name() const;
    // The granted folders, as the home folder lists them
    std::vector<StorageFiles::Entry> deviceEntries() const;

private:
    struct Step
    {
        QString path;
        QString name;
    };
    void enter(const QString &folder, const QString &name);

    QString m_folder;
    QString m_root;
    QString m_home;
    QString m_exit; // the local folder a device folder was entered from
    QStringList m_deviceFolders;
    std::vector<Step> m_trail;
};
