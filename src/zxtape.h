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

public slots:
    void open(QString filePath);
    void togglePlay();
    void rewind();
    void clear();
    bool write(QString filePath);

signals:
    void hasTapeChanged(bool hasTape);
    void programNameChanged();
    void playingChanged();

private:
    void setHasTape(bool hasTape);

    bool m_hasTape = false;
    bool m_playing = false;
    QString m_programName;
    ZxEmulator *m_zxEmulator = nullptr;
};
