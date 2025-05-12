/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

#include "zxrequests.h"

#include "zxgamesmodel.h"
#include "fuseemulator.h"

#include <QCryptographicHash>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {
    const QString zxSpectrumSearchTitle = QLatin1String("&fl[]=identifier&fl[]=title&sort[]=titleSorter+asc&rows=%1&page=%2&output=json");
    const QLatin1String metadata("https://archive.org/metadata/");
}

ZXRequest::ZXRequest(ZXGamesModel *parent)
    : QObject(parent)
    , m_model(parent)
{}

ZXRequest::~ZXRequest()
{}

void ZXRequest::abort()
{
    m_aborted = true;
    if (m_reply)
        m_reply->abort();
}

ZXSearchRequest::ZXSearchRequest(ZXGamesModel *parent)
    : ZXRequest(parent)
{
    QNetworkRequest req(parent->searchString() + zxSpectrumSearchTitle.arg(50).arg(1));
    req.setPriority(QNetworkRequest::HighPriority);
    m_reply = parent->networkManager().get(req);
    connect(m_reply.data(), &QNetworkReply::finished, this, [this] {
        if (!m_aborted && !m_data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(m_data);
            auto response = doc.object().value(QLatin1String("response")).toObject();
            size_t numFound = response.value(QLatin1String("numFound")).toInt();
            auto docs = response.value(QLatin1String("docs")).toArray();
            std::vector<ZXGamesModel::ZXData> data;
            for (const auto &doc : docs) {
                auto obj = doc.toObject();
                data.emplace_back(obj.value(QLatin1String("title")).toString(), obj.value(QLatin1String("identifier")).toString());
            }
            m_model->setSearchData(data, numFound);
        }
        m_reply->deleteLater();
        m_model->requestFinished(this);
        deleteLater();
    });

//    connect(m_reply.data(), &QNetworkReply::error, this, [this](QNetworkReply::NetworkError error){
//        if (error == QNetworkReply::NoError)
//            return;
//        abort();
//    });

    connect(m_reply.data(), &QNetworkReply::readyRead, this, [this] {
        m_data += m_reply->readAll();
    });
}

ZXFetchRequest::ZXFetchRequest(ZXGamesModel *parent, size_t start, size_t count)
    : ZXRequest(parent)
    , m_start(start)
    , m_count(count)
{
    QNetworkRequest req(parent->searchString() + zxSpectrumSearchTitle.arg(count).arg(start / 50 + 1));
    req.setPriority(QNetworkRequest::NormalPriority);
    m_reply = parent->networkManager().get(req);
    connect(m_reply.data(), &QNetworkReply::finished, this, [this] {
        if (!m_aborted && !m_data.isEmpty()) {
            auto response = QJsonDocument::fromJson(m_data).object().value(QLatin1String("response")).toObject();
            auto docs = response.value(QLatin1String("docs")).toArray();
            std::vector<ZXGamesModel::ZXData> data;
            for (const auto &doc : docs) {
                auto obj = doc.toObject();
                data.emplace_back(obj.value(QLatin1String("title")).toString(), obj.value(QLatin1String("identifier")).toString());
            }
            m_model->setFetchedData(data);
        }
        m_reply->deleteLater();
        m_model->requestFinished(this);
        deleteLater();
    });

    connect(m_reply.data(), &QNetworkReply::readyRead, this, [this] {
        m_data += m_reply->readAll();
    });
}

ZXUpdateRequest::ZXUpdateRequest(ZXGamesModel *parent, const QString &identifier, size_t row)
    : ZXRequest(parent)
    , m_identifier(identifier)
    , m_row(row)
{
    QNetworkRequest req(metadata + identifier);
    req.setPriority(QNetworkRequest::LowPriority);
    m_reply = parent->networkManager().get(req);
    connect(m_reply.data(), &QNetworkReply::finished, this, [this] {
        if (!m_aborted && !m_data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(m_data);
            auto rootObject = doc.object();
            auto files = rootObject.value(QLatin1String("files")).toArray();
            auto metadata = rootObject.value(QLatin1String("metadata")).toObject();
            QString emulator_ext = QLatin1Char('.') + metadata.value(QLatin1String("emulator_ext")).toString();
            for (const auto &file : files) {
                auto obj = file.toObject();
                QString name = obj.value(QLatin1String("name")).toString();
                if (name.endsWith(emulator_ext)) {
                    QString md5 = obj.value(QLatin1String("md5")).toString().toLower();
                    QString path = FuseEmulator::instance().saveFilePath(name);
                    QFile f(path);
                    if (!f.open(QIODevice::ReadOnly) ||
                            QCryptographicHash::hash(f.readAll(), QCryptographicHash::Md5).toHex().toLower() != md5) {
                        auto workable_servers = rootObject.value(QLatin1String("workable_servers")).toArray();
                        if (workable_servers.empty())
                            return;

                        auto download_url
                            = QLatin1String("https://")
                              + workable_servers.at(rand() % workable_servers.size()).toString()
                              + rootObject.value(QLatin1String("dir")).toString() + QLatin1Char('/')
                              + name;

                        QNetworkRequest req(download_url);
                        req.setPriority(QNetworkRequest::HighPriority);
                        auto reply = m_model->networkManager().get(req);
                        connect(reply, &QNetworkReply::readyRead, this, [reply, this]{
                            m_fileData += reply->readAll();
                        });

                        connect(reply, &QNetworkReply::finished, this, [reply, md5, path, this] {
                            if (QCryptographicHash::hash(m_fileData, QCryptographicHash::Md5).toHex().toLower() == md5) {
                                QDir d(QFileInfo(path).absoluteDir());
                                if (d.mkpath(d.absolutePath())) {
                                    QFile f(path);
                                    if (f.open(QIODevice::WriteOnly)) {
                                        f.write(m_fileData);
                                        f.close();
                                        m_model->updateData(m_row, path);
                                    }
                                }
                            }
                            m_model->updateFinished(m_identifier);
                            reply->deleteLater();
                            deleteLater();
                        });
                    } else {
                        m_model->updateData(m_row, path);
                        m_model->updateFinished(m_identifier);
                        deleteLater();
                    }
                    break;
                }
            }
        }
        m_reply->deleteLater();
    });

    connect(m_reply.data(), &QNetworkReply::readyRead, this, [this] {
        m_data += m_reply->readAll();
    });
}
