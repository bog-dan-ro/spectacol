#ifndef ZXGAMESMODEL_H
#define ZXGAMESMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QNetworkAccessManager>
#include <QUrl>

#include <memory>
#include <unordered_set>
#include <vector>

class ZXRequest;
class ZXGamesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(ZXRole)

public:
    enum ZXRole {
        Title = Qt::DisplayRole,
        Path = Qt::UserRole,
        ScreenFile
    };

    struct ZXData {
        ZXData(const QString &title, const QString &identifier)
            : title(title)
            , identifier(identifier)
        {}
        QString title;
        QString identifier;
        QString path;
    };

public:
    explicit ZXGamesModel(QObject *parent = 0);
    ~ZXGamesModel();

    const QString &searchString() const { return m_searchString; }
    void setSearchData(const std::vector<ZXData> &data, size_t total);
    void setFetchedData(const std::vector<ZXData> &data);
    void updateData(size_t row, const QString &path);
    QNetworkAccessManager &networkManager() { return m_netManager; }
    void requestFinished(ZXRequest* req);
    void updateFinished(const QString &identifier);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void search(const QString &title = QString(), const QString &firstTitle = QString());
    QString getPath(int index);

private:
    void abortAll();

private:
    size_t m_totalFound = 0;
    size_t m_fetchStart = 0;
    QString m_searchString;

    std::unordered_set<ZXRequest*> m_pendingRequests;
    mutable QHash<QString, ZXRequest*> m_pendingUpdates;

    std::vector<ZXData> m_data;

    QNetworkAccessManager m_netManager;
};

#endif // ZXGAMESMODEL_H
