#include "folderlistmodel.h"

#include <QDateTime>
#include <QDirIterator>

FolderListModel::FolderListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_spectrumInit = libspectrum_default_init();
    libspectrum_init(&m_spectrumInit);
}

FolderListModel::~FolderListModel()
{
    libspectrum_end(m_spectrumInit.context);
}

void FolderListModel::setFolder(QString folder)
{
    if (folder.startsWith(QLatin1Literal("file://")))
        folder = folder.mid(7);

    if (m_folder == folder)
        return;

    m_folder = QFileInfo(folder).absoluteFilePath();
    if (!m_folder.startsWith(m_rootFolder))
        m_folder = m_rootFolder;

    refresh();
    emit folderChanged(folder);
}

void FolderListModel::setRootFolder(QString rootFolder)
{
    if (rootFolder.startsWith(QLatin1Literal("file://")))
        rootFolder = rootFolder.mid(7);

    if (m_rootFolder == rootFolder)
        return;

    m_rootFolder = QFileInfo(rootFolder).absoluteFilePath();
    emit rootFolderChanged(rootFolder);

    if (!m_folder.startsWith(m_rootFolder))
        setFolder(m_rootFolder);
    else
        refresh();
}

FolderListModel::FileSortCriteria FolderListModel::sortCriteria() const
{
    return m_sortCriteria;
}

void FolderListModel::setSortCriteria(FolderListModel::FileSortCriteria sortCriteria)
{
    if (m_sortCriteria == sortCriteria)
        return;

    m_sortCriteria = sortCriteria;
    emit sortCriteriaChanged(sortCriteria);
    refresh();
}

bool FolderListModel::showDirsFirst() const
{
    return m_showDirsFirst;
}

void FolderListModel::setShowDirsFirst(bool showDirsFirst)
{
    if (m_showDirsFirst == showDirsFirst)
        return;

    m_showDirsFirst = showDirsFirst;
    emit showDirsFirstChanged(showDirsFirst);
}

bool FolderListModel::isDir(int index)
{
    if (size_t(index) >= m_files.size())
        return false;
    return m_files[index].isDir();
}

QString FolderListModel::path(int index)
{
    if (size_t(index) >= m_files.size())
        return QString();
    return m_files[index].absoluteFilePath();
}

void FolderListModel::cdUp()
{
    QDir d(m_folder);
    d.cdUp();
    setFolder(d.absolutePath());
}

int FolderListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_files.size();
}

QVariant FolderListModel::data(const QModelIndex &index, int role) const
{
    if (size_t(index.row()) >= m_files.size())
        return QVariant();

    switch (role) {
    case Name:
        return m_files[index.row()].fileName();
    case Path:
        return m_files[index.row()].absoluteFilePath();
    case IsDir:
        return m_files[index.row()].isDir();
    }
    return QVariant();
}

QHash<int, QByteArray> FolderListModel::roleNames() const
{
    static QHash<int, QByteArray> ret = {
        {Name, "name"},
        {Path, "path"},
        {IsDir, "isDir"}
    };
    return ret;
}

bool FolderListModel::canOpen(const QString &path)
{
    libspectrum_id_t fileType;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const auto &buffer = f.read(1024*1024);
    auto error = libspectrum_identify_file(m_spectrumInit.context, &fileType, path.toUtf8().constData(), (const unsigned char *)buffer.constData(), buffer.size());
    return error == LIBSPECTRUM_ERROR_NONE && fileType != LIBSPECTRUM_ID_UNKNOWN;
}

void FolderListModel::refresh()
{
    beginResetModel();
    m_files.clear();
    QDirIterator it(m_folder, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    while(it.hasNext()) {
        it.next();
        const auto &inf = it.fileInfo();
        if (inf.isDir() || canOpen(inf.absoluteFilePath().toUtf8().constData()))
            m_files.push_back(inf);
    }

    std::sort(m_files.begin(), m_files.end(), [this](const QFileInfo &a, const QFileInfo &b){
        if (m_showDirsFirst) {
            if (a.isDir() && !b.isDir())
                return true;

            if (!a.isDir() && b.isDir())
                return false;
        }
        switch (m_sortCriteria) {
        case ByName:
            return a.fileName() < b.fileName();
        case ByDateDesc:
            return a.lastModified() > b.lastModified();
        }
        return true;
    });
    endResetModel();
}
