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

#include "screenfilterinput.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

QSize filterInputPixelScale(const QImage &image, QSize maximumScale)
{
    QSize scale(1, 1);
    if (image.isNull())
        return scale;
    Q_ASSERT(image.depth() == 32);
    const int vertical = std::max(1, maximumScale.height());
    if (vertical > 1 && image.height() % vertical == 0) {
        bool repeated = true;
        const auto rowBytes = std::size_t(image.width()) * sizeof(quint32);
        for (int y = 0; y < image.height() && repeated; y += vertical)
            for (int repeat = 1; repeat < vertical && repeated; ++repeat)
                repeated = std::memcmp(image.constScanLine(y), image.constScanLine(y + repeat),
                                       rowBytes)
                        == 0;
        if (repeated)
            scale.setHeight(vertical);
    }

    const int horizontal = std::max(1, maximumScale.width());
    if (horizontal > 1 && image.width() % horizontal == 0) {
        bool repeated = true;
        for (int y = 0; y < image.height() && repeated; y += scale.height()) {
            const auto *row = reinterpret_cast<const quint32 *>(image.constScanLine(y));
            for (int x = 0; x < image.width() && repeated; x += horizontal)
                for (int repeat = 1; repeat < horizontal && repeated; ++repeat)
                    repeated = row[x] == row[x + repeat];
        }
        if (repeated)
            scale.setWidth(horizontal);
    }
    return scale;
}
