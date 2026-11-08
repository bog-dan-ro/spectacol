/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

#include <zxkit/video/VideoFilters.h>

#include <QString>
#include <span>
#include <vector>

using namespace Qt::StringLiterals;

struct ScreenFilterPreset
{
    QString id;
    QString name;
    std::vector<zxkit::VideoFilterStage> stages;
};

// Shared by the existing QML selector, persistent settings and the renderer.
// IDs describe ordered stages so saved choices are independent of menu order.
inline std::span<const ScreenFilterPreset> screenFilterPresets()
{
    using F = zxkit::VideoFilter;
    static const std::vector<ScreenFilterPreset> presets = [] {
        std::vector<ScreenFilterPreset> result{
            { u"normal"_s, u"Normal"_s, {} },
            { u"scanlines:1"_s, u"CRT Scanlines"_s, { { F::Scanlines, 1 } } },
            { u"2xsai:2"_s, u"2xSaI"_s, { { F::Sai, 2 } } },
            { u"hq:2"_s, u"HQ2x"_s, { { F::Hq, 2 } } },
        };
        for (const auto &info : zxkit::videoFilterCatalog()) {
            for (uint32_t scale = info.minScale; scale <= info.maxScale; ++scale) {
                const auto id = QString::fromUtf8(info.id.data(), qsizetype(info.id.size()))
                        + u":"_s + QString::number(scale);
                if ((info.filter == F::Nearest || info.filter == F::Bilinear) && scale == 1)
                    continue; // Normal already covers identity scaling.
                bool exists = false;
                for (const auto &preset : result)
                    exists |= preset.id == id;
                if (exists)
                    continue;
                auto name = QString::fromUtf8(info.name.data(), qsizetype(info.name.size()));
                if (scale > 1 && info.filter != F::Sai && info.filter != F::SuperSai)
                    name += u" "_s + QString::number(scale) + u"x"_s;
                result.push_back({ id, name, { { info.filter, scale } } });
            }
        }
        result.push_back({ u"hq:2+tv:2"_s, u"HQ2x + TV2x (4x)"_s, { { F::Hq, 2 }, { F::Tv, 2 } } });
        result.push_back({ u"hq:4+scanlines:1"_s,
                           u"HQ4x + Scanlines"_s,
                           { { F::Hq, 4 }, { F::Scanlines, 1 } } });
        result.push_back({ u"xbrz:4+scanlines:1"_s,
                           u"xBRZ4x + Scanlines"_s,
                           { { F::Xbrz, 4 }, { F::Scanlines, 1 } } });
        result.push_back({ u"xbrz:4+rgbmask:1"_s,
                           u"xBRZ4x + RGB mask"_s,
                           { { F::Xbrz, 4 }, { F::RgbMask, 1 } } });
        result.push_back(
                { u"hq:4+tv:4"_s, u"HQ4x + TV4x (16x)"_s, { { F::Hq, 4 }, { F::Tv, 4 } } });
        return result;
    }();
    return presets;
}
