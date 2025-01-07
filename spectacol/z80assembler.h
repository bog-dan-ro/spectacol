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
#include <QJSEngine>
#include <QValidator>

#include <vector>
#include <tuple>

class Z80Assembler : public QValidator
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit Z80Assembler(QObject *parent = 0);
    Q_INVOKABLE bool write(const QString &asmLine, int address, const QByteArray &assembledBytes = QByteArray()) const;

    // QValidator interface
    State validate(QString &value, int &pos) const override;

private:
    QByteArray assemble(const QString &asmLine, int address, const QByteArray &assembledBytes = QByteArray()) const;

private:
    typedef std::pair<QString, QString> RulesType;
    std::vector<RulesType> m_rules;
    typedef std::pair<QString, QString> AssembleRules;
    std::vector<RulesType> m_assembleRules;
    mutable QJSEngine m_engine;
};
