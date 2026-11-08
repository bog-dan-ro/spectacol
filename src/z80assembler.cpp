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

#include "z80assembler.h"

#include "qmlui.h"
#include "zxemulator.h"

#include <zxkit/debug/Assembler.h>

namespace {

zxkit::AssemblerOptions assemblerOptions(bool z80n)
{
    // Plain numbers are in the base the debugger shows them in
    return { .z80n = z80n, .defaultBase = debuggerOutputBase() };
}

} // namespace

Z80Assembler::Z80Assembler(QObject *parent) : QValidator(parent) { }

bool Z80Assembler::write(const QString &asmLine, int address) const
{
    auto &zx = ZxEmulator::instance();
    std::expected<std::vector<uint8_t>, std::string> bytes;
    {
        // Like the debugger's poke command: no contention, no t-states, no breakpoint checks
        auto emulator = zx.emulator().writeLock();
        bytes = zxkit::Assembler::assemble(asmLine.toStdString(), uint16_t(address),
                                           assemblerOptions(emulator->cpu().isZ80nEnabled()));
        if (bytes)
            emulator->writeMemory(uint16_t(address), *bytes, true);
    }
    if (!bytes) {
        zx.showMessage(QString::fromStdString(bytes.error()), ZxEmulator::Warning);
        return false;
    }
    return true;
}

QValidator::State Z80Assembler::validate(QString &value, int &) const
{
    const bool z80n = ZxEmulator::instance().emulator().readLock()->cpu().isZ80nEnabled();
    return zxkit::Assembler::assemble(value.toStdString(), uint16_t(m_address),
                                      assemblerOptions(z80n))
            ? Acceptable
            : Intermediate;
}
