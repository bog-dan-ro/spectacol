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

#include <QQmlEngine>

#include <QObject>

#include <cstddef>
#include <functional>

class ZxEmulator;
class ZxTape : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use ZxEmulator.tape instead")
    Q_PROPERTY(bool hasTape MEMBER m_hasTape NOTIFY hasTapeChanged)
    Q_PROPERTY(QString programName MEMBER m_programName NOTIFY programNameChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(QString suggestedFileName READ suggestedFileName NOTIFY programNameChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY sourceChanged)
    Q_PROPERTY(bool inserted READ inserted NOTIFY insertedChanged)

public:
    explicit ZxTape(ZxEmulator *parent = 0);
    void updateBrowseData();
    QString programName() const
    {
        return m_programName;
    }
    void setProgramName(const QString &programName);
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE bool canSaveTap() const;
    bool playing() const
    {
        return m_playing;
    }
    void setPlaying(bool playing);
    QString suggestedFileName() const;
    bool canSave() const;
    bool inserted() const;
    void setSource(const QString &path, const QString &format);
    void beforeReplacing(std::function<void()> action);
    bool changePending() const;

public slots:
    void togglePlay();
    void rewind();
    void newBlank();
    void clear();
    bool save();
    bool saveAs(const QString &filePath, const QString &format);
    void resolveChange(bool proceed);

signals:
    void hasTapeChanged(bool hasTape);
    void programNameChanged();
    void playingChanged();
    void sourceChanged();
    void insertedChanged();
    void unsavedChanges();

private:
    void setHasTape(bool hasTape);
    void checkPendingChange();
    void resetDeck(bool blank);
    bool write(const QString &filePath, const QString &format, bool newExport);

    bool m_hasTape = false;
    bool m_playing = false;
    bool m_blank = false;
    QString m_sourcePath;
    QString m_sourceFormat;
    std::size_t m_savedBlocks = 0;
    std::function<void()> m_pendingChange;
    bool m_waitingForPause = false;
    QString m_programName;
    ZxEmulator *m_zxEmulator = nullptr;
};
