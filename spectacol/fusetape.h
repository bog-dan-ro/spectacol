/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#include <QTimer>
#include <QQmlEngine>

#include "fuseobject.h"

extern "C"  {
# include <libspectrum.h>
}

class FuseEmulator;
class FuseTape : public FuseObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use fuse.tape instead")
    Q_PROPERTY(bool hasTape MEMBER m_hasTape NOTIFY hasTapeChanged)
    Q_PROPERTY(QString programName MEMBER m_programName NOTIFY programNameChanged)

public:
    explicit FuseTape(FuseEmulator *parent = 0);
    void updateBrowseData();
    QString programName() const;
    void setProgramName(const QString &programName);
    void refreshData();

public slots:
    void open(QString filePath);
    void togglePlay();
    void rewind();
    void clear();
    void write(QString filePath);

signals:
    void hasTapeChanged(bool hasTape);
    void programNameChanged();

private:
    static void checkProgramName(libspectrum_tape_block *block, void *user_data);

private:
    bool m_hasTape = false;
    QByteArray m_tapeData;
    QString m_programName;
    FuseEmulator *m_fuseEmulator = nullptr;
    QTimer m_saveSnapshotTimer;
};
