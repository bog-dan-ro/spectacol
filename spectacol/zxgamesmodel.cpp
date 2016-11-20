#include "zxgamesmodel.h"
#include "zxrequests.h"

namespace {
    const QString zxSpectrumLibraryGames = QLatin1String("https://archive.org/advancedsearch.php?q=collection:(zx_spectrum_library_games)");
    const QString zxSpectrumSearchTitle = QLatin1String(" AND title:(%1)");
    const QLatin1String zxSpectrumSearchFirstTitle(" AND firstTitle:");
}

ZXGamesModel::ZXGamesModel(QObject *parent) : QAbstractListModel(parent)
{}

ZXGamesModel::~ZXGamesModel()
{
    abortAll();
}

int ZXGamesModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QVariant ZXGamesModel::data(const QModelIndex &index, int role) const
{
    if (size_t(index.row()) >= m_data.size())
        return QVariant();

    auto path = [this, &index] {
        const QString &path = m_data[index.row()].path;
        if (path.isEmpty()) {
            const QString &identifier = m_data[index.row()].identifier;
            if (!m_pendingUpdates.contains(identifier))
                m_pendingUpdates.insert(identifier, new ZXUpdateRequest(const_cast<ZXGamesModel*>(this), identifier, index.row()));
        }
        return path;
    };

    switch (role) {
    case Title:
        return m_data[index.row()].title;
    case Path:
        return path();
    case ScreenFile:
        return path();
    }
    return QVariant();
}

bool ZXGamesModel::canFetchMore(const QModelIndex &/*parent*/) const
{
    return m_totalFound > m_fetchStart;
}

void ZXGamesModel::fetchMore(const QModelIndex &/*parent*/)
{
    if (!m_fetchStart || m_fetchStart != m_data.size())
        return;

    auto fetchCount = std::min(size_t(50), m_totalFound - m_fetchStart);
    if (fetchCount)
        m_pendingRequests.insert(new ZXFetchRequest(this, m_fetchStart, fetchCount));
}

QHash<int, QByteArray> ZXGamesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Title] = "title";
    roles[Path] = "path";
    roles[ScreenFile] = "screenFile";
    return roles;

}

void ZXGamesModel::setSearchData(const std::vector<ZXGamesModel::ZXData> &data, size_t total)
{
    beginResetModel();
    m_data = std::move(data);
    m_totalFound = total;
    m_fetchStart = m_data.size();
    endResetModel();
    if (m_data.empty())
        emit noResults();
}

void ZXGamesModel::setFetchedData(const std::vector<ZXGamesModel::ZXData> &data)
{
    beginInsertRows(QModelIndex(), m_fetchStart, m_fetchStart + data.size() - 1);
    m_data.insert(m_data.end(), data.begin(), data.end());
    m_fetchStart = m_data.size();
    endInsertRows();
}

void ZXGamesModel::updateData(size_t row, const QString &path)
{
    if (row >= m_data.size())
        return;
    m_data[row].path = path;
    emit dataChanged(index(row), index(row));
}

void ZXGamesModel::requestFinished(ZXRequest *req)
{
    m_pendingRequests.erase(m_pendingRequests.find(req));
}

void ZXGamesModel::updateFinished(const QString &identifier)
{
    m_pendingUpdates.remove(identifier);
}

void ZXGamesModel::search(const QString &title, const QString &firstTitle)
{
    abortAll();

    m_totalFound = 0;
    m_fetchStart = 0;
    m_data.clear();

    m_searchString = QLatin1String("https://archive.org/advancedsearch.php?q=collection:(zx_spectrum_library_games)");
    if (!title.isEmpty())
        m_searchString += zxSpectrumSearchTitle.arg(title.toHtmlEscaped());

    if (firstTitle.size() == 1)
        m_searchString += zxSpectrumSearchFirstTitle + firstTitle;

    m_pendingRequests.insert(new ZXSearchRequest(this));
}

QString ZXGamesModel::getPath(int index)
{
    if (size_t(index) >= m_data.size())
        return QString();

    return m_data[index].path;
}

void ZXGamesModel::abortAll()
{
    for (auto req : m_pendingRequests)
        req->abort();

    for (auto it = m_pendingUpdates.cbegin(); it != m_pendingUpdates.cend(); ++it)
        it.value()->abort();
}
