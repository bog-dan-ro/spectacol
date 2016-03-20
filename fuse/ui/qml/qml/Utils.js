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

.pragma library

function shortcutText(text)
{
    var ampersantPos = text.indexOf("&");
    if (ampersantPos === -1)
        return text;
    return text.substr(0, ampersantPos) + "<b><u>" + text.substr(ampersantPos + 1, 1) + "</u></b>" + text.substr(ampersantPos + 2);
}
