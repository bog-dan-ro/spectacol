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

#include "storagelocation.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QQmlEngine>
#include <QStringList>

#include <cstdint>
#include <span>
#include <vector>

// A folder's games for Quick Open. Besides local folders it browses the device folders the user
// granted on Android (content:// trees): they are listed in the home folder, and Up leaves them.
class FolderListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged FINAL)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(
            QString homeFolder READ homeFolder WRITE setHomeFolder NOTIFY homeFolderChanged FINAL)
    Q_PROPERTY(QStringList deviceFolders READ deviceFolders WRITE setDeviceFolders NOTIFY
                       deviceFoldersChanged FINAL)
    Q_PROPERTY(QString folderTitle READ folderTitle NOTIFY folderChanged FINAL)
    Q_PROPERTY(QString folderName READ folderName NOTIFY folderChanged FINAL)
    Q_PROPERTY(bool inDeviceFolder READ inDeviceFolder NOTIFY folderChanged FINAL)
    // A device folder is being listed
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(FileSortCriteria sortCriteria READ sortCriteria WRITE setSortCriteria NOTIFY
                       sortCriteriaChanged)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY
                       showDirsFirstChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(
            FilterType filterClass READ filterClass WRITE setFilterClass NOTIFY filterClassChanged)

public:
    // CoverSource is the screen, TypeIcon what shows until it is there
    enum FileFields {
        Name = Qt::DisplayRole,
        Path = Qt::UserRole,
        IsDir,
        IsDevice,
        CoverSource,
        TypeIcon
    };
    Q_ENUM(FileFields)

    enum FileSortCriteria {
        ByName,
        ByDateDesc,
    };
    Q_ENUM(FileSortCriteria)

    enum FilterType { All, Tapes, Disks, HostFiles, Microdrives };
    Q_ENUM(FilterType)

public:
    FolderListModel(QObject *parent = nullptr);
    ~FolderListModel();

    QString search() const
    {
        return m_search;
    }
    void setSearch(const QString &search);
    QString folder() const
    {
        return m_location.folder();
    }
    void setFolder(QString folder);

    QString rootFolder() const
    {
        return m_location.root();
    }
    void setRootFolder(QString rootFolder);

    // Where the device folders are listed (the Spectacol folder)
    QString homeFolder() const
    {
        return m_location.home();
    }
    void setHomeFolder(QString homeFolder);
    QStringList deviceFolders() const
    {
        return m_location.deviceFolders();
    }
    void setDeviceFolders(QStringList deviceFolders);

    // Where we are, for people: the local path, or the device folder and the folders below it
    QString folderTitle() const;
    QString folderName() const;
    bool inDeviceFolder() const;
    bool busy() const
    {
        return m_busy;
    }

    FileSortCriteria sortCriteria() const;
    void setSortCriteria(FileSortCriteria sortCriteria);

    bool showDirsFirst() const;
    void setShowDirsFirst(bool showDirsFirst);

    int currentIndex() const;
    void setCurrentIndex(int currentIndex);

    FilterType filterClass() const;
    void setFilterClass(FilterType filterClass);

public slots:
    bool isDir(int index);
    // A device folder's entry in the home folder (not a folder inside one)
    bool isDeviceFolder(int index);
    QString path(int index);
    void cdUp();
    void rememberSelection(int index);
    void refresh();

signals:
    void searchChanged();
    void folderChanged(QString folder);
    void rootFolderChanged(QString rootFolder);
    void homeFolderChanged();
    void busyChanged();
    void deviceFoldersChanged();
    void sortCriteriaChanged(FolderListModel::FileSortCriteria sortCriteria);
    void showDirsFirstChanged(bool showDirsFirst);
    void currentIndexChanged(int currentIndex);
    void filterClassChanged(FolderListModel::FilterType filterClass);

public:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Entry
    {
        QString name;
        QString path;
        QDateTime modified;
        bool dir = false;
        bool device = false;
        qint64 size = 0;
    };
    bool canOpen(const QString &path);
    bool accepts(const QString &fileName, std::span<const uint8_t> header) const;
    // After the folder changed
    void moved();
    void sortAndRefilter();
    void setBusy(bool busy);
    void refilter();
    QString selectionKey() const;

private:
    std::vector<Entry> m_files, m_allFiles;
    StorageLocation m_location;
    QString m_listedFolder; // the device folder m_allFiles lists
    int m_listing = 0; // the generation of the listing
    bool m_busy = false;
    QString m_search;
    bool m_refreshing = false;
    FileSortCriteria m_sortCriteria = ByName;
    bool m_showDirsFirst = true;
    int m_currentIndex = -1;
    FilterType m_filterClass = All;
};
