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

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

#include <atomic>
#include <functional>
#include <stop_token>
#include <thread>

// Android's shared storage, which the app can only read through folders the user picks:
// - device folders, granted once and listed in Quick Open's Spectacol folder;
// - the folder where Spectacol 4 kept its files, which it copies into app storage once.
class DeviceStorage : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool available READ available CONSTANT FINAL)
    Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged FINAL)
    Q_PROPERTY(QString legacyFolder READ legacyFolder CONSTANT FINAL)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged FINAL)
    Q_PROPERTY(int importedFiles READ importedFiles NOTIFY importProgressChanged FINAL)
    Q_PROPERTY(QString importSummary READ importSummary NOTIFY importProgressChanged FINAL)

public:
    struct CopyResult
    {
        int copied = 0;
        int skipped = 0; // the same file was already there
        int renamed = 0; // a different file had the name: copied with a numbered one
        QStringList failed;
        bool stopped = false;
    };
    using Progress = std::function<void(int copied, const QString &name)>;
    // Copies the files and folders inside source into destination, keeping what is there:
    // identical files are skipped, different ones with the same name get a numbered name.
    // Files keep their modification times. Works with local folders and content:// trees.
    static CopyResult copyTree(const QString &source, const QString &destination,
                               std::stop_token stop = {}, const Progress &progress = {});

    explicit DeviceStorage(QString dataPath, QObject *parent = nullptr);
    ~DeviceStorage() override;

    // Device folders can be added (Android, with a folder picker)
    bool available() const;
    QStringList folders() const;
    // Where Spectacol 4 kept its files, as people know the place; empty when there was nothing.
    QString legacyFolder() const;
    bool importing() const;
    int importedFiles() const;
    QString importSummary() const;

    Q_INVOKABLE void addFolder();
    Q_INVOKABLE void forgetFolder(const QString &path);
    Q_INVOKABLE void importLegacyFolder();
    Q_INVOKABLE void postponeLegacyImport(bool permanently);

signals:
    void foldersChanged();
    void folderAdded(const QString &path);
    // Once per start, while the old files haven't been copied or declined
    void offerLegacyImport();
    void importingChanged();
    void importProgressChanged();
    void message(const QString &text, int level);

private:
    void setFolders(QStringList folders);
    void startImport(const QString &tree);
    void finishImport(const CopyResult &result, const QString &tree);

    QString m_dataPath;
    QStringList m_folders;
    QString m_legacyPath;
    bool m_importing = false;
    std::atomic_int m_importedFiles = 0;
    QString m_importSummary;
    std::jthread m_worker;
};
