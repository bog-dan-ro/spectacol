#ifndef FOLDERLISTMODEL_H
#define FOLDERLISTMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QFileInfo>
#include <libspectrum.h>

class FolderListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(FileFields)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged)


public:
    enum FileFields {
        Name = Qt::DisplayRole,
        Path = Qt::UserRole,
        IsDir
    };

public:
    FolderListModel(QObject *parent = nullptr);
    ~FolderListModel();

    QString folder() const { return m_folder; }
    void setFolder(QString folder);

    QString rootFolder() const { return m_rootFolder; }
    void setRootFolder(QString rootFolder);

public slots:
    bool isDir(int index);
    QString path(int index);
    void cdUp();

signals:
    void folderChanged(QString folder);
    void rootFolderChanged(QString rootFolder);

protected:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canOpen(const QString &path);
    void updateFiles();

private:
    std::vector<QFileInfo> m_files;
    QString m_folder;
    QString m_rootFolder;
    libspectrum_init_t m_spectrumInit;
};

#endif // FOLDERLISTMODEL_H
