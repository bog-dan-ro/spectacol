/* Copyright (c) 2015-2026, BogDan Vatra. SPDX-License-Identifier: GPL-3.0-or-later */

#include "filemanagerpanel.h"

#include <zxkit/media/BasicCodec.h>
#include <zxkit/media/MediaArchive.h>

#include <QAbstractItemModelTester>
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <string>
#include <string_view>
#include <vector>

using namespace Qt::StringLiterals;
using namespace std::literals;

namespace {

void writeFile(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(bytes), bytes.size());
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

zxkit::MediaArchive openImage(const QString &path)
{
    const auto bytes = readFile(path);
    return zxkit::MediaArchive::open(std::span(reinterpret_cast<const uint8_t *>(bytes.constData()),
                                               std::size_t(bytes.size())),
                                     (u'.' + QFileInfo(path).suffix()).toStdString());
}

int formatIndex(const FileManagerPanel &panel, const QString &suffix,
                const QString &filesystem = {})
{
    const auto formats = panel.imageFormats();
    for (int i = 0; i < formats.size(); ++i) {
        const auto format = formats[i].toMap();
        if (format[u"suffix"_s] == suffix && format[u"filesystem"_s].toString() == filesystem)
            return i;
    }
    return -1;
}

QStringList names(const FileManagerPanel &panel)
{
    QStringList result;
    for (int i = 0; i < panel.rowCount(); ++i)
        result.append(panel.data(panel.index(i), FileManagerPanel::Name).toString());
    return result;
}

QString pageTitles(const QVariantMap &view)
{
    QStringList titles;
    for (const auto &page : view[u"pages"_s].toList())
        titles.append(page.toMap()[u"title"_s].toString());
    return titles.join(u',');
}

} // namespace

class FileManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void hostFolders()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        FileManagerPanel left;
        FileManagerPanel right;
        QAbstractItemModelTester contract(&left,
                                          QAbstractItemModelTester::FailureReportingMode::QtTest);
        left.setPeer(&right);
        right.setPeer(&left);
        QSignalSpy failures(&left, &FileManagerPanel::failed);
        left.setFolder(root.path());
        QVERIFY(left.makeFolder(u"games"_s));
        QVERIFY(!left.makeFolder(u"a/b"_s));
        QCOMPARE(failures.count(), 1);
        writeFile(root.filePath(u"games/readme.txt"_s), "10 PRINT 1\n");
        QVERIFY(left.makeFolder(u"games/inner"_s) == false);
        QDir(root.path()).mkpath(u"games/inner"_s);
        writeFile(root.filePath(u"games/inner/deep.bin"_s), QByteArray(10, 'x'));
        left.refresh();
        QCOMPARE(names(left), QStringList({ u".."_s, u"games"_s }));

        // Copy the folder, recursively, into the other panel's folder
        QDir(root.path()).mkpath(u"copy"_s);
        right.setFolder(root.filePath(u"copy"_s));
        QVERIFY(left.copyTo(left.rowOf(u"games"_s), &right));
        QCOMPARE(readFile(root.filePath(u"copy/games/inner/deep.bin"_s)), QByteArray(10, 'x'));
        QCOMPARE(left.conflicts(left.rowOf(u"games"_s), &right), QStringList{ u"games"_s });
        QVERIFY(!left.copyTo(left.rowOf(u"games"_s), &right));
        // Into itself, or onto the same folder
        QVERIFY(!left.copyTo(left.rowOf(u"games"_s), &left));

        // Rename, view and edit a text file
        QVERIFY(left.open(left.rowOf(u"games"_s)));
        QVERIFY(left.rename(left.rowOf(u"readme.txt"_s), u"listing.bas"_s));
        const auto row = left.rowOf(u"listing.bas"_s);
        QVERIFY(row > 0);
        QCOMPARE(left.editable(row)[u"text"_s].toString(), u"10 PRINT 1\n"_s);
        QVERIFY(left.saveText(row, u"10 PRINT 2\n"_s));
        QCOMPARE(readFile(root.filePath(u"games/listing.bas"_s)), QByteArray("10 PRINT 2\n"));
        QCOMPARE(pageTitles(left.view(row)), u"Text,Hex"_s);
        const auto games = left.up();
        QCOMPARE(games, left.rowOf(u"games"_s));

        // Data larger than a screen can still be looked at as one
        writeFile(root.filePath(u"big.bin"_s), QByteArray(7000, 0));
        left.refresh();
        QCOMPARE(pageTitles(left.view(left.rowOf(u"big.bin"_s))), u"Hex,As screen"_s);
        QVERIFY(left.remove(left.rowOf(u"big.bin"_s)));

        // Marked entries move together; a folder is removed with its contents
        writeFile(root.filePath(u"one.bin"_s), "1");
        writeFile(root.filePath(u"two.bin"_s), "2");
        left.refresh();
        left.toggleMark(left.rowOf(u"one.bin"_s));
        left.toggleMark(left.rowOf(u"two.bin"_s));
        QCOMPARE(left.markedCount(), 2);
        QCOMPARE(left.targetNames(0), QStringList({ u"one.bin"_s, u"two.bin"_s }));
        QVERIFY(left.copyTo(0, &right, true));
        QCOMPARE(left.markedCount(), 0);
        QCOMPARE(left.rowOf(u"one.bin"_s), -1);
        QCOMPARE(readFile(root.filePath(u"copy/two.bin"_s)), QByteArray("2"));
        QVERIFY(left.remove(left.rowOf(u"games"_s)));
        QVERIFY(!QFileInfo::exists(root.filePath(u"games"_s)));
    }

    void newImages()
    {
        QTemporaryDir root;
        FileManagerPanel panel;
        panel.setFolder(root.path());
        const auto formats = panel.imageFormats();
        for (int i = 0; i < formats.size(); ++i) {
            const auto name = u"image%1"_s.arg(i);
            QVERIFY2(panel.createImage(name, i),
                     qPrintable(formats[i].toMap()[u"text"_s].toString()));
            const auto file = name + u'.' + formats[i].toMap()[u"suffix"_s].toString();
            QVERIFY(!panel.createImage(name, i));
            QVERIFY(panel.open(panel.rowOf(file)));
            QVERIFY2(panel.inMedia(), qPrintable(file));
            const auto row = panel.up();
            QCOMPARE(row, panel.rowOf(file));
        }
    }

    void tapes()
    {
        QTemporaryDir root;
        FileManagerPanel host;
        FileManagerPanel tape;
        host.setPeer(&tape);
        tape.setPeer(&host);
        QSignalSpy failures(&tape, &FileManagerPanel::failed);
        host.setFolder(root.path());
        tape.setFolder(root.path());
        QByteArray screen(6912, 0);
        screen[6144] = char(0x38);
        writeFile(root.filePath(u"title.scr"_s), screen);
        writeFile(root.filePath(u"hello.bas"_s), "10 PRINT \"HELLO\"\n20 GO TO 10\n");
        QVERIFY(tape.createImage(u"game"_s, formatIndex(tape, u"tzx"_s)));
        QVERIFY(tape.open(tape.rowOf(u"game.tzx"_s)));
        QVERIFY(tape.inMedia());
        QCOMPARE(tape.filesystem(), u"tape"_s);
        QCOMPARE(names(tape), QStringList({ u".."_s, u"files"_s, u"blocks"_s, u"basic"_s }));

        // Host files land in files/ (a screen keeps its address), BASIC text is compiled
        QVERIFY(host.copyTo(host.rowOf(u"title.scr"_s), &tape));
        QVERIFY(host.copyTo(host.rowOf(u"hello.bas"_s), &tape));
        auto image = openImage(root.filePath(u"game.tzx"_s));
        QCOMPARE(image.read("files/title.scr"sv).size(), 6912u);
        const auto entries = image.list();
        const auto screenEntry =
                std::ranges::find(entries, "files/title.scr"s, &zxkit::MediaEntry::name);
        QVERIFY(screenEntry != entries.end());
        QCOMPARE(screenEntry->attributes.at("param1"s), 16384u);
        QCOMPARE(image.read("basic/hello.bas"sv), image.read("basic/hello.bas"sv));
        QVERIFY(!image.read("files/hello"sv).empty());

        QVERIFY(tape.open(tape.rowOf(u"files"_s)));
        QCOMPARE(names(tape), QStringList({ u".."_s, u"title.scr"_s, u"hello"_s }));
        const auto screenRow = tape.rowOf(u"title.scr"_s);
        QCOMPARE(tape.data(tape.index(screenRow), FileManagerPanel::Kind).toString(), u"screen"_s);
        QCOMPARE(pageTitles(tape.view(screenRow)), u"Screen,Hex,Disassembly"_s);
        QVERIFY(tape.view(screenRow)[u"pages"_s]
                        .toList()[0]
                        .toMap()[u"image"_s]
                        .toString()
                        .startsWith(u"data:image/png;base64,"_s));
        const auto basicRow = tape.rowOf(u"hello"_s);
        QCOMPARE(tape.data(tape.index(basicRow), FileManagerPanel::Kind).toString(), u"basic"_s);
        QCOMPARE(pageTitles(tape.view(basicRow)), u"Unhacked,Unfiltered,Hex"_s);
        // BASIC comes in parts to highlight; the others say how to colour them
        const auto basicPage = tape.view(basicRow)[u"pages"_s].toList()[0].toMap();
        QCOMPARE(basicPage[u"style"_s].toString(), u"basic"_s);
        const auto firstLine = basicPage[u"parts"_s].toList()[0].toList();
        QCOMPARE(firstLine[0].toMap()[u"k"_s].toString(), u"line"_s);
        QCOMPARE(firstLine[1].toMap()[u"k"_s].toString(), u"keyword"_s);
        QCOMPARE(firstLine[1].toMap()[u"t"_s].toString(), u"PRINT"_s);
        const auto unfiltered = tape.view(basicRow)[u"pages"_s].toList()[1].toMap();
        QCOMPARE(unfiltered[u"style"_s].toString(), u"lossless"_s);
        QVERIFY(unfiltered[u"lines"_s].toStringList()[0].startsWith(u"10 \\{PRINT}"_s));
        QCOMPARE(tape.view(basicRow)[u"pages"_s].toList()[2].toMap()[u"style"_s].toString(),
                 u"hex"_s);
        QVERIFY(tape.view(basicRow)[u"pages"_s]
                        .toList()[0]
                        .toMap()[u"lines"_s]
                        .toStringList()
                        .contains(u"  10 PRINT \"HELLO\""_s));

        // A protected loader shows the values it uses, and the decoys LIST shows
        auto protectedTape = openImage(root.filePath(u"game.tzx"_s));
        const auto loader =
                zxkit::BasicCodec::compile("10 \\{CLEAR}99999\\x0E\\x00\\x00\\xC5\\x62\\x00\n"sv);
        protectedTape.put("files/loader"sv, loader, { { "type"s, 0 } });
        const auto protectedBytes = protectedTape.encode();
        writeFile(root.filePath(u"game.tzx"_s),
                  QByteArray(reinterpret_cast<const char *>(protectedBytes.data()),
                             qsizetype(protectedBytes.size())));
        tape.refresh();
        const auto loaderPage = tape.view(tape.rowOf(u"loader"_s))[u"pages"_s].toList()[0].toMap();
        QCOMPARE(loaderPage[u"lines"_s].toStringList(), QStringList{ u"  10 CLEAR 25285"_s });
        const auto decoy = loaderPage[u"parts"_s].toList()[0].toList()[3].toMap();
        QCOMPARE(decoy[u"t"_s].toString(), u"25285"_s);
        QCOMPARE(decoy[u"l"_s].toString(), u"99999"_s);
        QVERIFY(tape.remove(tape.rowOf(u"loader"_s)));
        // Code in a REM run by a second line 0: its USR address gets a disassembly
        const std::vector<uint8_t> remLoader{ 0,    0,    9,    0,    0xEA, 0x10, 6,    'H', 'I',
                                              0xF3, 0xC9, 0,    0x0D, 0,    0,    10,   0,   0xF5,
                                              0xC0, '0',  0x0E, 0,    0,    0xD4, 0x5C, 0,   0x0D };
        protectedTape = openImage(root.filePath(u"game.tzx"_s));
        protectedTape.put("files/remcode"sv, remLoader, { { "type"s, 0 } });
        const auto remBytes = protectedTape.encode();
        writeFile(root.filePath(u"game.tzx"_s),
                  QByteArray(reinterpret_cast<const char *>(remBytes.data()),
                             qsizetype(remBytes.size())));
        tape.refresh();
        const auto remView = tape.view(tape.rowOf(u"remcode"_s));
        QCOMPARE(pageTitles(remView), u"Unhacked,Unfiltered,USR 23764,Hex"_s);
        QCOMPARE(remView[u"pages"_s].toList()[0].toMap()[u"lines"_s].toStringList(),
                 QStringList({ u"   0 REM {INK 6}HI {machine code: 3 bytes at 23764}"_s,
                               u"   0 PRINT USR 23764"_s }));
        QVERIFY(remView[u"pages"_s].toList()[2].toMap()[u"lines"_s].toStringList()[0].startsWith(
                u"5CD4  F3"_s));
        QVERIFY(tape.remove(tape.rowOf(u"remcode"_s)));

        // Edit the program, then its attributes and name
        const auto listing = tape.editable(basicRow)[u"text"_s].toString();
        QVERIFY(listing.contains(u"\\{GO TO} 10"_s));
        QVERIFY(tape.saveText(basicRow, u"10 PRINT \"BYE\"\n"_s));
        QVERIFY(tape.view(tape.rowOf(u"hello"_s))[u"pages"_s]
                        .toList()[0]
                        .toMap()[u"lines"_s]
                        .toStringList()
                        .contains(u"  10 PRINT \"BYE\""_s));
        const auto fields = tape.attributeFields(tape.rowOf(u"hello"_s));
        QStringList keys;
        for (const auto &field : fields)
            keys.append(field.toMap()[u"key"_s].toString());
        QCOMPARE(keys, QStringList({ u"type"_s, u"param1"_s, u"param2"_s, u"pause"_s }));
        QVERIFY(tape.setAttributes(tape.rowOf(u"hello"_s),
                                   { { u"param1"_s, 10 }, { u"pause"_s, 500 } }));
        image = openImage(root.filePath(u"game.tzx"_s));
        const auto edited = image.list();
        const auto program = std::ranges::find(edited, "files/hello"s, &zxkit::MediaEntry::name);
        QCOMPARE(program->attributes.at("param1"s), 10u);
        QCOMPARE(program->attributes.at("pause"s), 500u);
        QVERIFY(tape.rename(tape.rowOf(u"hello"_s), u"intro"_s));
        QVERIFY(tape.rowOf(u"intro"_s) > 0);
        QVERIFY(!tape.rename(tape.rowOf(u"intro"_s), u"much too long"_s));

        // Out to the host
        host.refresh();
        QVERIFY(tape.copyTo(tape.rowOf(u"title.scr"_s), &host, false, u"copy.scr"_s));
        QCOMPARE(readFile(root.filePath(u"copy.scr"_s)), screen);

        // Blocks: four of them; reorder, copy one onto another tape, remove one
        const auto files = tape.up();
        QCOMPARE(files, tape.rowOf(u"files"_s));
        QVERIFY(tape.open(tape.rowOf(u"blocks"_s)));
        QVERIFY(tape.tapeBlocks());
        QCOMPARE(tape.rowCount(), 5);
        // The screen's data block (flag, 6912 bytes, checksum) shows as a screen; its header doesn't
        QCOMPARE(pageTitles(tape.view(2)), u"Screen,Block,Hex,Raw TZX"_s);
        QCOMPARE(pageTitles(tape.view(1)), u"Block,Hex,Raw TZX"_s);
        // Rows show the block's number and contents
        const auto label = [&](int row) {
            return tape.data(tape.index(row), FileManagerPanel::Label).toString();
        };
        const auto header = label(1);
        QCOMPARE(header, u"#0 Bytes: title.scr"_s);
        QCOMPARE(tape.data(tape.index(1), FileManagerPanel::Name).toString(), u"000000.tzx"_s);
        QCOMPARE(tape.moveBlock(1, 2), 3);
        QCOMPARE(label(3), u"#2 Bytes: title.scr"_s);
        QCOMPARE(tape.moveBlock(3, -2), 1);
        QCOMPARE(label(1), header);
        QCOMPARE(tape.moveBlock(1, -5), 1);
        QVERIFY(host.createImage(u"other"_s, formatIndex(host, u"tap"_s)));
        QVERIFY(host.open(host.rowOf(u"other.tap"_s)));
        QVERIFY(host.open(host.rowOf(u"blocks"_s)));
        tape.toggleMark(1);
        tape.toggleMark(2);
        QVERIFY(tape.copyTo(1, &host));
        QCOMPARE(host.rowCount(), 3);
        QVERIFY(tape.copyTo(1, &host));
        QCOMPARE(host.rowCount(), 4);
        QCOMPARE(openImage(root.filePath(u"other.tap"_s)).read("files/title.scr"sv).size(), 6912u);
        QVERIFY(tape.remove(4));
        QCOMPARE(tape.rowCount(), 4);
        QVERIFY(openImage(root.filePath(u"game.tzx"_s)).list().size() > 0);
        // A TAP tape's blocks are one-block TAP files: the viewer reads them, a TZX tape takes them
        QCOMPARE(host.data(host.index(1), FileManagerPanel::Name).toString(), u"000000.tap"_s);
        QCOMPARE(pageTitles(host.view(1)), u"Block,Hex,Raw TAP"_s);
        QVERIFY(host.copyTo(1, &tape));
        QCOMPARE(tape.data(tape.index(4), FileManagerPanel::Name).toString(), u"000003.tzx"_s);
        QCOMPARE(tape.data(tape.index(4), FileManagerPanel::Label).toString(),
                 host.data(host.index(1), FileManagerPanel::Label)
                         .toString()
                         .replace(u"#0"_s, u"#3"_s));
        QCOMPARE(failures.count(), 1); // the long name
    }

    void sharedImage()
    {
        QTemporaryDir root;
        FileManagerPanel left;
        FileManagerPanel right;
        left.setPeer(&right);
        right.setPeer(&left);
        left.setFolder(root.path());
        right.setFolder(root.path());
        QVERIFY(left.createImage(u"disk"_s, formatIndex(left, u"dsk"_s, u"plus3"_s)));
        writeFile(root.filePath(u"code.bin"_s), QByteArray(300, char(0xC9)));
        right.refresh();
        QVERIFY(left.open(left.rowOf(u"disk.dsk"_s)));
        QVERIFY(right.open(right.rowOf(u"disk.dsk"_s)));
        QCOMPARE(left.filesystem(), u"plus3"_s);
        left.up();
        QVERIFY(left.copyTo(left.rowOf(u"code.bin"_s), &right));
        QVERIFY(left.open(left.rowOf(u"disk.dsk"_s)));
        // Both panels show one image: a copy inside it is seen by the other at once
        QVERIFY(left.copyTo(left.rowOf(u"CODE.BIN"_s), &right, false, u"COPY.BIN"_s));
        QVERIFY(right.rowOf(u"COPY.BIN"_s) > 0);
        QVERIFY(left.rowOf(u"COPY.BIN"_s) > 0);
        QVERIFY(!left.copyTo(left.rowOf(u"CODE.BIN"_s), &right));
        QVERIFY(left.copyTo(left.rowOf(u"COPY.BIN"_s), &right, true, u"MOVED.BIN"_s));
        QCOMPARE(left.rowOf(u"COPY.BIN"_s), -1);
        // Without a +3DOS header, CP/M rounds the file up to 128-byte records
        auto moved = openImage(root.filePath(u"disk.dsk"_s)).read("MOVED.BIN"sv);
        QCOMPARE(moved.size(), 384u);
        moved.resize(300);
        QCOMPARE(moved, std::vector<uint8_t>(300, 0xC9));
        QVERIFY(left.setAttributes(left.rowOf(u"MOVED.BIN"_s), { { u"readonly"_s, 1 } }));
        QVERIFY(!left.remove(left.rowOf(u"MOVED.BIN"_s)));
        QVERIFY(left.setAttributes(left.rowOf(u"MOVED.BIN"_s), { { u"readonly"_s, 0 } }));
        QVERIFY(left.remove(left.rowOf(u"MOVED.BIN"_s)));
    }

    void trdos()
    {
        QTemporaryDir root;
        FileManagerPanel tape;
        FileManagerPanel disk;
        tape.setPeer(&disk);
        disk.setPeer(&tape);
        tape.setFolder(root.path());
        disk.setFolder(root.path());
        QVERIFY(tape.createImage(u"source"_s, formatIndex(tape, u"tap"_s)));
        QVERIFY(disk.createImage(u"target"_s, formatIndex(disk, u"trd"_s, u"trdos"_s)));
        auto source = openImage(root.filePath(u"source.tap"_s));
        const auto program = zxkit::BasicCodec::compile("10 PRINT 1\n"sv);
        source.put("files/loader"sv, program, { { "type"s, 0 }, { "param1"s, 10 } });
        source.put("files/code"sv, std::vector<uint8_t>(100, 1),
                   { { "type"s, 3 }, { "param1"s, 40000 } });
        const auto bytes = source.encode();
        writeFile(
                root.filePath(u"source.tap"_s),
                QByteArray(reinterpret_cast<const char *>(bytes.data()), qsizetype(bytes.size())));
        tape.refresh();
        disk.refresh();
        QVERIFY(tape.open(tape.rowOf(u"source.tap"_s)));
        QVERIFY(tape.open(tape.rowOf(u"files"_s)));
        QVERIFY(disk.open(disk.rowOf(u"target.trd"_s)));
        QCOMPARE(disk.filesystem(), u"trdos"_s);
        tape.markAll(true);
        QCOMPARE(tape.conflicts(0, &disk), QStringList());
        QVERIFY(tape.copyTo(0, &disk));
        QCOMPARE(names(disk), QStringList({ u".."_s, u"loader.B"_s, u"code.C"_s }));
        const auto code = disk.rowOf(u"code.C"_s);
        QCOMPARE(disk.data(disk.index(code), FileManagerPanel::Info).toString(), u"Code 40000"_s);
        QCOMPARE(
                disk.data(disk.index(disk.rowOf(u"loader.B"_s)), FileManagerPanel::Kind).toString(),
                u"basic"_s);
        QCOMPARE(disk.conflicts(code, &tape), QStringList());
        // and back: the letter becomes a tape type
        QVERIFY(disk.copyTo(code, &tape, false, u"code2"_s));
        const auto entries = openImage(root.filePath(u"source.tap"_s)).list();
        const auto back = std::ranges::find(entries, "files/code2"s, &zxkit::MediaEntry::name);
        QVERIFY(back != entries.end());
        QCOMPARE(back->attributes.at("type"s), 3u);
        QCOMPARE(back->attributes.at("param1"s), 40000u);
    }

    // The Next's SD card: a FAT image edited in place, holding the CP/M drive as a P3D image
    void imagesInsideCards()
    {
        QTemporaryDir root;
        const auto save = [&](const QString &name, const std::vector<uint8_t> &bytes) {
            writeFile(root.filePath(name),
                      QByteArray(reinterpret_cast<const char *>(bytes.data()),
                                 qsizetype(bytes.size())));
        };
        auto card = zxkit::MediaArchive::create(".img"sv, { .filesystem = "fat"s });
        card.makeDirectory("nextzxos"sv);
        card.put("nextzxos/cpm-a.p3d"sv, zxkit::MediaArchive::create(".p3d"sv).encode());
        save(u"card.img"_s, card.encode());
        auto hc = zxkit::MediaArchive::create(".dsk"sv, { .filesystem = "cpm-720"s });
        const std::vector<uint8_t> app(1000, 0xC9);
        hc.put("APP.COM"sv, app);
        save(u"hc.dsk"_s, hc.encode());
        const auto cardSize = QFileInfo(root.filePath(u"card.img"_s)).size();

        FileManagerPanel left;
        FileManagerPanel right;
        left.setPeer(&right);
        right.setPeer(&left);
        left.setFolder(root.path());
        right.setFolder(root.path());
        QSignalSpy written(&left, &FileManagerPanel::mediaWritten);
        QSignalSpy failures(&left, &FileManagerPanel::failed);
        QVERIFY(left.open(left.rowOf(u"card.img"_s)));
        QCOMPARE(left.filesystem(), u"fat"_s);
        QCOMPARE(left.hostPath(0), QString());
        QVERIFY(left.open(left.rowOf(u"nextzxos"_s)));
        const auto image = left.rowOf(u"cpm-a.p3d"_s);
        QCOMPARE(left.data(left.index(image), FileManagerPanel::IsMedia).toBool(), true);
        QVERIFY(left.open(image));
        QCOMPARE(left.filesystem(), u"next-cpm"_s);
        QVERIFY(left.location().endsWith(u"card.img › nextzxos/cpm-a.p3d"_s));

        // An HC CP/M app onto the Next's CP/M drive: the P3D goes back into the card, in place
        QVERIFY(right.open(right.rowOf(u"hc.dsk"_s)));
        QVERIFY(right.copyTo(right.rowOf(u"APP.COM"_s), &left));
        QVERIFY(left.rowOf(u"APP.COM"_s) > 0);
        QCOMPARE(failures.count(), 0);
        auto stored = openImage(root.filePath(u"card.img"_s));
        const auto p3d = stored.read("nextzxos/cpm-a.p3d"sv);
        auto copied = zxkit::MediaArchive::open(p3d, ".p3d"sv).read("APP.COM"sv);
        copied.resize(app.size());
        QCOMPARE(copied, app);
        QCOMPARE(QFileInfo(root.filePath(u"card.img"_s)).size(), cardSize);

        // CP/M text: CR LF lines up to the end-of-file mark; edits keep them
        right.up();
        QVERIFY(right.open(right.rowOf(u"hc.dsk"_s)));
        QVERIFY(left.makeFolder(u"x"_s) == false); // CP/M has no folders
        hc = openImage(root.filePath(u"hc.dsk"_s));
        hc.put("README.TXT"sv,
               std::span(reinterpret_cast<const uint8_t *>("Hello\r\nNext\r\n"), 13));
        hc.put("NOTES"sv, std::span(reinterpret_cast<const uint8_t *>("plain words\r\n"), 13));
        // WordStar marks word ends with bit 7
        hc.put("WS.DOC"sv,
               std::span(reinterpret_cast<const uint8_t *>("Hello wor\xEC"
                                                           "d\r\n"),
                         12));
        // A program with an early end-of-file byte is no text
        std::vector<uint8_t> program(300, 0x3E);
        program[5] = 0x1A;
        program[6] = 0;
        hc.put("EOF.COM"sv, program);
        save(u"hc.dsk"_s, hc.encode());
        right.refresh();
        const auto readme = right.rowOf(u"README.TXT"_s);
        QCOMPARE(right.data(right.index(readme), FileManagerPanel::Kind).toString(), u"text"_s);
        QCOMPARE(pageTitles(right.view(readme)), u"Text,Hex"_s);
        QCOMPARE(right.view(readme)[u"pages"_s].toList()[0].toMap()[u"lines"_s].toStringList(),
                 QStringList({ u"Hello"_s, u"Next"_s }));
        QCOMPARE(pageTitles(right.view(right.rowOf(u"NOTES"_s))), u"Text,Hex"_s);
        QCOMPARE(pageTitles(right.view(right.rowOf(u"APP.COM"_s))), u"Hex,Disassembly"_s);
        QCOMPARE(pageTitles(right.view(right.rowOf(u"EOF.COM"_s))), u"Hex,Disassembly"_s);
        const auto wordStar = right.rowOf(u"WS.DOC"_s);
        QCOMPARE(right.view(wordStar)[u"pages"_s].toList()[0].toMap()[u"lines"_s].toStringList(),
                 QStringList{ u"Hello world"_s });
        QVERIFY(!right.editable(wordStar)[u"note"_s].toString().isEmpty());
        QVERIFY(right.editable(right.rowOf(u"APP.COM"_s)).isEmpty());
        QCOMPARE(right.editable(readme)[u"text"_s].toString(), u"Hello\nNext\n"_s);
        QVERIFY(right.saveText(readme, u"Hello\nZX Next\n"_s));
        const auto saved = openImage(root.filePath(u"hc.dsk"_s)).read("README.TXT"sv);
        QVERIFY(QByteArray(reinterpret_cast<const char *>(saved.data()), qsizetype(saved.size()))
                        .startsWith("Hello\r\nZX Next\r\n\x1A"));

        // Back out to the card, the cursor on the image left
        const auto imageRow = left.up();
        QCOMPARE(imageRow, left.rowOf(u"cpm-a.p3d"_s));
        QCOMPARE(left.filesystem(), u"fat"_s);
        QVERIFY(written.count() > 0);
        QVERIFY(left.sameFile(written.back()[0].toString(), root.filePath(u"card.img"_s)));

        // The card's own files and folders
        QVERIFY(left.makeFolder(u"apps"_s));
        QVERIFY(left.open(left.rowOf(u"apps"_s)));
        right.up();
        QVERIFY(right.copyTo(right.rowOf(u"hc.dsk"_s), &left));
        QVERIFY(left.rename(left.rowOf(u"hc.dsk"_s), u"hc2000.dsk"_s));
        QVERIFY(left.setAttributes(left.rowOf(u"hc2000.dsk"_s), { { u"readonly"_s, 1 } }));
        QVERIFY(!left.remove(left.rowOf(u"hc2000.dsk"_s)));
        QVERIFY(left.setAttributes(left.rowOf(u"hc2000.dsk"_s), { { u"readonly"_s, 0 } }));
        // An image on the card opens from the card
        QVERIFY(left.open(left.rowOf(u"hc2000.dsk"_s)));
        QVERIFY(left.rowOf(u"APP.COM"_s) > 0);
        left.up();
        left.up();
        QVERIFY(left.remove(left.rowOf(u"apps"_s)));
        QCOMPARE(left.rowOf(u"apps"_s), -1);
        stored = openImage(root.filePath(u"card.img"_s));
        const auto entries = stored.list();
        QVERIFY(std::ranges::none_of(entries, [](const zxkit::MediaEntry &entry) {
            return entry.name.starts_with("nextzxos/apps"sv);
        }));
        left.up();
        QVERIFY(left.inMedia());
        left.up();
        QVERIFY(!left.inMedia());
    }

    // The viewer's copies, PNG exports and addresses
    void viewerTools()
    {
        QTemporaryDir root;
        FileManagerPanel panel;
        panel.setFolder(root.path());
        panel.copyText(u"10 PRINT 1\n"_s);
        QCOMPARE(QGuiApplication::clipboard()->text(), u"10 PRINT 1\n"_s);
        QByteArray screen(6912, 0);
        screen[6144] = char(0x38);
        writeFile(root.filePath(u"title.scr"_s), screen);
        panel.refresh();
        const auto image = panel.view(panel.rowOf(u"title.scr"_s))[u"pages"_s]
                                   .toList()[0]
                                   .toMap()[u"image"_s]
                                   .toString();
        panel.copyImage(image);
        QCOMPARE(QGuiApplication::clipboard()->image().size(), QSize(256, 192));
        const auto saved = panel.saveImage(image, u"title.scr"_s);
        QCOMPARE(QFileInfo(saved).fileName(), u"title.png"_s);
        QCOMPARE(QImage(saved).size(), QSize(256, 192));
        QCOMPARE(QFileInfo(panel.saveImage(image, u"title.scr"_s)).fileName(), u"title-2.png"_s);

        QCOMPARE(panel.parseAddress(u"23803"_s), 23803);
        QCOMPARE(panel.parseAddress(u"$5CFB"_s), 0x5CFB);
        QCOMPARE(panel.parseAddress(u"#5cfb"_s), 0x5CFB);
        QCOMPARE(panel.parseAddress(u"0x5CFB"_s), 0x5CFB);
        QCOMPARE(panel.parseAddress(u"5CFBh"_s), 0x5CFB);
        QCOMPARE(panel.parseAddress(u"62DD"_s), 0x62DD);
        QCOMPARE(panel.parseAddress(u"65536"_s), -1);
        QCOMPARE(panel.parseAddress(u"nope"_s), -1);

        // LD HL,$632F at 8000: going to 8001 restarts there
        const QByteArray code("\x21\x2F\x63\xC9", 4);
        const auto lines = panel.disassemble(code, 0x8000, 0x8001);
        QCOMPARE(lines.size(), 4);
        QVERIFY(lines[0].startsWith(u"8000  21 2F 63"_s));
        QVERIFY(lines[1].startsWith(u"8001  2F"_s));
        QVERIFY(lines[3].startsWith(u"8003  C9"_s));
        QCOMPARE(panel.disassemble(code, 0x8000, -1).size(), 2);
    }

    void unreadableImages()
    {
        QTemporaryDir root;
        FileManagerPanel panel;
        panel.setFolder(root.path());
        writeFile(root.filePath(u"blank.img"_s), QByteArray(737280, 0));
        writeFile(root.filePath(u"broken.tap"_s), QByteArray("\x05\x00\x00\x01", 4));
        // An HC-2000 BASIC disk in a DSK container: its geometry suggests +3, its catalogue doesn't
        auto hc = zxkit::MediaArchive::create(".dsk"sv, { .filesystem = "hc-basic-80"s });
        hc.put("PROG"sv, zxkit::BasicCodec::compile("10 PRINT 1\n"sv), { { "type"s, 0 } });
        const auto bytes = hc.encode();
        writeFile(
                root.filePath(u"hc.dsk"_s),
                QByteArray(reinterpret_cast<const char *>(bytes.data()), qsizetype(bytes.size())));
        panel.refresh();
        QSignalSpy needed(&panel, &FileManagerPanel::filesystemNeeded);
        QSignalSpy failures(&panel, &FileManagerPanel::failed);
        QVERIFY(panel.open(panel.rowOf(u"blank.img"_s)));
        QCOMPARE(needed.count() + failures.count(), 1);
        QVERIFY(!panel.inMedia());
        if (needed.count())
            QVERIFY(!needed.front()[1].toList().isEmpty());
        failures.clear();
        QVERIFY(panel.open(panel.rowOf(u"broken.tap"_s)));
        QCOMPARE(failures.count(), 1);
        QVERIFY(!panel.inMedia());
        QCOMPARE(pageTitles(panel.view(panel.rowOf(u"broken.tap"_s))), u"Hex"_s);
        QVERIFY(panel.open(panel.rowOf(u"hc.dsk"_s)));
        QVERIFY(panel.inMedia());
        QCOMPARE(panel.filesystem(), u"hc-basic-80"_s);
        QVERIFY(panel.rowOf(u"PROG"_s) > 0);
    }
};

QTEST_MAIN(FileManagerTest)
#include "test_file_manager.moc"
