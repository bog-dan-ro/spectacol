#ifndef ZXREQUESTS_H
#define ZXREQUESTS_H

#include <QObject>
#include <QPointer>

class ZXGamesModel;
class QNetworkReply;
class ZXRequest : public QObject
{
    Q_OBJECT
public:
    explicit ZXRequest(ZXGamesModel *parent);
    virtual ~ZXRequest();
    void abort();

protected:
    QPointer<QNetworkReply> m_reply;
    QByteArray m_data;
    ZXGamesModel *m_model;
    bool m_aborted = false;
};

class ZXSearchRequest : public ZXRequest
{
    Q_OBJECT
public:
    explicit ZXSearchRequest(ZXGamesModel *parent);
};

class ZXFetchRequest : public ZXRequest
{
public:
    explicit ZXFetchRequest(ZXGamesModel *parent, size_t start, size_t count);

private:
    size_t m_start;
    size_t m_count;
};

class ZXUpdateRequest : public ZXRequest
{
public:
    explicit ZXUpdateRequest(ZXGamesModel *parent, const QString &identifier, size_t row);

private:
    QString m_identifier;
    size_t m_row;
    QByteArray m_fileData;
};

#endif // ZXREQUESTS_H
