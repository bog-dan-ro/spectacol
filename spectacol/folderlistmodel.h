/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#include <QAbstractListModel>
#include <QFileInfo>
#include <QCache>
#include <QQmlEngine>

#include <vector>

extern "C"  {
# include <libspectrum.h>
}

class FolderListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(FileSortCriteria sortCriteria READ sortCriteria WRITE setSortCriteria NOTIFY sortCriteriaChanged)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY showDirsFirstChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(FilterType filterClass READ filterClass WRITE setFilterClass NOTIFY filterClassChanged)

public:
    enum FileFields {
        Name = Qt::DisplayRole,
        Path = Qt::UserRole,
        IsDir
    };
    Q_ENUM(FileFields)

    enum FileSortCriteria {
        ByName,
        ByDateDesc,
    };
    Q_ENUM(FileSortCriteria)

    enum FilterType {
        All,
        Tapes
    };
    Q_ENUM(FilterType)

public:
    FolderListModel(QObject *parent = nullptr);
    ~FolderListModel();

    QString folder() const { return m_folder; }
    void setFolder(QString folder);

    QString rootFolder() const { return m_rootFolder; }
    void setRootFolder(QString rootFolder);

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
    QString path(int index);
    void cdUp();
    void refresh();

signals:
    void folderChanged(QString folder);
    void rootFolderChanged(QString rootFolder);
    void sortCriteriaChanged(FileSortCriteria sortCriteria);
    void showDirsFirstChanged(bool showDirsFirst);
    void currentIndexChanged(int currentIndex);
    void filterClassChanged(FilterType filterClass);

protected:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canOpen(const QString &path);

private:
    std::vector<QFileInfo> m_files;
    QString m_folder;
    QString m_rootFolder;
    FileSortCriteria m_sortCriteria = ByName;
    bool m_showDirsFirst = true;
    int m_currentIndex = -1;
    FilterType m_filterClass = All;
};
