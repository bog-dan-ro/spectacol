/*
    Copyright (c) 2016-2026, BogDan Vatra <bogdan@kde.org>

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
#include <QValidator>

// The debugger's inline assembler: validates and writes one instruction with ZxKit's assembler
class Z80Assembler : public QValidator
{
    Q_OBJECT
    QML_ELEMENT
    // Where the instruction goes: relative jumps depend on it
    Q_PROPERTY(int address MEMBER m_address NOTIFY addressChanged)

public:
    explicit Z80Assembler(QObject *parent = nullptr);

    // Assembles `asmLine` at `address` and writes it to the memory (ROM included)
    Q_INVOKABLE bool write(const QString &asmLine, int address) const;

    // QValidator interface
    State validate(QString &value, int &pos) const override;

signals:
    void addressChanged();

private:
    int m_address = 0;
};
