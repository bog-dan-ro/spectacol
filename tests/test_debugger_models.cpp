/* Copyright (c) 2026, BogDan Vatra <bogdan@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "debuggermodels.h"
#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>
#include <zxkit/machines/ZxNextMachine.h>

using namespace Qt::StringLiterals;
class DebuggerModelsTest : public QObject {
    Q_OBJECT
private slots:
    void memory()
    {
        auto core = zxkit::makeShared<zxkit::Emulator>();
        DebuggerMemoryModel model(core);
        QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QCOMPARE(model.rowCount(), 16);
        QCOMPARE(model.rowCount(model.index(0)), 0);
        QVERIFY(!model.data({}, DebuggerMemoryModel::Cells).isValid());
        QVERIFY(model.goTo(u"$8001"_s));
        QCOMPARE(model.address(), 0x8001);
        QVERIFY(model.writeByte(u"A5"_s));
        QCOMPARE(core.readLock()->mmu().peek(0x8001), 0xA5);
        model.setEditable(false);
        QVERIFY(!model.writeByte(u"12"_s));
        QCOMPARE(core.readLock()->mmu().peek(0x8001), 0xA5);
        model.setEditable(true);
        QVERIFY(!model.goTo(u"10000"_s));
        QVERIFY(!model.writeByte(u"100"_s));
        QVERIFY(model.goTo(u"0xFFFF"_s));
        model.moveCursor(2147483647);
        QCOMPARE(model.address(), 65535);
        model.moveCursor(-2147483647);
        QCOMPARE(model.address(), 0);
        const auto old = core.readLock()->mmu().peek(0);
        QVERIFY(!model.writeByte(u"45"_s));
        QCOMPARE(core.readLock()->mmu().peek(0), old);
        QVERIFY(model.writeByte(u"45"_s, true));
        QCOMPARE(core.readLock()->mmu().peek(0), 0x45);
        const auto ram = core.readLock()->machine().ramBankId();
        model.setBankId(ram);
        QVERIFY(model.goTo(u"BFFF"_s));
        QVERIFY(model.writeByte(u"76"_s));
        QCOMPARE(core.readLock()->mmu().bank(ram).page1k(47)[1023], 0x76);
        model.setColumns(4);
        QCOMPARE(model.rowCount(), 64);
        model.setColumns(8);
        QCOMPARE(model.rowCount(), 32);
        model.setColumns(16);
        model.setCursor(-1);
        QCOMPARE(model.cursor(), 255);
        QVERIFY(!model.goTo(u"C000"_s));
        core.writeLock()->selectMachine("next");
        model.refresh();
        model.setBankId(core.readLock()->machine().ramBankId());
        const auto ramSize = [&] { const auto locked=core.readLock(); return locked->mmu().bank(locked->machine().ramBankId()).size(); }();
        QVERIFY(model.goTo(QString::number(ramSize-1,16)));
        QVERIFY(model.writeByte(u"E3"_s));
        core.writeLock()->selectMachine("48k");
        model.refresh();
        QVERIFY(model.address() < 49152);
    }
    void rawState()
    {
        auto core = zxkit::makeShared<zxkit::Emulator>();
        DebuggerStateModel next(core, DebuggerStateModel::Kind::NextRegisters);
        DebuggerStateModel mmu(core, DebuggerStateModel::Kind::Mmu);
        QAbstractItemModelTester nextContract(&next, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QAbstractItemModelTester mmuContract(&mmu, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QVERIFY(!next.available());
        QCOMPARE(next.rowCount(), 0);
        QVERIFY(mmu.available());
        QCOMPARE(mmu.rowCount(), 64);
        QVERIFY(mmu.data(mmu.index(0), DebuggerStateModel::Details).toString().contains(u"Read-only"_s));
        core.writeLock()->selectMachine("next");
        unsigned reads = 0;
        {
            auto locked = core.writeLock();
            auto& regs = static_cast<zxkit::ZxNextMachine&>(locked->machine()).nextReg();
            regs.setReadHandler(0x80, [&](uint8_t, uint8_t) {
                ++reads;
                return uint8_t(99);
            });
            regs.write(0x80, 0x42);
            regs.write(0x56, 20);
        }
        next.refresh();
        mmu.refresh();
        QVERIFY(next.available());
        QCOMPARE(next.rowCount(), 256);
        QCOMPARE(reads, 0u);
        QCOMPARE(next.data(next.index(0x80), DebuggerStateModel::ValueText).toString(), u"$42"_s);
        QVERIFY(mmu.data(mmu.index(48), DebuggerStateModel::ValueText).toString().contains(u"028000"_s));
        QCOMPARE(next.rowCount(next.index(0)), 0);
        core.writeLock()->selectMachine("128k");
        next.refresh();
        mmu.refresh();
        QVERIFY(!next.available());
        QCOMPARE(next.rowCount(), 0);
        QCOMPARE(mmu.rowCount(), 64);
    }
};
QTEST_GUILESS_MAIN(DebuggerModelsTest)
#include "test_debugger_models.moc"
