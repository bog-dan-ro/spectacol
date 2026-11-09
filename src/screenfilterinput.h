/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QImage>
#include <QSize>

// Collapse only exact copies within the frame's presentation grid. A genuine
// high-resolution pixel prevents horizontal collapse, while doubled rows can
// still be removed independently. Input pixels must be 32-bit RGBA.
QSize filterInputPixelScale(const QImage &image, QSize maximumScale);
