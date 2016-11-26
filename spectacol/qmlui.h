/* qmlui.h: convenient functions to post runnables on fuse thread.

    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

#ifndef QMLUI_H
#define QMLUI_H

#include <QString>

#include <functional>

extern "C" {
# include <debugger/debugger.h>
}

typedef std::function<void()> SpectrumEventFunction;

void pokeEvent(const SpectrumEventFunction &event);

template <typename T>
inline QString formatNumber(T nr)
{
    if (debugger_output_base == 10)
        return QString::number(nr);
    return QLatin1Literal("0x") + QString(QLatin1Literal("%1")).arg(nr, sizeof(T) * 2, 16, QLatin1Char('0')).toUpper();
}

void showErrors();
void hideErrors();

struct BlockErrors {
    BlockErrors() { hideErrors(); }
    ~BlockErrors() { showErrors(); }
};

#endif // QMLUI_H
