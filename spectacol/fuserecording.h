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

#include <QQmlEngine>

#include "fuseobject.h"

class FuseEmulator;
class FuseRecording : public FuseObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use fuse.recording instead")

    Q_PROPERTY(Type type MEMBER m_type NOTIFY typeChanged)

public:
    enum class Type {
        None,
        Playback,
        Recording
    };
    Q_ENUM(Type)

public:
    explicit FuseRecording(FuseEmulator *parent = nullptr);
    void setStopVisible(bool visible);

public slots:
    void start();
    void continueRecording();
    void rollback();
    void rollbackTo();
    void finalise();
    void stop();

signals:
    void typeChanged(Type type);

private:
    FuseEmulator *m_emulator;
    Type m_type = Type::None;
};
