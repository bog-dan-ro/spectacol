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

#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>
#include <memory>

using namespace Qt::StringLiterals;

class KeyboardInputTest : public QObject {
    Q_OBJECT
    QQmlEngine engine;
    std::unique_ptr<QObject> input;
    static constexpr int CobraControl = Qt::Key_Control | 0x20000000;

    void press(const QString &owner, int key) {
        QVERIFY(QMetaObject::invokeMethod(input.get(), "press", Q_ARG(QString, owner), Q_ARG(int, key)));
    }
    void release(const QString &owner, bool cancelled = false) {
        QVERIFY(QMetaObject::invokeMethod(input.get(), "release", Q_ARG(QString, owner), Q_ARG(bool, cancelled)));
    }
    void clear() { QVERIFY(QMetaObject::invokeMethod(input.get(), "clear")); }
    QSet<int> held() const {
        const auto keys = input->property("heldKeys").value<QJSValue>().toVariant().toList();
        QSet<int> result;
        for (const auto &key : keys) result.insert(key.toInt());
        return result;
    }
    void sticky() { input->setProperty("sticky", true); }
private slots:
    void init() {
        QQmlComponent component(&engine, QUrl(u"qrc:/KeyboardInput.qml"_s));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        input.reset(component.create());
        QVERIFY(input);
        input->setProperty("extraModifier", CobraControl);
    }
    void overlapAndRepeat() {
        QSignalSpy changes(input.get(), SIGNAL(keyChanged(int,bool)));
        press(u"finger1"_s, Qt::Key_A);
        press(u"finger2"_s, Qt::Key_A);
        press(u"finger1"_s, Qt::Key_B); // Repeats retain the original key.
        release(u"finger1"_s);
        QCOMPARE(held(), QSet<int>{Qt::Key_A});
        QCOMPARE(changes.size(), 1);
        release(u"finger2"_s);
        QVERIFY(held().isEmpty());
        QCOMPARE(changes.size(), 2);
    }
    void cancellationIsLocal() {
        press(u"finger1"_s, Qt::Key_A);
        press(u"finger2"_s, Qt::Key_B);
        press(u"navigation"_s, Qt::Key_B);
        release(u"finger1"_s, true);
        release(u"navigation"_s, true);
        QCOMPARE(held(), QSet<int>{Qt::Key_B});
        release(u"finger2"_s);
        QVERIFY(held().isEmpty());
    }
    void momentaryModifiers() {
        press(u"shift"_s, Qt::Key_Shift);
        press(u"symbol"_s, Qt::Key_Control);
        press(u"cobra"_s, CobraControl);
        press(u"letter"_s, Qt::Key_0);
        release(u"letter"_s);
        QCOMPARE(held(), (QSet<int>{Qt::Key_Shift, Qt::Key_Control, CobraControl}));
        release(u"symbol"_s, true);
        QCOMPARE(held(), (QSet<int>{Qt::Key_Shift, CobraControl}));
    }
    void stickyOverlappingChords() {
        sticky();
        press(u"modifier"_s, Qt::Key_Control);
        release(u"modifier"_s);
        press(u"finger1"_s, Qt::Key_A);
        press(u"finger2"_s, Qt::Key_B);
        release(u"finger1"_s);
        QCOMPARE(held(), (QSet<int>{Qt::Key_Control, Qt::Key_B}));
        release(u"finger2"_s);
        QVERIFY(held().isEmpty());
        press(u"modifier"_s, CobraControl);
        release(u"modifier"_s);
        press(u"finger1"_s, Qt::Key_0);
        release(u"finger1"_s, true);
        QVERIFY(held().isEmpty());
    }
    void stickyModifierStillTouched() {
        sticky();
        press(u"modifier"_s, Qt::Key_Shift);
        press(u"letter"_s, Qt::Key_A);
        release(u"letter"_s);
        QCOMPARE(held(), QSet<int>{Qt::Key_Shift});
        release(u"modifier"_s);
        QVERIFY(held().isEmpty());
        press(u"modifier"_s, CobraControl);
        press(u"letter"_s, Qt::Key_0);
        release(u"modifier"_s, true);
        QCOMPARE(held(), (QSet<int>{CobraControl, Qt::Key_0}));
        release(u"letter"_s);
        QVERIFY(held().isEmpty());
    }
    void stickyCancellationAndRetap() {
        sticky();
        press(u"modifier"_s, Qt::Key_Shift);
        release(u"modifier"_s, true);
        QVERIFY(held().isEmpty());
        press(u"modifier"_s, Qt::Key_Shift);
        release(u"modifier"_s);
        press(u"oldChord"_s, Qt::Key_A);
        press(u"modifier"_s, Qt::Key_Shift); // Toggle off.
        release(u"modifier"_s);
        press(u"modifier"_s, Qt::Key_Shift); // A new latch.
        release(u"modifier"_s);
        release(u"oldChord"_s);
        QCOMPARE(held(), QSet<int>{Qt::Key_Shift});
        press(u"newChord"_s, Qt::Key_B);
        release(u"newChord"_s);
        QVERIFY(held().isEmpty());
    }
    void clearingAndLateReleases() {
        sticky();
        press(u"modifier"_s, Qt::Key_Control);
        release(u"modifier"_s);
        press(u"finger"_s, Qt::Key_A);
        QSignalSpy changes(input.get(), SIGNAL(keyChanged(int,bool)));
        clear();
        QCOMPARE(changes.size(), 2);
        QCOMPARE(changes[0][0].toInt(), int(Qt::Key_A)); // Key before modifier.
        QCOMPARE(changes[1][0].toInt(), int(Qt::Key_Control));
        release(u"finger"_s);
        QCOMPARE(changes.size(), 2);
        QVERIFY(held().isEmpty());
        press(u"finger"_s, Qt::Key_A);
        input->setProperty("sticky", false);
        QVERIFY(held().isEmpty());
    }
    void extendedModeKeepsOtherOwners() {
        sticky();
        press(u"modifier"_s, Qt::Key_Shift);
        release(u"modifier"_s);
        press(u"modifier"_s, Qt::Key_Control);
        release(u"modifier"_s);
        press(u"letter"_s, Qt::Key_A);
        QVERIFY(QMetaObject::invokeMethod(input.get(), "releaseLatch", Q_ARG(int, Qt::Key_Shift)));
        QVERIFY(QMetaObject::invokeMethod(input.get(), "releaseLatch", Q_ARG(int, Qt::Key_Control)));
        QCOMPARE(held(), (QSet<int>{Qt::Key_Shift, Qt::Key_Control, Qt::Key_A}));
        release(u"letter"_s);
        QVERIFY(held().isEmpty());
    }
};
QTEST_GUILESS_MAIN(KeyboardInputTest)
#include "test_keyboard_input.moc"
