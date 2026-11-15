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
// A short list, one for each look: ZxKit offers every filter at every scale, which is too
// many choices for a menu. Settings saved with another preset fall back to its filter here.
inline std::span<const ScreenFilterPreset> screenFilterPresets()
{
    using F = zxkit::VideoFilter;
    static const std::vector<ScreenFilterPreset> presets{
        { u"normal"_s, u"Normal"_s, {} },
        { u"scanlines:1"_s, u"CRT Scanlines"_s, { { F::Scanlines, 1 } } },
        { u"rgbmask:1"_s, u"RGB phosphor mask"_s, { { F::RgbMask, 1 } } },
        { u"tv:2"_s, u"TV2x"_s, { { F::Tv, 2 } } },
        { u"hq:2"_s, u"HQ2x"_s, { { F::Hq, 2 } } },
        { u"hq:4"_s, u"HQ4x"_s, { { F::Hq, 4 } } },
        { u"xbrz:4"_s, u"xBRZ4x"_s, { { F::Xbrz, 4 } } },
        { u"xbrz:4+scanlines:1"_s,
          u"xBRZ4x + Scanlines"_s,
          { { F::Xbrz, 4 }, { F::Scanlines, 1 } } },
    };
    return presets;
}
