/* Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "debugsymbols.h"
#include "folderlistmodel.h"
#include "pokemodel.h"
#include "romoverrides.h"
#include <QAbstractItemModelTester>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <zxkit/machines/ZxNextMachine.h>

using namespace Qt::StringLiterals;
class FilesFeaturesTest : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QString write(const QString &name, const QByteArray &bytes)
    {
        const auto path = QDir(home.path()).filePath(name);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) qFatal("Cannot write fixture");
        return path;
    }
    int machine(const QString &id)
    {
        const auto ids = zxkit::MachineRegistry().availableIds();
        const auto it = std::ranges::find(ids, id.toStdString());
        return int(it - ids.begin());
    }
private slots:
    void initTestCase()
    {
        QVERIFY(home.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path() + u"/settings"_s);
        qputenv("XDG_DATA_HOME", (home.path() + u"/data"_s).toUtf8());
    }
    void trainerGroups()
    {
        auto core = zxkit::makeShared<zxkit::Emulator>(zxkit::EmulatorConfig{.machineId = "128k"});
        PokeModel model(core);
        QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        QSignalSpy errors(&model, &PokeModel::failed);
        const auto path = write(u"group.pok"_s, "NLives and energy\r\nM 8 36864 10 0\r\nZ 4 49152 256 0\r\nNBad ROM group\r\nM 8 36865 99 0\r\nZ 8 0 99 0\r\nY\r\n");
        {
            auto locked = core.writeLock();
            locked->mmu().poke(36864, 17);
            locked->mmu().bank(locked->machine().ramBankId()).page1k(64)[0] = 23;
        }
        QVERIFY(model.loadPok(path));
        QCOMPARE(model.rowCount({}), 2);
        QCOMPARE(model.requestedValues(0).size(), 1);
        QVERIFY(!model.setActive(0, true));
        QCOMPARE(core.readLock()->mmu().peek(36864), 17);
        QVERIFY(!model.setActive(0, true, {256}));
        QVERIFY(model.setActive(0, true, {42}));
        QCOMPARE(core.readLock()->mmu().peek(36864), 10);
        {
            auto locked = core.writeLock();
            QCOMPARE(locked->mmu().bank(locked->machine().ramBankId()).page1k(64)[0], 42);
            locked->ioBus().write(0x7ffd, 4);
        }
        QCOMPARE(core.readLock()->mmu().peek(49152), 42);
        QVERIFY(model.setActive(0, false));
        QCOMPARE(core.readLock()->mmu().peek(36864), 17);
        QCOMPARE(core.readLock()->mmu().peek(49152), 23);
        QVERIFY(!model.setActive(1, true)); // atomic rejection: no first-byte side effect
        QCOMPARE(core.readLock()->mmu().peek(36865), 0);
        QVERIFY(model.setActive(0, true));
        QVERIFY(!model.loadPok(path)); // active group must be explicitly disabled before replacement
        model.clear();
        QCOMPARE(core.readLock()->mmu().peek(36864), 10);
        QVERIFY(errors.count() >= 4);
    }
    void trainerDiscoveryAndValidation()
    {
        auto core = zxkit::makeShared<zxkit::Emulator>();
        PokeModel model(core);
        const auto game = write(u"games/Dizzy 4.sna"_s, "snapshot");
        const auto adjacent = write(u"games/DIZZY 4.POK"_s, "NAdjacent\nZ 8 36864 77 0\nY\n");
        const auto fallback = write(u"pokes/dizzy 4.pok"_s, "NFolder\nZ 8 36864 88 0\nY\n");
        model.setPokesFolder(QFileInfo(fallback).absolutePath());
        model.gameLoaded(game);
        QCOMPARE(model.sourcePath(), adjacent);
        QCOMPARE(model.data(model.index(0), PokeModel::Label).toString(), u"Adjacent"_s);
        QVERIFY(model.setActive(0, true));
        QCOMPARE(core.readLock()->mmu().peek(36864), 77);
        QVERIFY(model.setActive(0, false));
        QFile::remove(adjacent);
        model.gameLoaded(game);
        QCOMPARE(model.sourcePath(), fallback);
        QVERIFY(!model.loadPok(write(u"overflow.pok"_s, "NBad\nZ 8 102400 1 0\nY\n")));
        QVERIFY(!model.loadPok(write(u"overflow-byte.pok"_s, "NBad\nZ 8 36864 65536 0\nY\n")));
        QCOMPARE(model.sourcePath(), fallback);
        model.gameLoaded({});
        QCOMPARE(model.rowCount({}), 0);
        model.refreshBanks();
        QVERIFY(model.addPoke(0, 36864, 255));
        QVERIFY(model.setActive(0, false));
        QCOMPARE(core.readLock()->mmu().peek(36864), 0);
        QVERIFY(!model.addPoke(0, 0, 22));
        model.clear();
        QVERIFY(model.loadPok(write(u"indented.pok"_s, "\xEF\xBB\xBFNGroup\n  M 8 36864 44 0\n  Z 8 36865 45 0\nY\n")));
        QVERIFY(model.setActive(0, true));
        QCOMPARE(core.readLock()->mmu().peek(36864), 44);
        QCOMPARE(core.readLock()->mmu().peek(36865), 45);
        QVERIFY(model.setActive(0, false));
        core.writeLock()->selectMachine("128k");
        QVERIFY(!model.setActive(0, true));
    }
    void romPersistenceAndMachineScope()
    {
        RomOverrides model;
        model.setMachineIndex(machine(u"48k"_s));
        QCOMPARE(model.romSlots().size(), 1);
        const auto bundled = model.resolve("48k", "48.rom", 16384);
        QCOMPARE(bundled.size(), 16384u);
        QVERIFY(!model.useFile(0, write(u"short.rom"_s, "1234")));
        QCOMPARE(model.resolve("48k", "48.rom", 16384), bundled);
        const auto custom = write(u"custom.rom"_s, QByteArray(16384, '\x76'));
        QVERIFY(model.useFile(0, custom));
        QFile::remove(custom);
        QCOMPARE(model.resolve("48k", "48.rom", 16384)[0], 0x76);
        QCOMPARE(model.resolve("16k", "48.rom", 16384), bundled);
        RomOverrides restarted;
        QCOMPARE(restarted.resolve("48k", "48.rom", 16384)[0], 0x76);
        zxkit::EmulatorConfig config;
        config.machineRomResolver = [&](auto id, auto name, auto size) { return model.resolve(id, name, size); };
        zxkit::Emulator core(config);
        QCOMPARE(core.mmu().peek(0), 0x76);
        core.selectMachine("16k");
        QCOMPARE(core.mmu().peek(0), bundled[0]);
        model.setMachineIndex(machine(u"128k"_s));
        QCOMPARE(model.romSlots().size(), 2);
        QVERIFY(model.useFile(1, write(u"128.rom"_s, QByteArray(16384, '\x55'))));
        core.selectMachine("128k");
        QCOMPARE(core.mmu().bank(core.machine().romBankId()).bytes()[16384], 0x55);
        QVERIFY(model.restoreMachine());
        QCOMPARE(model.resolve("48k", "48.rom", 16384)[0], 0x76);
        model.setMachineIndex(machine(u"48k"_s));
        QVERIFY(model.restore(0));
        QCOMPARE(model.resolve("48k", "48.rom", 16384), bundled);
        RomOverrides restored;
        QCOMPARE(restored.resolve("48k", "48.rom", 16384), bundled);
        model.setMachineIndex(machine(u"next"_s));
        QCOMPARE(model.romSlots().size(), 4);
        QCOMPARE(model.romSlots()[0].toMap().value(u"size"_s).toInt(), 65536);
        QVERIFY(model.useFile(0, write(u"next.rom"_s, QByteArray(65536, '\x5a'))));
        config.machineId = "next";
        config.romResolver = [](auto, auto size) { return std::vector<uint8_t>(size, 0); };
        zxkit::Emulator next(config);
        QCOMPARE(next.mmu().bank(next.machine().romBankId()).bytes()[0], 0x5a);
        QVERIFY(model.restoreMachine());
    }
    void symbolsAndSourceMappings()
    {
        DebugSymbols model;
        QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        zxkit::Emulator core({.machineId = "next"});
        auto &regs = static_cast<zxkit::ZxNextMachine &>(core.machine()).nextReg();
        regs.write(0x54, 14);
        const auto path = write(u"project.sld.txt"_s,
            "|SLD.data.version|1\r\nmain.asm|1||0|-1|-1|Z|pages.size:8192,pages.count:224,slots.count:8\r\n"
            "main.asm|42:3:9|macro.asm|8|14|32768|T|\r\nmain.asm|42||0|14|32768|L|game,MainLoop,,+used\r\n"
            "main.asm|56||0|18|32768|T|\r\nmain.asm|56||0|18|32768|L|game,OtherBank,,+used\r\n"
            "main.asm|57||0|-1|999999|L|,CONST,,+equ\r\n");
        QVERIFY2(model.load(path), qPrintable(model.error()));
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.resolve(core, u"game.MainLoop"_s), 32768);
        QCOMPARE(model.resolve(core, u"game.OtherBank"_s), -2);
        QVERIFY(model.annotation(core, 32768).contains(u"main.asm:42"_s));
        QVERIFY(!model.annotation(core, 32768).contains(u"OtherBank"_s));
        regs.write(0x54, 18);
        QCOMPARE(model.resolve(core, u"game.MainLoop"_s), -2);
        QVERIFY(model.annotation(core, 32768).contains(u"main.asm:56"_s));
        model.setSearch(u"other"_s);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.rowAddress(core, 0), 32768);
        QVERIFY(!model.load(write(u"broken.sld"_s, "main.asm|1||0|0|65536|L|,bad\n")));
        QCOMPARE(model.path(), path);
        QVERIFY(!model.load(write(u"empty-label.sld"_s,
            "game.asm|1||0|14|32768|L|,Valid\ngame.asm|2||0|14|32768|L|,,\n")));
        QCOMPARE(model.path(), path);
        QVERIFY(model.load(write(u"labels.sym"_s, "Start: EQU 0x8000\nLoop = $8010\nDecimal EQU 32768\nBig: EQU 0x100000\n")));
        QCOMPARE(model.count(), 3);
        QCOMPARE(model.resolve(core, u"Loop"_s), 0x8010);
        QVERIFY(model.annotation(core, 0x8010).contains(u"Loop"_s));
        QVERIFY(!model.load(write(u"invalid.sym"_s, "invalid garbage data\n")));
        QCOMPARE(model.count(), 3);
        model.clear();
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.annotation(core, 0x8010).isEmpty());
        QVERIFY(model.load(write(u"hex-label.sld"_s, "main.asm|1||0|-1|-1|Z|pages.size:8192,pages.count:224,slots.count:8\nmain.asm|42||0|14|32769|L|,FACE,,+used\n")));
        QCOMPARE(model.resolve(core, u"FACE"_s), -2); // known banked labels cannot fall back to a hex address
        regs.write(0x54, 14);
        QCOMPARE(model.resolve(core, u"FACE"_s), 32769);
        QCOMPARE(model.resolve(core, u"unknown"_s), -1);
    }
    void folderSearchAndSelection()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto make = [&](const QString &name) { QFile file(directory.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("1234"); };
        make(u"Alpha.tap"_s); make(u"Zeta.sna"_s); make(u"Alpha.pok"_s); make(u"ignored.txt"_s);
        QDir(directory.path()).mkdir(u"child"_s);
        {
            FolderListModel model;
            QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
            model.setRootFolder(directory.path());
            QCOMPARE(model.rowCount({}), 3);
            QCOMPARE(model.path(1), directory.filePath(u"Alpha.tap"_s));
            model.setCurrentIndex(2);
            model.setSearch(u"aLpHa"_s);
            QCOMPARE(model.rowCount({}), 1);
            QCOMPARE(model.path(0), directory.filePath(u"Alpha.tap"_s));
            model.setSearch({});
            QCOMPARE(model.currentIndex(), 2);
            model.setFolder(directory.filePath(u"child"_s));
            model.cdUp();
            QCOMPARE(model.currentIndex(), 2);
        }
        FolderListModel restarted;
        restarted.setRootFolder(directory.path());
        QCOMPARE(restarted.currentIndex(), 2);
        restarted.setFolder(directory.path() + u"-sibling"_s);
        QCOMPARE(restarted.folder(), directory.path());
        restarted.setFolder(QUrl::fromLocalFile(directory.path()).toString());
        QCOMPARE(restarted.folder(), directory.path());
        restarted.setFilterClass(FolderListModel::HostFiles);
        QCOMPARE(restarted.rowCount({}), 5);
        restarted.setSearch(u"ignored"_s);
        QCOMPARE(restarted.rowCount({}), 1);
        restarted.setCurrentIndex(0);
        restarted.setSearch({});
        QCOMPARE(restarted.currentIndex(), 0); // filtered navigation did not overwrite folder selection
        restarted.setCurrentIndex(0); // explicitly selecting the first row must persist it too
        const auto first = restarted.path(0);
        QDir(directory.path()).mkdir(u"AAA"_s);
        restarted.refresh();
        QCOMPARE(restarted.path(restarted.currentIndex()), first);
        QCOMPARE(restarted.currentIndex(), 1);
        restarted.setSearch(u"ignored"_s);
        restarted.rememberSelection(0); // opening a filtered result is an explicit selection
        restarted.setSearch({});
        QCOMPARE(restarted.path(restarted.currentIndex()), directory.filePath(u"ignored.txt"_s));
    }
};
QTEST_GUILESS_MAIN(FilesFeaturesTest)
#include "test_files_features.moc"
