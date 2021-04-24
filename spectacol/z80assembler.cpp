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

#include "z80assembler.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include <map>

extern "C"  {
# include <debugger/debugger.h>
}

Z80Assembler::Z80Assembler(QObject *parent) : QValidator(parent)
{
    QFile f(":/z80_opcodes.csv");
    if (!f.open(QIODevice::ReadOnly))
        return;

    auto globalObject = m_engine.globalObject();
    while (!f.atEnd()) {
        auto line = f.readLine().trimmed();
        if (line.isEmpty() || line[0] == '#')
            continue;

        if (line[0] == '%') {
            auto rules = line.mid(1).trimmed().split(' ');
            if (rules.size() != 2)
                continue;
            m_rules.emplace_back(std::make_pair(rules[0], rules[1].trimmed()));
            continue;
        }

        if (line[0] == '=') {
            auto rules = line.mid(1).trimmed().split(' ');
            if (rules.size() != 2)
                continue;
            globalObject.setProperty(QString::fromLatin1(rules[0]), rules[1].toInt());
            continue;
        }

        auto rules = line.split('\t');
        if (rules.size() != 2)
            continue;
        m_assembleRules.emplace_back(std::make_pair(QRegularExpression::escape(rules[0].trimmed()), rules[1].trimmed()));
    }
}

static inline int toNumber(QString number)
{
    number = number.toUpper();
    if (number.startsWith(QLatin1String("0X")))
        return number.mid(2).toInt(nullptr, 16);

    if (number.endsWith(QLatin1Char('H'))) {
        number.chop(1);
        return number.toInt(nullptr, 16);
    }

    return number.toInt(nullptr, debugger_output_base);
}

static inline QString makeValue(const QString &pattern, const QString &value, int address)
{
    if (pattern == QLatin1String("nn")) {
        int number = toNumber(value);
        Q_ASSERT(number < 0x10000);
        return QString(QLatin1String("%1 %2")).arg(number&0xff).arg(number>>8);
    } else if (pattern == QLatin1String("p")) {
        int number = toNumber(value);
        number -= address;
        Q_ASSERT(number < 0x100);
        return QString::number(uint8_t(number));
    } else if (pattern == QLatin1String("n") || pattern == QLatin1String("o")) {
        int number = toNumber(value);
        Q_ASSERT(number < 0x100);
        return QString::number(number);
    }
    return value;
}

QByteArray Z80Assembler::assemble(const QString &asmLine, int address, const QByteArray &assembledBytes) const
{
    for (const auto &as : m_assembleRules) {
        QString asmPattern = as.first;
        std::map<int, QString> rules;
        for (const auto &rule : m_rules) {
            int pos = asmPattern.indexOf(rule.first);
            if (pos == -1)
                continue;
            rules[pos] = rule.first;
            asmPattern.replace(rule.first, rule.second);
        }
        auto match = QRegularExpression(QLatin1Char('^') + asmPattern + QLatin1Char('$')).match(asmLine);
        if (!match.hasMatch())
            continue;

        auto matches = match.capturedTexts();
        matches.takeFirst();
        if (rules.size() != size_t(matches.size())) {
            qWarning() << "rules != matches";
            continue;
        }
        address += rules.size() + 1;
        QString bytes = as.second;
        auto it = rules.cbegin();

        for (const QString &capture : qAsConst(matches)) {
            bytes.replace(it->second, makeValue(it->second, capture, address));
            ++it;
        }

        QByteArray values;
        const auto lines = bytes.split(QLatin1Char(' '));
        for (QString eval : lines) {
            eval = eval.replace(QLatin1String("0x0x"), QLatin1String("0x"));
            auto val = m_engine.evaluate(eval);
            if (val.isError()) {
                qWarning() << "Fail to evaluate " << eval;
                return QByteArray();
            }
            int byte = val.toInt();
            Q_ASSERT(byte < 0x100);
            values.push_back(uint8_t(byte));
        }

        if (!assembledBytes.isEmpty() && assembledBytes != values) {
            if ((uint8_t(assembledBytes[0]) == 0xdd || uint8_t(assembledBytes[0]) == 0xfd) &&
                assembledBytes.endsWith(values)) {
                values.push_front(assembledBytes.left(assembledBytes.size() - values.size()));
            } else if (uint8_t(assembledBytes[0]) == 0xed) {
                values = assembledBytes;
            }
        }

        return values;
    }
    return QByteArray();
}

bool Z80Assembler::write(const QString &asmLine, int address, const QByteArray &assembledBytes) const
{
    const QByteArray bytes = assemble(asmLine.toUpper(), address, assembledBytes);
    if (bytes.isEmpty())
        return false;

    for (uint8_t byte : bytes)
        writebyte(address++, byte);

    return true;
}

QValidator::State Z80Assembler::validate(QString &value, int &) const
{
    return assemble(value.toUpper(), 0).isEmpty() ? QValidator::Intermediate : QValidator::Acceptable;
}
