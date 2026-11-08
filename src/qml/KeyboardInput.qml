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

import QtQml

// Each finger/navigation press owns its original key and any sticky modifiers
// used by that chord. Emit only changes to the union of all these owners.
QtObject {
    id: input
    property bool sticky: false
    property int extraModifier: 0
    property var heldKeys: []
    property var latchedKeys: []
    property var _owners: ({})
    property var _latches: ({})
    property int _serial: 0
    signal keyChanged(int key, bool pressed)

    onStickyChanged: clear()
    onExtraModifierChanged: clear()

    function isModifier(key: int): bool {
        return key === Qt.Key_Shift || key === Qt.Key_Control || key === extraModifier;
    }

    function sync(): void {
        const desired = {};
        for (const key of Object.keys(_latches))
            desired[key] = true;
        for (const owner of Object.values(_owners)) {
            for (const key of owner.keys)
                desired[key] = true;
        }
        const next = Object.keys(desired).map(Number);
        // Press modifiers before ordinary keys, release ordinary keys first.
        const changes = next.filter(key => heldKeys.indexOf(key) < 0);
        changes.sort((a, b) => Number(isModifier(b)) - Number(isModifier(a)));
        for (const key of changes)
            keyChanged(key, true);
        const releases = heldKeys.filter(key => next.indexOf(key) < 0);
        releases.sort((a, b) => Number(isModifier(a)) - Number(isModifier(b)));
        for (const key of releases)
            keyChanged(key, false);
        heldKeys = next;
        latchedKeys = Object.keys(_latches).map(Number);
    }

    function press(owner: string, key: int): void {
        if (!key || _owners[owner] !== undefined)
            return;
        const record = {keys: [], latches: {}, modifier: 0, token: 0};
        if (sticky && isModifier(key)) {
            record.modifier = key;
            if (_latches[key] !== undefined) {
                delete _latches[key];
            } else {
                record.token = ++_serial;
                _latches[key] = record.token;
                // The latch may be consumed by a chord before this finger
                // lifts; its physical hold still owns the modifier meanwhile.
                record.keys.push(key);
            }
        } else {
            record.keys.push(key);
            if (!isModifier(key)) {
                for (const modifier of Object.keys(_latches)) {
                    record.keys.push(Number(modifier));
                    record.latches[modifier] = _latches[modifier];
                }
            }
        }
        _owners[owner] = record;
        sync();
    }

    function release(owner: string, cancelled: bool): void {
        const record = _owners[owner];
        if (record === undefined)
            return;
        delete _owners[owner];
        // A cancelled modifier tap must not leave a latch behind. Tokens keep
        // late releases from consuming a modifier that was tapped again later.
        if (cancelled && record.token && _latches[record.modifier] === record.token)
            delete _latches[record.modifier];
        for (const modifier of Object.keys(record.latches)) {
            if (_latches[modifier] === record.latches[modifier])
                delete _latches[modifier];
        }
        sync();
    }

    function releaseLatch(key: int): void {
        delete _latches[key];
        sync();
    }

    function clear(): void {
        _owners = ({});
        _latches = ({});
        sync();
    }
}
