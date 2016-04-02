#ifndef FOLDERLISTMODEL_H
#define FOLDERLISTMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QFileInfo>
#include <libspectrum.h>

class FolderListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(FileFields FileSortCriteria)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(FileSortCriteria sortCriteria READ sortCriteria WRITE setSortCriteria NOTIFY sortCriteriaChanged)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY showDirsFirstChanged)

public:
    enum FileFields {
        Name = Qt::DisplayRole,
        Path = Qt::UserRole,
        IsDir
    };

    enum FileSortCriteria {
        ByName,
        ByDateDesc,
    };

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
    libspectrum_init_t m_spectrumInit;
    FileSortCriteria m_sortCriteria = ByName;
    bool m_showDirsFirst = true;
};

#endif // FOLDERLISTMODEL_H
