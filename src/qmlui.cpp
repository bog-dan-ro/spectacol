/* qmlui.cpp: UI helpers: the debugger number base and blocking the error messages.

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

#include "qmlui.h"

#include <atomic>

namespace {
std::atomic_uint_fast32_t s_blockErrors{ 0 };
std::atomic<int> s_debuggerOutputBase{ 16 };
} // namespace

int debuggerOutputBase()
{
    return s_debuggerOutputBase.load(std::memory_order_relaxed);
}

void setDebuggerOutputBase(int base)
{
    s_debuggerOutputBase.store(base == 10 ? 10 : 16, std::memory_order_relaxed);
}

void showErrors()
{
    --s_blockErrors;
}

void hideErrors()
{
    ++s_blockErrors;
}
