/* Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "contenturi.h"
#include "debugsymbols.h"
#include "devicestorage.h"
#include "folderlistmodel.h"
#include "pokemodel.h"
#include "romoverrides.h"
#include <QAbstractItemModelTester>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>
#include <QUrl>
#include <algorithm>
#include <string_view>
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
    void cobraRomRenamePersistence()
    {
        using namespace std::literals;

        const auto dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QByteArray legacyBytes(16384, '\x5a');
        const QByteArray replacementBytes(16384, '\x76');
        for (const auto &id : { u"cobra_64k"_s, u"cobra_80k"_s }) {
            auto legacyId = id;
            legacyId.insert(5, u'1');
            const auto legacyKey = u"CustomROMs/"_s + legacyId + u"/cobra1_boot.rom"_s;
            const auto legacyPath = dataPath + u"/ROMs/"_s + legacyId + u"/cobra1_boot.rom"_s;
            QVERIFY(QDir().mkpath(QFileInfo(legacyPath).absolutePath()));
            QFile legacyFile(legacyPath);
            QVERIFY(legacyFile.open(QIODevice::WriteOnly));
            QCOMPARE(legacyFile.write(legacyBytes), legacyBytes.size());
            legacyFile.close();
            QSettings().setValue(legacyKey, u"legacy.rom"_s);

            RomOverrides renamed;
            renamed.setMachineIndex(machine(id));
            QCOMPARE(renamed.romSlots().size(), 5);
            QCOMPARE(renamed.romSlots()[0].toMap().value(u"source"_s).toString(), u"legacy.rom"_s);
            QCOMPARE(renamed.resolve(id.toStdString(), "cobra_boot.rom"sv, 16384)[0], 0x5a);
            const auto otherId = id == u"cobra_64k"_s ? "cobra_80k"sv : "cobra_64k"sv;
            QVERIFY(renamed.resolve(otherId, "cobra_boot.rom"sv, 16384).empty());

            QVERIFY(renamed.useFile(0, write(u"replacement.rom"_s, replacementBytes)));
            RomOverrides restarted;
            restarted.setMachineIndex(machine(id));
            QCOMPARE(restarted.resolve(id.toStdString(), "cobra_boot.rom"sv, 16384)[0], 0x76);
            QVERIFY(restarted.restore(0));
            QVERIFY(!QSettings().contains(legacyKey));
            RomOverrides restored;
            QVERIFY(restored.resolve(id.toStdString(), "cobra_boot.rom"sv, 16384).empty());
        }
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
    void contentUris()
    {
        const auto tree = u"content://com.android.externalstorage.documents/tree/primary%3AGames"_s;
        const auto root = tree + u"/document/primary%3AGames"_s;
        const auto sub = tree + u"/document/primary%3AGames%2FSub"_s;
        const auto game = tree + u"/document/primary%3AGames%2FSub%2FJet%20Set%20(Willy).tap"_s;
        QVERIFY(ContentUri::isContent(tree));
        QVERIFY(!ContentUri::isContent(u"/sdcard/Games"_s));
        QCOMPARE(ContentUri::canonical(tree), root);
        QCOMPARE(ContentUri::canonical(tree + u'/'), root);
        QCOMPARE(ContentUri::canonical(u"/home/x"_s), u"/home/x"_s);
        // the same encoding as Android's Uri.encode(): "()" stay, a space doesn't
        QCOMPARE(ContentUri::canonical(tree + u"/document/primary%3AGames%2FSub%2FJet Set %28Willy%29.tap"_s), game);
        QVERIFY(ContentUri::sameTree(tree, game));
        QVERIFY(!ContentUri::sameTree(tree, u"content://com.android.externalstorage.documents/tree/primary%3AGamesX"_s));
        QVERIFY(!ContentUri::sameTree(tree, u"/sdcard/Games"_s));
        QCOMPARE(ContentUri::treeRoot(game), root);
        QCOMPARE(ContentUri::parentFolder(game), sub);
        QCOMPARE(ContentUri::parentFolder(sub), root);
        QVERIFY(ContentUri::parentFolder(root).isEmpty());
        QCOMPARE(ContentUri::name(game), u"Jet Set (Willy).tap"_s);
        QCOMPARE(ContentUri::name(tree), u"Games"_s);
        // a whole volume: its id ends with ':'
        const auto card = u"content://com.android.externalstorage.documents/tree/1A2B-3C4D%3A"_s;
        const auto cardGame = card + u"/document/1A2B-3C4D%3AGames%2Fx.z80"_s;
        QCOMPARE(ContentUri::parentFolder(cardGame), card + u"/document/1A2B-3C4D%3AGames"_s);
        QCOMPARE(ContentUri::parentFolder(card + u"/document/1A2B-3C4D%3AGames"_s), card + u"/document/1A2B-3C4D%3A"_s);
        // opaque ids (a cloud provider's): no parent, and the way down is kept by the model
        const auto opaque = u"content://com.example.docs/tree/acc%3D1%3Broot/document/acc%3D1%3Bdoc%3D42"_s;
        QVERIFY(ContentUri::parentFolder(opaque).isEmpty());
        // QDir::filePath() joins names to a tree: not a document
        QCOMPARE(ContentUri::canonical(tree + u"/Sub/x.tap"_s), tree + u"/Sub/x.tap"_s);
        QVERIFY(ContentUri::parentFolder(tree + u"/Sub/x.tap"_s).isEmpty());

        const auto document = u"content://com.android.externalstorage.documents/document/"_s;
        QCOMPARE(ContentUri::documentForPath(u"/storage/emulated/0/Spectacol/"_s), document + u"primary%3ASpectacol"_s);
        QCOMPARE(ContentUri::documentForPath(u"/sdcard/My Games"_s), document + u"primary%3AMy%20Games"_s);
        QCOMPARE(ContentUri::documentForPath(u"/storage/1a2b-3c4d/Spectacol"_s), document + u"1A2B-3C4D%3ASpectacol"_s);
        QVERIFY(ContentUri::documentForPath(u"/data/user/0/eu.licentia.games.spectacol/files"_s).isEmpty());
        QVERIFY(ContentUri::documentForPath(u"Spectacol"_s).isEmpty());
        QCOMPARE(ContentUri::displayPath(u"/storage/emulated/0/Spectacol/"_s), u"Internal storage/Spectacol"_s);
        QCOMPARE(ContentUri::displayPath(u"/storage/1A2B-3C4D/Spectacol"_s), u"1A2B-3C4D/Spectacol"_s);
    }
    void deviceFolderNavigation()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QDir(directory.path()).mkdir(u"Snapshots"_s);
        QFile tape(directory.filePath(u"Local.tap"_s));
        QVERIFY(tape.open(QIODevice::WriteOnly));
        tape.write("1234");
        tape.close();
        const auto games = u"content://com.android.externalstorage.documents/tree/primary%3AGames"_s;
        const auto gamesRoot = games + u"/document/primary%3AGames"_s;
        const auto sub = games + u"/document/primary%3AGames%2FSub"_s;
        const auto deep = games + u"/document/primary%3AGames%2FSub%2FDeep"_s;
        const auto music = u"content://com.android.externalstorage.documents/tree/primary%3AMusic"_s;

        FolderListModel model;
        QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.setRootFolder(directory.path());
        model.setHomeFolder(directory.path());
        model.setDeviceFolders({ games, music });
        QCOMPARE(model.rowCount({}), 4);
        // the device folders come first, before the folders
        QVERIFY(model.isDeviceFolder(0) && model.isDeviceFolder(1));
        QCOMPARE(model.data(model.index(0), FolderListModel::Name).toString(), u"Games"_s);
        QCOMPARE(model.path(0), gamesRoot);
        QVERIFY(model.isDir(0));
        QCOMPARE(model.data(model.index(1), FolderListModel::IsDevice).toBool(), true);
        QCOMPARE(model.path(2), directory.filePath(u"Snapshots"_s));
        QVERIFY(!model.isDeviceFolder(2));
        model.setCurrentIndex(0);

        // in: the title follows the way down
        model.setFolder(model.path(0));
        QCOMPARE(model.folder(), gamesRoot);
        QVERIFY(model.inDeviceFolder());
        QCOMPARE(model.folderTitle(), u"Games"_s);
        model.setFolder(sub);
        model.setFolder(deep);
        QCOMPARE(model.folderTitle(), u"Games/Sub/Deep"_s);
        QCOMPARE(model.folderName(), u"Deep"_s);
        model.cdUp();
        QCOMPARE(model.folder(), sub);
        model.cdUp();
        QCOMPARE(model.folder(), gamesRoot);
        // Up from the device folder: back where we came from, on its row
        model.cdUp();
        QCOMPARE(model.folder(), directory.path());
        QVERIFY(!model.inDeviceFolder());
        QCOMPARE(model.currentIndex(), 0);

        // straight into a sub folder (Quick Open remembers the last one): the ids give the way
        model.setFolder(deep);
        QCOMPARE(model.folderTitle(), u"Games/Sub/Deep"_s);
        model.cdUp();
        QCOMPARE(model.folder(), sub);
        QCOMPARE(model.folderTitle(), u"Games/Sub"_s);
        model.cdUp();
        QCOMPARE(model.folder(), gamesRoot);
        model.cdUp();
        QCOMPARE(model.folder(), directory.path());
        // opaque document ids: only the folder itself, and Up leaves the tree
        const auto cloud = u"content://com.example.docs/tree/root"_s;
        const auto cloudFolder = cloud + u"/document/f%3D42"_s;
        model.setDeviceFolders({ games, music, cloud });
        model.setFolder(cloudFolder);
        QCOMPARE(model.folder(), cloudFolder);
        QCOMPARE(model.folderName(), u"f=42"_s);
        model.cdUp();
        QCOMPARE(model.folder(), directory.path());
        model.setDeviceFolders({ games, music });

        // folders that weren't granted can't be opened
        model.setFolder(u"content://com.android.externalstorage.documents/tree/primary%3ADCIM"_s);
        QCOMPARE(model.folder(), directory.path());

        // forgotten while in it
        model.setFolder(sub);
        model.setDeviceFolders({ music });
        QCOMPARE(model.folder(), directory.path());
        QCOMPARE(model.rowCount({}), 3);

        // only the home folder lists them, and only for media
        model.setFolder(directory.filePath(u"Snapshots"_s));
        QCOMPARE(model.rowCount({}), 0);
        model.setFolder(directory.path());
        model.setFilterClass(FolderListModel::HostFiles);
        QVERIFY(!model.isDeviceFolder(0));
        QCOMPARE(model.rowCount({}), 2);
    }
    void copyTree()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(source.isValid() && destination.isValid());
        auto make = [](const QString &path, const QByteArray &bytes, const QDateTime &time = {}) {
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(bytes), bytes.size());
            QVERIFY(file.flush()); // a later write would set the time again
            if (time.isValid())
                QVERIFY(file.setFileTime(time, QFileDevice::FileModificationTime));
        };
        const QDateTime saved(QDate(2019, 5, 4), QTime(13, 14, 15), QTimeZone::UTC);
        make(source.filePath(u"Snapshots/Jetpac/1.szx"_s), "ZXST-one", saved);
        make(source.filePath(u"Downloaded/j/jet/Jetpac.tap"_s), "tape");
        make(source.filePath(u"same.sna"_s), "identical");
        make(source.filePath(u"clash.sna"_s), "from the old folder");
        QDir(source.path()).mkpath(u"Empty"_s);
        make(destination.filePath(u"same.sna"_s), "identical");
        make(destination.filePath(u"clash.sna"_s), "already here!!!!!!!");
        make(destination.filePath(u"clash_1.sna"_s), "taken too");

        QStringList shown;
        const auto result = DeviceStorage::copyTree(source.path(), destination.path(), {},
                                                    [&](int copied, const QString &name) {
                                                        QCOMPARE(copied, shown.size() + 1);
                                                        shown.append(name);
                                                    });
        QCOMPARE(result.copied, 3);
        QCOMPARE(result.skipped, 1);
        QCOMPARE(result.renamed, 1);
        QVERIFY(result.failed.isEmpty());
        QVERIFY(!result.stopped);
        QVERIFY(shown.contains(u"Snapshots/Jetpac/1.szx"_s));
        auto read = [&](const QString &name) {
            QFile file(destination.filePath(name));
            return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
        };
        QCOMPARE(read(u"Snapshots/Jetpac/1.szx"_s), "ZXST-one");
        QCOMPARE(QFileInfo(destination.filePath(u"Snapshots/Jetpac/1.szx"_s)).lastModified().toUTC(), saved);
        QCOMPARE(read(u"Downloaded/j/jet/Jetpac.tap"_s), "tape");
        QCOMPARE(read(u"clash.sna"_s), "already here!!!!!!!");
        QCOMPARE(read(u"clash_1.sna"_s), "taken too");
        QCOMPARE(read(u"clash_2.sna"_s), "from the old folder");
        QVERIFY(QFileInfo(destination.filePath(u"Empty"_s)).isDir());

        // again: everything is already there
        const auto again = DeviceStorage::copyTree(source.path(), destination.path());
        QCOMPARE(again.copied, 0);
        QCOMPARE(again.skipped, 4); // clash_2.sna is the old clash.sna

        std::stop_source stop;
        stop.request_stop();
        QTemporaryDir other;
        const auto stopped = DeviceStorage::copyTree(source.path(), other.path(), stop.get_token());
        QVERIFY(stopped.stopped);
        QCOMPARE(stopped.copied, 0);
        QVERIFY(!DeviceStorage::copyTree(source.filePath(u"missing"_s), other.path()).failed.isEmpty());
    }
    void legacyImport()
    {
        QTemporaryDir old;
        QTemporaryDir data;
        QVERIFY(old.isValid() && data.isValid());
        QDir(old.path()).mkpath(u"Snapshots/Game"_s);
        QFile snapshot(old.filePath(u"Snapshots/Game/1.szx"_s));
        QVERIFY(snapshot.open(QIODevice::WriteOnly));
        snapshot.write("ZXST");
        snapshot.close();
        QSettings().remove(u"Android"_s);
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(!offered.wait(50)); // nothing to bring over
            QVERIFY(storage.legacyFolder().isEmpty());
            QVERIFY(!storage.available()); // desktop: the whole file system is there
        }
        QSettings().setValue(u"Android/legacyDataPath"_s, old.path() + u'/');
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(offered.wait(1000));
            storage.postponeLegacyImport(false);
        }
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(offered.wait(1000)); // later is the next start
            QSignalSpy messages(&storage, &DeviceStorage::message);
            QSignalSpy importing(&storage, &DeviceStorage::importingChanged);
            storage.importLegacyFolder();
            QVERIFY(storage.importing());
            QTRY_VERIFY(!storage.importing());
            QCOMPARE(storage.importedFiles(), 1);
            QCOMPARE(importing.size(), 2);
            QCOMPARE(messages.size(), 1);
            QVERIFY(storage.importSummary().contains(u"1 file"_s));
            QVERIFY(QFile::exists(data.filePath(u"Snapshots/Game/1.szx"_s)));
        }
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(!offered.wait(50)); // done
            QVERIFY(!storage.legacyFolder().isEmpty()); // Options can still copy them again
        }
        QSettings().setValue(u"Android/legacyImport"_s, u"later"_s);
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(offered.wait(1000));
            storage.postponeLegacyImport(true);
        }
        {
            DeviceStorage storage(data.path());
            QSignalSpy offered(&storage, &DeviceStorage::offerLegacyImport);
            QVERIFY(!offered.wait(50)); // never
        }
        QSettings().remove(u"Android"_s);
    }
};
QTEST_GUILESS_MAIN(FilesFeaturesTest)
#include "test_files_features.moc"
