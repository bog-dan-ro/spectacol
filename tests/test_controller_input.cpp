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

#include "controllerinput.h"

#include <QTest>
#include <limits>

class ControllerTest : public QObject {
    Q_OBJECT
private slots:
    void joystickSources()
    {
        ControllerInput input;
        input.axis(ControllerInput::LeftX, -1);
        input.axis(ControllerInput::RightX, -1);
        input.press(1, 0, ControllerInput::LeftBit);
        QCOMPARE(input.state().joystick, ControllerInput::LeftBit);
        input.axis(ControllerInput::LeftX, 0);
        input.axis(ControllerInput::RightX, 0);
        QCOMPARE(input.state().joystick, ControllerInput::LeftBit);
        input.release(1);
        QCOMPARE(input.state().joystick, 0);
        input.axis(ControllerInput::RightY, 1);
        input.press(2, 0);
        input.press(3, 0);
        input.release(2);
        QCOMPARE(input.state().joystick, ControllerInput::DownBit | ControllerInput::FireBit);
        input.release(3);
        QCOMPARE(input.state().joystick, ControllerInput::DownBit);
    }
    void qaop()
    {
        ControllerInput input;
        input.configure(ControllerInput::Qaop, ControllerInput::qaopKeys, 0.4);
        input.axis(ControllerInput::LeftX, -1);
        input.axis(ControllerInput::LeftY, -1);
        input.press(1, 0);
        input.press(2, 0);
        QCOMPARE(input.state().joystick, 0);
        QCOMPARE(input.state().keys, (QSet<int>{Qt::Key_Q, Qt::Key_O, Qt::Key_Space}));
        input.release(1);
        QVERIFY(input.state().keys.contains(Qt::Key_Space));
        input.release(2);
        QVERIFY(!input.state().keys.contains(Qt::Key_Space));
        input.axis(ControllerInput::LeftX, 0);
        input.axis(ControllerInput::LeftY, 0);
        QVERIFY(input.state().keys.isEmpty());
    }
    void customAndSharedKeys()
    {
        ControllerInput input;
        input.configure(ControllerInput::Custom,
                        {Qt::Key_Shift, Qt::Key_A, Qt::Key_Shift, Qt::Key_P, Qt::Key_Shift}, 0.4);
        input.axis(ControllerInput::LeftX, -1);
        input.axis(ControllerInput::LeftY, -1);
        input.press(1, 0);
        input.press(2, Qt::Key_Shift);
        QCOMPARE(input.state().keys, QSet<int>{Qt::Key_Shift});
        input.axis(ControllerInput::LeftX, 0);
        input.axis(ControllerInput::LeftY, 0);
        input.release(1);
        QCOMPARE(input.state().keys, QSet<int>{Qt::Key_Shift});
        input.release(2);
        QVERIFY(input.state().keys.isEmpty());
        input.press(3, Qt::Key_A);
        input.configure(ControllerInput::Qaop, ControllerInput::qaopKeys, 0.4);
        QVERIFY(input.state().keys.isEmpty());
        QVERIFY(!input.release(3).has_value());
    }
    void touchAndDisconnect()
    {
        ControllerInput input;
        input.configure(ControllerInput::Qaop, ControllerInput::qaopKeys, 0.4);
        input.axis(ControllerInput::LeftX, -1);
        input.axis(ControllerInput::TouchX, -1);
        input.press(1, Qt::Key_A);
        input.press(ControllerInput::TouchButtonOffset + 1, 0);
        input.clearPhysical();
        QCOMPARE(input.state().keys, (QSet<int>{Qt::Key_O, Qt::Key_Space}));
        input.clearTouch();
        QVERIFY(input.state().keys.isEmpty());
        input.axis(ControllerInput::LeftX, 1);
        input.axis(ControllerInput::TouchX, 1);
        input.clearTouch();
        QCOMPARE(input.state().keys, QSet<int>{Qt::Key_P});
        input.clear();
        QVERIFY(input.state().keys.isEmpty());
    }
    void deadZone()
    {
        ControllerInput input;
        input.axis(ControllerInput::LeftX, 0.39);
        QCOMPARE(input.state().joystick, 0);
        input.axis(ControllerInput::LeftX, 0.4);
        QCOMPARE(input.state().joystick, ControllerInput::RightBit);
        input.axis(ControllerInput::LeftX, std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(input.state().joystick, 0);
        input.axis(ControllerInput::RightX, -1);
        input.axis(ControllerInput::LeftX, 0);
        QCOMPARE(input.state().joystick, ControllerInput::LeftBit);
        input.configure(ControllerInput::Joystick, ControllerInput::qaopKeys, 0);
        QCOMPARE(input.state().joystick, 0);
        input.axis(ControllerInput::LeftX, 0.001);
        QCOMPARE(input.state().joystick, ControllerInput::RightBit);
    }
    void actionsRetainPressIdentity()
    {
        ControllerInput input;
        input.press(1, 10000);
        input.press(2, 10000);
        input.press(1, 20000); // A repeat must not replace the held action.
        QCOMPARE(input.release(1), std::optional<int>{10000});
        QVERIFY(input.state().actions.contains(10000));
        QCOMPARE(input.release(2), std::optional<int>{10000});
        QVERIFY(input.state().actions.isEmpty());
        QVERIFY(!input.release(2).has_value());
        input.press(3, 10000);
        input.clear();
        QVERIFY(!input.release(3).has_value());
    }
    void keyOwnership()
    {
        HeldKeyState keys;
        keys.setSource(1, {10});             // Physical modifier
        keys.setSource(2, {10, 20});         // A punctuation chord using it
        const auto changes = keys.setSource(2, {});
        QCOMPARE(changes.size(), 1);
        QCOMPARE(changes[0], (std::pair<int, bool>{20, false}));
        QCOMPARE(keys.keys(), QSet<int>{10});
        keys.setSource(3, {10, 30});         // Controller chord
        keys.setSource(1, {});
        QCOMPARE(keys.keys(), (QSet<int>{10, 30}));
        QVERIFY(keys.setSource(3, {10, 30}).isEmpty());
        const auto cleared = keys.clear();
        QCOMPARE(cleared.size(), 2);
        for (const auto &[key, pressed] : cleared)
            QVERIFY(!pressed);
        QVERIFY(keys.keys().isEmpty());
        QVERIFY(keys.setSource(3, {}).isEmpty());
    }
};
QTEST_GUILESS_MAIN(ControllerTest)
#include "test_controller_input.moc"
