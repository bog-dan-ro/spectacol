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

#include "filemanagerpanel.h"

#include <zxkit/media/FatImage.h>
#include <zxkit/media/MediaView.h>
#include <zxkit/media/BasicCodec.h>
#include <zxkit/media/MediaPreview.h>
#include <zxkit/media/TapeFormats.h>

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QFuture>
#include <QSet>
#include <QtConcurrent/QtConcurrentRun>
#include <QUrl>

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace Qt::StringLiterals;
using namespace std::literals;

namespace {

// Whole images are edited in memory: leave SD-card sized images to the SD card page
constexpr qint64 MaxMediaBytes = 128 * 1024 * 1024;
constexpr qint64 MaxTextBytes = 1024 * 1024;
constexpr int ScreenBytes = 6912;

bool tapeSuffix(const QString &suffix)
{
    return suffix == u"tap"_s || suffix == u"tzx"_s || suffix == u"pzx"_s || suffix == u"csw"_s;
}

bool mediaSuffix(const QString &suffix)
{
    static const QSet<QString> suffixes = { u"tap"_s,  u"tzx"_s, u"pzx"_s, u"csw"_s, u"dsk"_s,
                                            u"edsk"_s, u"trd"_s, u"scl"_s, u"cqm"_s, u"td0"_s,
                                            u"opd"_s,  u"mgt"_s, u"d40"_s, u"d80"_s, u"p3d"_s,
                                            u"img"_s,  u"raw"_s, u"mdr"_s, u"nex"_s };
    return suffixes.contains(suffix);
}

bool textSuffix(const QString &suffix)
{
    return zxkit::textFileName((u"x."_s + suffix).toStdString());
}

QString hostKind(bool dir, const QString &name)
{
    if (dir)
        return u"folder"_s;
    const auto suffix = QFileInfo(name).suffix().toLower();
    if (tapeSuffix(suffix))
        return u"tape"_s;
    if (suffix == u"mdr"_s || suffix == u"nex"_s)
        return u"program"_s;
    if (mediaSuffix(suffix))
        return u"disk"_s;
    if (suffix == u"sna"_s || suffix == u"z80"_s || suffix == u"szx"_s)
        return u"snapshot"_s;
    if (suffix == u"scr"_s)
        return u"screen"_s;
    if (suffix == u"rzx"_s)
        return u"recording"_s;
    if (textSuffix(suffix))
        return u"text"_s;
    return u"file"_s;
}

QString fromStd(std::string_view text)
{
    return QString::fromUtf8(text.data(), qsizetype(text.size()));
}

std::optional<uint64_t> attribute(const zxkit::MediaAttributes &attributes, std::string_view key)
{
    const auto i = attributes.find(key);
    if (i == attributes.end())
        return {};
    return i->second;
}

bool trdos(const QString &filesystem)
{
    return filesystem.startsWith(u"trdos"_s);
}

bool booleanAttribute(std::string_view key)
{
    static constexpr std::array keys = {
        "readonly"sv,  "hidden"sv,   "system"sv,     "archive"sv,
        "protected"sv, "readable"sv, "executable"sv, "deletable"sv
    };
    return std::ranges::find(keys, key) != keys.end();
}

bool readOnlyAttribute(std::string_view key)
{
    return key == "user"sv || key == "raw"sv || key == "native-type"sv || key.starts_with("tail"sv);
}

// The kind of data a media file holds: its tape/+3DOS type, or TR-DOS's letter
// An entry's tag: what zxkit::mediaContent() says it holds
QString contentKind(zxkit::MediaContent content)
{
    switch (content) {
    case zxkit::MediaContent::Basic:
        return u"basic"_s;
    case zxkit::MediaContent::NumberArray:
    case zxkit::MediaContent::CharacterArray:
        return u"array"_s;
    case zxkit::MediaContent::Code:
        return u"code"_s;
    case zxkit::MediaContent::Screen:
        return u"screen"_s;
    case zxkit::MediaContent::Text:
        return u"text"_s;
    case zxkit::MediaContent::TapeBlock:
        return u"block"_s;
    case zxkit::MediaContent::Snapshot:
        return u"snapshot"_s;
    case zxkit::MediaContent::Unknown:
        break;
    }
    return u"file"_s;
}

// Local files and device documents
std::vector<uint8_t> readHostFile(const QString &path, qint64 limit, QString &error)
{
    return StorageFiles::read(path, limit, error);
}

bool writeHostFile(const QString &path, std::span<const uint8_t> bytes, QString &error)
{
    return StorageFiles::write(path, bytes, error);
}

QStringList toQt(const std::vector<std::string> &lines)
{
    QStringList out;
    out.reserve(qsizetype(lines.size()));
    for (const auto &line : lines)
        out.append(QString::fromStdString(line));
    return out;
}

QStringList textLines(const QString &text)
{
    auto lines = text.split(u'\n');
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    return lines;
}

QString imageUrl(const zxkit::VideoFrame &frame)
{
    QImage image(int(frame.width()), int(frame.height()), QImage::Format_RGBA8888);
    for (uint32_t y = 0; y < frame.height(); ++y) {
        const auto row = frame.scanline(y);
        std::memcpy(image.scanLine(int(y)), row.data(), row.size() * sizeof(uint32_t));
    }
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return u"data:image/png;base64,"_s + QString::fromLatin1(png.toBase64());
}

// A page of text lines; the style says how the viewer colours them: "text", "hex",
// "disassembly" or "basic" (with the listing's parts)
QVariantMap linesPage(const QString &title, const QStringList &lines,
                      const QString &style = u"text"_s)
{
    return { { u"title"_s, title }, { u"lines"_s, lines }, { u"style"_s, style } };
}

QString partKind(zxkit::BasicCodec::ListingPart::Kind kind)
{
    using Kind = zxkit::BasicCodec::ListingPart::Kind;
    switch (kind) {
    case Kind::LineNumber:
        return u"line"_s;
    case Kind::Keyword:
        return u"keyword"_s;
    case Kind::Number:
        return u"number"_s;
    case Kind::String:
        return u"string"_s;
    case Kind::Comment:
        return u"comment"_s;
    case Kind::Control:
        return u"control"_s;
    case Kind::Code:
        return u"code"_s;
    case Kind::Text:
        break;
    }
    return u"text"_s;
}

// The program's lines without its variables (the tape header's second parameter)
uint64_t programLength(std::span<const uint8_t> program)
{
    std::size_t length = 0;
    while (program.size() - length >= 4 && program[length] < 40) {
        const std::size_t line = (program[length + 2] | (program[length + 3] << 8)) + 4;
        if (line > program.size() - length)
            break;
        length += line;
    }
    return length;
}

QString typeText(const QString &filesystem, const zxkit::MediaAttributes &attributes,
                 const QString &kind)
{
    const auto type = attribute(attributes, "type"sv);
    const auto param1 = attribute(attributes, "param1"sv);
    if (!type)
        return {};
    if (trdos(filesystem)) {
        switch (*type) {
        case 'B':
            return FileManagerPanel::tr("BASIC");
        case 'C':
            return FileManagerPanel::tr("Code %1").arg(param1.value_or(0));
        case 'D':
            return FileManagerPanel::tr("Data");
        case '#':
            return FileManagerPanel::tr("Stream");
        }
        return QChar(char16_t(*type));
    }
    switch (*type) {
    case 0:
        return param1 && *param1 < 32768 ? FileManagerPanel::tr("Program, LINE %1").arg(*param1)
                                         : FileManagerPanel::tr("Program");
    case 1:
        return FileManagerPanel::tr("Number array");
    case 2:
        return FileManagerPanel::tr("Character array");
    case 3:
        return kind == u"screen"_s ? FileManagerPanel::tr("Screen %1").arg(param1.value_or(0))
                                   : FileManagerPanel::tr("Bytes %1").arg(param1.value_or(0));
    }
    return FileManagerPanel::tr("Type %1").arg(*type);
}

QString flagsText(const zxkit::MediaAttributes &attributes)
{
    QStringList flags;
    if (attribute(attributes, "readonly"sv).value_or(0))
        flags.append(FileManagerPanel::tr("R/O"));
    if (attribute(attributes, "hidden"sv).value_or(0))
        flags.append(FileManagerPanel::tr("hidden"));
    if (attribute(attributes, "system"sv).value_or(0))
        flags.append(FileManagerPanel::tr("system"));
    return flags.join(u", "_s);
}

// A TAP tape's blocks are one-block TAP files, other tapes' one-block TZX files
QString blockName(std::size_t index, const QString &mediaPath)
{
    const auto suffix = QFileInfo(mediaPath).suffix().toLower() == u"tap"_s ? u"tap"_s : u"tzx"_s;
    return u"blocks/%1.%2"_s.arg(index, 6, 10, u'0').arg(suffix);
}

std::optional<std::size_t> blockIndex(const QString &key)
{
    if (!key.startsWith(u"blocks/"_s) || !(key.endsWith(u".tzx"_s) || key.endsWith(u".tap"_s)))
        return {};
    bool ok = false;
    const auto index = key.mid(7, key.size() - 11).toULongLong(&ok);
    return ok ? std::optional<std::size_t>(index) : std::nullopt;
}

std::vector<zxkit::TapeBlock> parseBlock(std::span<const uint8_t> bytes)
{
    const bool tzx = bytes.size() >= 7 && std::memcmp(bytes.data(), "ZXTape!", 7) == 0;
    return tzx ? zxkit::TapeFormats::parseTzx(bytes) : zxkit::TapeFormats::parseTap(bytes);
}

// Attributes keep their meaning between TR-DOS letters and tape/+3DOS types
zxkit::MediaAttributes translateAttributes(const QString &from, const QString &to,
                                           zxkit::MediaAttributes attributes)
{
    const auto type = attribute(attributes, "type"sv);
    if (!type || trdos(from) == trdos(to))
        return attributes;
    if (trdos(from)) {
        attributes.erase("length"s);
        switch (*type) {
        case 'B':
            attributes["type"s] = 0;
            break;
        case 'C':
            attributes["type"s] = 3;
            break;
        case 'D':
            attributes["type"s] = 1;
            break;
        default:
            attributes.erase("type"s);
        }
        return attributes;
    }
    static constexpr std::array letters = { 'B', 'D', 'D', 'C' };
    if (*type < letters.size())
        attributes["type"s] = uint64_t(letters[*type]);
    else
        attributes.erase("type"s);
    attributes.erase("param2"s);
    attributes.erase("pause"s);
    return attributes;
}

// Defaults for a host file going onto a tape or disk
zxkit::MediaAttributes hostAttributes(const QString &name, qint64 size)
{
    if (name.endsWith(u".scr"_s, Qt::CaseInsensitive) && size == ScreenBytes)
        return { { "type"s, 3 }, { "param1"s, 16384 } };
    return {};
}

QString trdosName(const QString &name, const zxkit::MediaAttributes &attributes)
{
    if (name.contains(u'.'))
        return name;
    const auto type = attribute(attributes, "type"sv).value_or('C');
    return name.left(8) + u'.' + QChar(char16_t(type >= 33 && type <= 126 ? type : 'C'));
}

// The filesystem profiles that read the image: [{profile, files}], those with files first
QVariantList readableProfiles(std::span<const uint8_t> bytes, const std::string &extension)
{
    QVariantList choices;
    for (const auto &match : zxkit::MediaArchive::probe(bytes, extension))
        choices.append(QVariantMap{ { u"profile"_s, fromStd(match.profile) },
                                    { u"files"_s, qsizetype(match.files) } });
    return choices;
}

// What a host file or an entry holds when it is an image to open, else empty
QString imageKind(const QString &name)
{
    const auto suffix = QFileInfo(name).suffix().toLower();
    if (!mediaSuffix(suffix))
        return {};
    if (tapeSuffix(suffix))
        return u"tape"_s;
    return suffix == u"mdr"_s || suffix == u"nex"_s ? u"program"_s : u"disk"_s;
}

} // namespace

// The image a panel is in: its entries, and how its changes are saved
class FileManagerMedia
{
public:
    virtual ~FileManagerMedia() = default;
    QString key; // the host file, then each entry down to this image
    QString path; // the host file
    bool writable = false;

    virtual std::string filesystem() const = 0;
    /// At least the entries directly in `folder` ("" is the root), with full names; with
    /// `recursive`, all of those below it. Empty for a missing folder.
    virtual std::vector<zxkit::MediaEntry> entries(std::string_view folder,
                                                   bool recursive) const = 0;
    virtual std::vector<uint8_t> read(std::string_view name) const = 0;
    virtual void put(std::string_view name, std::span<const uint8_t> bytes,
                     const zxkit::MediaAttributes &attributes) = 0;
    virtual void remove(std::string_view name) = 0;
    virtual void rename(std::string_view from, std::string_view to, bool replace) = 0;
    virtual void setAttributes(std::string_view name, const zxkit::MediaAttributes &changes) = 0;
    virtual void makeDirectory(std::string_view name) = 0;
    /// Saves the changes; throws
    virtual void commit() = 0;
    /// Drops the unsaved changes
    virtual void revert() = 0;
    /// Rereads the image when its host file changed: -1 when it is gone, 1 when it changed
    virtual int refresh() = 0;

    bool exists(std::string_view name) const
    {
        const auto slash = name.rfind('/');
        const auto list =
                entries(slash == name.npos ? std::string_view{} : name.substr(0, slash), false);
        return std::ranges::find(list, name, &zxkit::MediaEntry::name) != list.end();
    }
};

namespace {

// A tape or disk image edited in memory through MediaArchive, saved into its host file or into
// the image holding it
class ArchiveMedia final : public FileManagerMedia
{
public:
    ArchiveMedia(zxkit::MediaArchive archive, zxkit::MediaOptions options, std::string extension)
        : m_archive(std::move(archive)),
          m_options(std::move(options)),
          m_extension(std::move(extension))
    {
    }
    std::function<void(std::span<const uint8_t>)> store;
    std::function<std::vector<uint8_t>()> load;
    // The host file's state, for a top-level image
    QDateTime modified;
    qint64 size = -1;

    std::string filesystem() const override
    {
        return m_archive.filesystem();
    }
    std::vector<zxkit::MediaEntry> entries(std::string_view folder, bool) const override
    {
        auto list = m_archive.list();
        if (!folder.empty()) {
            const auto prefix = std::string(folder) + '/';
            std::erase_if(list, [&](const zxkit::MediaEntry &entry) {
                return !entry.name.starts_with(prefix);
            });
        }
        return list;
    }
    std::vector<uint8_t> read(std::string_view name) const override
    {
        return m_archive.read(name);
    }
    void put(std::string_view name, std::span<const uint8_t> bytes,
             const zxkit::MediaAttributes &attributes) override
    {
        m_archive.put(name, bytes, attributes);
    }
    void remove(std::string_view name) override
    {
        m_archive.remove(name);
    }
    void rename(std::string_view from, std::string_view to, bool replace) override
    {
        m_archive.rename(from, to, replace);
    }
    void setAttributes(std::string_view name, const zxkit::MediaAttributes &changes) override
    {
        m_archive.setAttributes(name, changes);
    }
    void makeDirectory(std::string_view name) override
    {
        m_archive.makeDirectory(name);
    }
    void commit() override
    {
        store(m_archive.encode());
        if (modified.isValid()) {
            const QFileInfo info(path);
            modified = info.lastModified();
            size = info.size();
        }
    }
    void revert() override
    {
        m_archive = zxkit::MediaArchive::open(load(), m_extension, m_options);
    }
    int refresh() override
    {
        if (!modified.isValid())
            return 0;
        const QFileInfo info(path);
        if (!info.exists())
            return -1;
        if (info.lastModified() == modified && info.size() == size)
            return 0;
        revert();
        modified = info.lastModified();
        size = info.size();
        writable = info.isWritable();
        return 1;
    }

private:
    zxkit::MediaArchive m_archive;
    zxkit::MediaOptions m_options;
    std::string m_extension;
};

// A FAT card image (the Next's SD card) edited in place, a block at a time: it may be large
class FatMedia final : public FileManagerMedia
{
public:
    explicit FatMedia(const QString &image) : m_file(image) { }

    // Opens the image; false when it holds no FAT file system
    bool mount()
    {
        writable = m_file.open(QIODevice::ReadWrite);
        if (!writable && !m_file.open(QIODevice::ReadOnly))
            return false;
        zxkit::FatVolume::BlockWriter writer;
        if (writable)
            writer = [this](uint64_t lba, std::span<const uint8_t, 512> in) {
                return m_file.seek(qint64(lba * 512))
                        && m_file.write(reinterpret_cast<const char *>(in.data()), 512) == 512;
            };
        try {
            m_fat.emplace(
                    [this](uint64_t lba, std::span<uint8_t, 512> out) {
                        return m_file.seek(qint64(lba * 512))
                                && m_file.read(reinterpret_cast<char *>(out.data()), 512) == 512;
                    },
                    std::move(writer));
            return true;
        } catch (const std::exception &) {
            m_file.close();
            return false;
        }
    }
    std::string filesystem() const override
    {
        return "fat"s;
    }
    std::vector<zxkit::MediaEntry> entries(std::string_view folder, bool recursive) const override
    {
        return m_fat->list(folder, recursive);
    }
    std::vector<uint8_t> read(std::string_view name) const override
    {
        return m_fat->read(name);
    }
    void put(std::string_view name, std::span<const uint8_t> bytes,
             const zxkit::MediaAttributes &attributes) override
    {
        m_fat->put(name, bytes, attributes);
    }
    void remove(std::string_view name) override
    {
        m_fat->remove(name);
    }
    void rename(std::string_view from, std::string_view to, bool replace) override
    {
        m_fat->rename(from, to, replace);
    }
    void setAttributes(std::string_view name, const zxkit::MediaAttributes &changes) override
    {
        m_fat->setAttributes(name, changes);
    }
    void makeDirectory(std::string_view name) override
    {
        m_fat->makeDirectory(name);
    }
    void commit() override
    {
        if (!m_file.flush())
            throw std::runtime_error(m_file.errorString().toStdString());
    }
    void revert() override { }
    int refresh() override
    {
        return QFileInfo::exists(path) ? 0 : -1;
    }

private:
    QFile m_file;
    std::optional<zxkit::FatImage> m_fat;
};

bool fatSignature(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const auto sector = file.read(512);
    return sector.size() == 512 && uint8_t(sector[510]) == 0x55 && uint8_t(sector[511]) == 0xAA
            && (uint8_t(sector[0]) == 0xEB || uint8_t(sector[0]) == 0xE9 || sector[446 + 4] != 0);
}

// A host file or folder (local, or a device's document) copied into a host folder. The errors
// are collected, not reported: a copy involving a device folder runs off the GUI thread.
bool copyHostEntry(const QString &path, bool dir, const QString &folder, const QString &name,
                   bool overwrite, QStringList &errors)
{
    QString error;
    const auto existing = StorageFiles::find(folder, name);
    if (dir) {
        if (existing && !existing->dir) {
            errors.append(FileManagerPanel::tr("'%1' already exists.").arg(name));
            return false;
        }
        const auto into = existing ? existing->path : StorageFiles::makeFolder(folder, name, error);
        if (into.isEmpty()) {
            errors.append(FileManagerPanel::tr("Can't create folder '%1'.").arg(name));
            return false;
        }
        bool ok = true;
        for (const auto &child : StorageFiles::list(path))
            ok = copyHostEntry(child.path, child.dir, into, child.name, overwrite, errors) && ok;
        return ok;
    }
    if (existing && !overwrite) {
        errors.append(FileManagerPanel::tr("'%1' already exists.").arg(name));
        return false;
    }
    if (existing && existing->dir) {
        errors.append(FileManagerPanel::tr("Can't replace folder '%1'.").arg(name));
        return false;
    }
    // In blocks: a card image is copied whole, without holding it in memory
    if (StorageFiles::copy(path, folder, name, existing ? existing->path : QString(), error)
                .isEmpty()) {
        errors.append(error);
        return false;
    }
    return true;
}

} // namespace

FileManagerPanel::FileManagerPanel(QObject *parent) : QAbstractListModel(parent)
{
    m_location.setFolder(QDir::homePath());
    reload();
}

FileManagerPanel::~FileManagerPanel() = default;

void FileManagerPanel::setFolder(const QString &folder)
{
    // A remembered folder may have gone, be outside the allowed one or be unreadable (on
    // Android, the folders above Spectacol's): it starts from the top. The row opened knows
    // a device folder's name.
    QString name;
    for (const auto &entry : m_entries) {
        if (entry.key == folder && !entry.parent) {
            name = entry.name;
            break;
        }
    }
    if (!m_location.setFolder(folder, name) && !m_media)
        return;
    m_media.reset();
    m_parents.clear();
    m_mediaFolder.clear();
    reload();
    emit locationChanged();
}

void FileManagerPanel::setHomeFolder(const QString &folder)
{
    const auto previous = m_location.home();
    m_location.setHome(folder);
    if (m_location.home() == previous)
        return;
    emit homeFolderChanged();
    if (!m_media && (this->folder() == previous || this->folder() == m_location.home()))
        reload();
}

void FileManagerPanel::setDeviceFolders(const QStringList &folders)
{
    if (m_location.deviceFolders() == folders)
        return;
    const auto previous = folder();
    m_location.setDeviceFolders(folders);
    emit deviceFoldersChanged();
    if (folder() != previous) {
        // Forgotten while we were in it
        m_media.reset();
        m_parents.clear();
        m_mediaFolder.clear();
        reload();
        emit locationChanged();
    } else if (!m_media && folder() == m_location.home()) {
        reload();
    }
}

void FileManagerPanel::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

QString FileManagerPanel::location() const
{
    if (!m_media)
        return m_location.inDevice() ? m_location.title() : QDir::toNativeSeparators(folder());
    auto text = QDir::toNativeSeparators(m_media->path);
    for (const auto &parent : m_parents)
        text += u" › "_s
                + (parent.folder.isEmpty() ? parent.name : parent.folder + u'/' + parent.name);
    if (!m_mediaFolder.isEmpty())
        text += u" › "_s + m_mediaFolder;
    return text;
}

QString FileManagerPanel::mediaPath() const
{
    return m_media ? m_media->path : QString();
}

QString FileManagerPanel::filesystem() const
{
    return m_media ? fromStd(m_media->filesystem()) : QString();
}

bool FileManagerPanel::writable() const
{
    return m_media ? m_media->writable : StorageFiles::writable(folder());
}

bool FileManagerPanel::canMakeFolder() const
{
    return writable() && (!m_media || filesystem() == u"fat"_s);
}

bool FileManagerPanel::tapeBlocks() const
{
    return m_media && m_mediaFolder == u"blocks"_s && filesystem() == u"tape"_s;
}

int FileManagerPanel::markedCount() const
{
    return int(std::ranges::count_if(m_entries, &Entry::marked));
}

void FileManagerPanel::setPeer(FileManagerPanel *peer)
{
    if (m_peer == peer)
        return;
    m_peer = peer;
    emit peerChanged();
}

bool FileManagerPanel::fail(const QString &message) const
{
    emit const_cast<FileManagerPanel *>(this)->failed(message);
    return false;
}

bool FileManagerPanel::open(int row, const QString &filesystem)
{
    if (row < 0 || row >= count())
        return false;
    const auto entry = m_entries[size_t(row)];
    if (entry.parent) {
        up();
        return true;
    }
    if (entry.dir) {
        if (!m_media) {
            setFolder(entry.key);
            return true;
        }
        m_mediaFolder = entry.key;
        reload();
        emit locationChanged();
        return true;
    }
    if (!entry.media)
        return false;
    return openImage(row, filesystem);
}

std::shared_ptr<FileManagerMedia> FileManagerPanel::peerMedia(const QString &key) const
{
    if (!m_peer || !m_peer->m_media)
        return {};
    if (m_peer->m_media->key == key)
        return m_peer->m_media;
    for (const auto &parent : m_peer->m_parents)
        if (parent.media->key == key)
            return parent.media;
    return {};
}

bool FileManagerPanel::openImage(int row, const QString &filesystem)
{
    const auto entry = m_entries[size_t(row)];
    const bool nested = bool(m_media);
    // Panels on the same image share it, so they see each other's changes
    const bool document = StorageFiles::isDocument(entry.key);
    const auto key = nested ? m_media->key + u'\x1f' + entry.key
            : document      ? entry.key
                            : QFileInfo(entry.key).absoluteFilePath();
    auto media = filesystem.isEmpty() ? peerMedia(key) : nullptr;
    if (!media) {
        const auto suffix = QFileInfo(entry.name).suffix().toLower();
        // A card image is edited in place, whatever its size
        if (!nested && !document && (filesystem == u"fat"_s || filesystem.isEmpty())
            && fatSignature(entry.key)) {
            auto card = std::make_shared<FatMedia>(entry.key);
            if (card->mount())
                media = card;
        }
        if (!media) {
            if (entry.size > MaxMediaBytes) {
                fail(tr("'%1' is too large to edit here.").arg(entry.name));
                return true;
            }
            std::vector<uint8_t> bytes;
            try {
                if (nested) {
                    bytes = m_media->read(entry.key.toStdString());
                } else {
                    QString error;
                    bytes = readHostFile(entry.key, MaxMediaBytes, error);
                    if (!error.isEmpty())
                        throw std::runtime_error(error.toStdString());
                }
            } catch (const std::exception &e) {
                fail(QString::fromUtf8(e.what()));
                return true;
            }
            const auto extension = (u'.' + suffix).toStdString();
            zxkit::MediaOptions options;
            options.filesystem = filesystem.toStdString();
            std::optional<zxkit::MediaArchive> archive;
            try {
                archive.emplace(zxkit::MediaArchive::open(bytes, extension, options));
            } catch (const std::exception &e) {
                const auto message = QString::fromUtf8(e.what());
                if (filesystem.isEmpty() && !tapeSuffix(suffix) && suffix != u"nex"_s
                    && suffix != u"mdr"_s) {
                    // Detection is conservative: try every layout, and ask only when several fit
                    const auto choices = readableProfiles(bytes, extension);
                    if (choices.size() == 1)
                        return openImage(row, choices.front().toMap()[u"profile"_s].toString());
                    if (!choices.isEmpty()) {
                        emit filesystemNeeded(row, choices, message);
                        return true;
                    }
                }
                fail(tr("Can't open '%1': %2").arg(entry.name, message));
                return true;
            }
            auto image = std::make_shared<ArchiveMedia>(std::move(*archive), options, extension);
            if (nested) {
                // Saved into the image holding it
                const auto parent = m_media;
                const auto name = entry.key.toStdString();
                image->store = [parent, name](std::span<const uint8_t> bytes) {
                    try {
                        parent->put(name, bytes, {});
                        parent->commit();
                    } catch (...) {
                        parent->revert();
                        throw;
                    }
                };
                image->load = [parent, name] { return parent->read(name); };
                image->writable = parent->writable;
            } else {
                const auto path = entry.key;
                image->store = [path](std::span<const uint8_t> bytes) {
                    QString error;
                    if (!writeHostFile(path, bytes, error))
                        throw std::runtime_error(error.toStdString());
                };
                image->load = [path] {
                    QString error;
                    auto bytes = readHostFile(path, MaxMediaBytes, error);
                    if (!error.isEmpty())
                        throw std::runtime_error(error.toStdString());
                    return bytes;
                };
                if (document) {
                    // Without a time, refresh() doesn't ask the provider every time
                    image->writable = true;
                } else {
                    const QFileInfo info(path);
                    image->modified = info.lastModified();
                    image->size = info.size();
                    image->writable = info.isWritable();
                }
            }
            media = image;
        }
        media->key = key;
        media->path = nested ? m_media->path : entry.key;
    }
    if (nested)
        m_parents.push_back({ m_media, m_mediaFolder, entry.name });
    m_media = std::move(media);
    m_mediaFolder.clear();
    reload();
    emit locationChanged();
    return true;
}

int FileManagerPanel::up()
{
    QString left;
    if (m_media) {
        if (!m_mediaFolder.isEmpty()) {
            const auto slash = m_mediaFolder.lastIndexOf(u'/');
            left = m_mediaFolder.mid(slash + 1);
            m_mediaFolder = slash < 0 ? QString() : m_mediaFolder.left(slash);
        } else if (!m_parents.empty()) {
            // Out of a nested image, back to the folder holding it
            auto parent = std::move(m_parents.back());
            m_parents.pop_back();
            left = parent.name;
            m_media = std::move(parent.media);
            m_mediaFolder = parent.folder;
        } else {
            left = QFileInfo(m_media->path).fileName();
            m_media.reset();
        }
    } else {
        if (!m_location.inDevice() && QDir(folder()).isRoot())
            return -1;
        // Not above the allowed folder, nor into one we can't list and come back from
        left = m_location.name();
        if (!m_location.goUp())
            return 0;
        if (m_location.inDevice())
            m_select = left; // the listing comes later
    }
    reload();
    emit locationChanged();
    return std::max(0, rowOf(left));
}

void FileManagerPanel::refresh()
{
    if (m_media) {
        // The host file changed: reread it, and the images inside it from it
        try {
            const auto &outer = m_parents.empty() ? m_media : m_parents.front().media;
            const auto state = outer->refresh();
            if (state < 0)
                throw std::runtime_error(tr("'%1' is gone.").arg(m_media->path).toStdString());
            if (state > 0) {
                for (std::size_t i = 1; i < m_parents.size(); ++i)
                    m_parents[i].media->revert();
                if (!m_parents.empty())
                    m_media->revert();
            }
        } catch (const std::exception &e) {
            fail(QString::fromUtf8(e.what()));
            m_media.reset();
            m_parents.clear();
            m_mediaFolder.clear();
            emit locationChanged();
        }
    }
    reload();
}

bool FileManagerPanel::commit()
{
    try {
        m_media->commit();
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        try {
            m_media->revert();
        } catch (const std::exception &) {
            m_media.reset();
            m_parents.clear();
            m_mediaFolder.clear();
            emit locationChanged();
        }
        return false;
    }
    emit mediaWritten(m_media->path);
    return true;
}

void FileManagerPanel::reload()
{
    QSet<QString> marked;
    for (const auto &entry : m_entries)
        if (entry.marked)
            marked.insert(entry.key);
    if (!m_media && m_location.inDevice()) {
        listDocuments(marked);
        return;
    }
    m_listedFolder.clear();
    ++m_listing; // a device folder's listing still on its way is no longer wanted
    setBusy(false);
    beginResetModel();
    m_entries.clear();
    if (m_media)
        listMedia();
    else
        listHost();
    for (auto &entry : m_entries)
        entry.marked = !entry.parent && marked.contains(entry.key);
    endResetModel();
    emit countChanged();
    emit markedChanged();
}

FileManagerPanel::Entry FileManagerPanel::parentEntry() const
{
    Entry entry{ u".."_s, {}, -1, true, true };
    if (!m_location.inDevice())
        entry.key = QFileInfo(folder()).absolutePath();
    return entry;
}

void FileManagerPanel::appendHost(const std::vector<StorageFiles::Entry> &files)
{
    for (const auto &file : files) {
        Entry entry;
        entry.name = file.name;
        entry.key = file.path;
        entry.dir = file.dir;
        entry.size = file.dir ? -1 : file.size;
        entry.media = !file.dir && mediaSuffix(QFileInfo(file.name).suffix().toLower());
        entry.kind = hostKind(file.dir, file.name);
        entry.modified = file.modified;
        entry.info = file.modified.toString(u"yyyy-MM-dd hh:mm"_s);
        m_entries.push_back(std::move(entry));
    }
}

void FileManagerPanel::listHost()
{
    if (!m_location.atTop())
        m_entries.push_back(parentEntry());
    // The Spectacol folder lists the device folders, first
    if (!m_location.home().isEmpty() && folder() == m_location.home()) {
        for (const auto &device : m_location.deviceEntries()) {
            Entry entry{ device.name, device.path, -1, true };
            entry.device = true;
            entry.kind = u"folder"_s;
            m_entries.push_back(std::move(entry));
        }
    }
    appendHost(StorageFiles::list(folder()));
}

void FileManagerPanel::listDocuments(const QSet<QString> &marked)
{
    const auto shown = folder();
    if (shown != m_listedFolder) {
        // A new folder: only the way up until its listing comes
        beginResetModel();
        m_entries.clear();
        m_entries.push_back(parentEntry());
        endResetModel();
        emit countChanged();
        emit markedChanged();
        m_listedFolder = shown;
    }
    const auto generation = ++m_listing;
    setBusy(true);
    QtConcurrent::run([shown] {
        return StorageFiles::list(shown);
    }).then(this, [this, generation, shown, marked](const std::vector<StorageFiles::Entry> &files) {
        if (generation != m_listing || shown != folder() || m_media)
            return;
        beginResetModel();
        m_entries.clear();
        m_entries.push_back(parentEntry());
        appendHost(files);
        for (auto &entry : m_entries)
            entry.marked = !entry.parent && marked.contains(entry.key);
        endResetModel();
        emit countChanged();
        emit markedChanged();
        setBusy(false);
        if (!m_select.isEmpty())
            emit selectRequested(std::max(0, rowOf(std::exchange(m_select, {}))));
    });
}

void FileManagerPanel::listMedia()
{
    m_entries.push_back({ u".."_s, u".."_s, -1, true, true });
    std::vector<zxkit::MediaEntry> list;
    try {
        list = m_media->entries(m_mediaFolder.toStdString(), false);
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return;
    }
    const auto fs = filesystem();
    const bool tape = fs == u"tape"_s;
    const QString prefix = m_mediaFolder.isEmpty() ? QString() : m_mediaFolder + u'/';
    std::vector<Entry> folders;
    std::vector<Entry> files;
    QSet<QString> folderNames;
    QSet<QString> usedFolders;
    for (const auto &item : list) {
        const auto name = fromStd(item.name);
        if (!name.startsWith(prefix) || name.size() == prefix.size())
            continue;
        const auto rest = name.mid(prefix.size());
        const auto slash = rest.indexOf(u'/');
        if (slash >= 0) {
            const auto folder = rest.left(slash);
            usedFolders.insert(folder);
            if (!folderNames.contains(folder)) {
                folderNames.insert(folder);
                folders.push_back({ folder, prefix + folder, -1, true });
            }
            continue;
        }
        Entry entry;
        entry.name = rest;
        entry.key = name;
        entry.dir = item.directory;
        entry.size = item.directory ? -1 : qint64(item.size);
        entry.attributes = item.attributes;
        if (entry.dir) {
            if (folderNames.contains(rest))
                continue;
            folderNames.insert(rest);
            entry.kind = u"folder"_s;
            folders.push_back(std::move(entry));
            continue;
        }
        if (tape && name.startsWith(u"blocks/"_s)) {
            // The block's number and what it holds; its name is for copies
            entry.kind = u"block"_s;
            QString description;
            try {
                const auto blocks = parseBlock(m_media->read(item.name));
                if (!blocks.empty())
                    description = fromStd(blocks.front().description);
            } catch (const std::exception &) {
                description = tr("Unreadable block");
            }
            entry.label = u"#%1 %2"_s.arg(blockIndex(name).value_or(0)).arg(description);
        } else if (tape && name.startsWith(u"basic/"_s)) {
            entry.kind = u"basic"_s;
            entry.info = tr("Listing");
        } else {
            entry.kind = contentKind(
                    zxkit::mediaContent(fs.toStdString(), item.name, entry.attributes, item.size));
            QStringList info{ typeText(fs, entry.attributes, entry.kind),
                              flagsText(entry.attributes) };
            info.removeAll(QString());
            entry.info = info.join(u", "_s);
            // An image on a card or a disk opens like one on the host
            if (const auto image = imageKind(rest); !image.isEmpty() && !tape) {
                entry.media = true;
                if (entry.kind == u"file"_s)
                    entry.kind = image;
            }
        }
        files.push_back(std::move(entry));
    }
    for (auto &folder : folders) {
        folder.kind = u"folder"_s;
        // CP/M disks list all fifteen user areas: show the ones in use
        const bool userArea = folder.name.size() == 6 && folder.name.startsWith(u"user"_s);
        if (userArea && fs != u"fat"_s && !usedFolders.contains(folder.name))
            continue;
        m_entries.push_back(std::move(folder));
    }
    for (auto &file : files)
        m_entries.push_back(std::move(file));
}

std::vector<int> FileManagerPanel::targets(int row) const
{
    std::vector<int> rows;
    for (int i = 0; i < count(); ++i)
        if (m_entries[size_t(i)].marked)
            rows.push_back(i);
    if (rows.empty() && row >= 0 && row < count() && !m_entries[size_t(row)].parent)
        rows.push_back(row);
    return rows;
}

void FileManagerPanel::toggleMark(int row)
{
    if (row < 0 || row >= count() || m_entries[size_t(row)].parent || m_entries[size_t(row)].device)
        return;
    m_entries[size_t(row)].marked = !m_entries[size_t(row)].marked;
    emit dataChanged(index(row), index(row), { Marked });
    emit markedChanged();
}

void FileManagerPanel::markAll(bool marked)
{
    for (auto &entry : m_entries)
        entry.marked = marked && !entry.parent && !entry.device;
    if (count())
        emit dataChanged(index(0), index(count() - 1), { Marked });
    emit markedChanged();
}

QStringList FileManagerPanel::targetNames(int row) const
{
    QStringList names;
    for (auto i : targets(row))
        names.append(m_entries[size_t(i)].name);
    return names;
}

QString FileManagerPanel::hostPath(int row) const
{
    if (m_media)
        return m_parents.empty() && filesystem() != u"fat"_s ? m_media->path : QString();
    if (row < 0 || row >= count())
        return {};
    return m_entries[size_t(row)].key;
}

void FileManagerPanel::copyText(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}

void FileManagerPanel::copyImage(const QString &dataUrl) const
{
    QGuiApplication::clipboard()->setImage(
            QImage::fromData(QByteArray::fromBase64(dataUrl.section(u',', 1).toLatin1()), "PNG"));
}

QString FileManagerPanel::saveImage(const QString &dataUrl, const QString &name)
{
    const auto png = QByteArray::fromBase64(dataUrl.section(u',', 1).toLatin1());
    if (png.isEmpty()) {
        fail(tr("There is no image to save."));
        return {};
    }
    auto base = QFileInfo(name).completeBaseName();
    for (auto &c : base)
        if (u"/\\:*?\"<>|"_s.contains(c))
            c = u'_';
    if (base.isEmpty())
        base = u"screen"_s;
    QSet<QString> names;
    for (const auto &file : StorageFiles::list(folder()))
        names.insert(file.name);
    auto fileName = base + u".png"_s;
    for (int n = 2; names.contains(fileName); ++n)
        fileName = u"%1-%2.png"_s.arg(base).arg(n);
    QString error;
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t *>(png.constData()),
                                         std::size_t(png.size()));
    const auto path = StorageFiles::create(folder(), fileName, bytes, error);
    if (path.isEmpty()) {
        fail(error);
        return {};
    }
    peerRefresh();
    return path;
}

QStringList FileManagerPanel::disassemble(const QByteArray &bytes, int origin, int address) const
{
    return toQt(
            zxkit::disassemblyLines(std::span(reinterpret_cast<const uint8_t *>(bytes.constData()),
                                              std::size_t(bytes.size())),
                                    uint16_t(origin), address));
}

int FileManagerPanel::parseAddress(const QString &text) const
{
    auto value = text.trimmed().toUpper();
    int base = 10;
    if (value.startsWith(u'$') || value.startsWith(u'#')) {
        value.remove(0, 1);
        base = 16;
    } else if (value.startsWith(u"0X"_s)) {
        value.remove(0, 2);
        base = 16;
    } else if (value.endsWith(u'H')) {
        value.chop(1);
        base = 16;
    } else if (value.contains(QRegularExpression(u"[A-F]"_s))) {
        base = 16;
    }
    bool ok = false;
    const auto address = value.toInt(&ok, base);
    return ok && address >= 0 && address <= 65535 ? address : -1;
}

bool FileManagerPanel::sameFile(const QString &a, const QString &b) const
{
    if (a.isEmpty() || b.isEmpty())
        return false;
    if (StorageFiles::isDocument(a) || StorageFiles::isDocument(b))
        return StorageLocation::canonical(a) == StorageLocation::canonical(b);
    return QFileInfo(a) == QFileInfo(b);
}

int FileManagerPanel::rowOf(const QString &name) const
{
    for (int i = 0; i < count(); ++i)
        if (m_entries[size_t(i)].name == name)
            return i;
    return -1;
}

QString FileManagerPanel::mediaRoot() const
{
    // A tape's files go into files/; its root only holds the three views
    if (m_mediaFolder.isEmpty() && filesystem() == u"tape"_s)
        return u"files"_s;
    return m_mediaFolder;
}

QString FileManagerPanel::mediaChild(const QString &name, bool folder) const
{
    const auto root = folder ? m_mediaFolder : mediaRoot();
    return root.isEmpty() ? name : root + u'/' + name;
}

bool FileManagerPanel::sameMedia(const FileManagerPanel &other) const
{
    return m_media && other.m_media && m_media == other.m_media;
}

void FileManagerPanel::peerRefresh()
{
    reload();
    if (m_peer && m_peer != this)
        m_peer->refresh();
}

bool FileManagerPanel::makeFolder(const QString &name)
{
    if (m_working)
        return fail(tr("Wait for the copy to finish."));
    const auto trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(u'/'))
        return fail(tr("Invalid folder name."));
    if (m_media) {
        try {
            m_media->makeDirectory(mediaChild(trimmed).toStdString());
        } catch (const std::exception &e) {
            return fail(QString::fromUtf8(e.what()));
        }
        if (!commit())
            return false;
    } else {
        QString error;
        if (StorageFiles::makeFolder(folder(), trimmed, error).isEmpty())
            return fail(tr("Can't create folder '%1'.").arg(trimmed));
    }
    peerRefresh();
    return true;
}

QString FileManagerPanel::basicPath(const Entry &entry) const
{
    // A tape's BASIC listing edits, renames and changes its logical file
    if (filesystem() == u"tape"_s && entry.key.startsWith(u"basic/"_s)
        && entry.key.endsWith(u".bas"_s))
        return u"files/"_s + entry.key.mid(6, entry.key.size() - 10);
    return entry.key;
}

bool FileManagerPanel::rename(int row, const QString &newName)
{
    if (m_working)
        return fail(tr("Wait for the copy to finish."));
    if (row < 0 || row >= count() || m_entries[size_t(row)].parent)
        return false;
    const auto &entry = m_entries[size_t(row)];
    if (entry.device)
        return fail(tr("A device folder keeps its name."));
    const auto name = newName.trimmed();
    if (name.isEmpty() || (!m_media && name.contains(u'/')))
        return fail(tr("Invalid name."));
    if (name == entry.name)
        return true;
    if (m_media) {
        auto from = basicPath(entry);
        auto to = m_mediaFolder.isEmpty() ? name : m_mediaFolder + u'/' + name;
        if (from != entry.key)
            to = u"files/"_s + (name.endsWith(u".bas"_s) ? name.chopped(4) : name);
        try {
            m_media->rename(from.toStdString(), to.toStdString(), false);
        } catch (const std::exception &e) {
            return fail(QString::fromUtf8(e.what()));
        }
        if (!commit())
            return false;
    } else {
        QString error;
        if (StorageFiles::rename(entry.key, name, error).isEmpty())
            return fail(tr("Can't rename '%1' to '%2'.").arg(entry.name, name));
    }
    peerRefresh();
    return true;
}

bool FileManagerPanel::removeSource(const Source &source)
{
    if (!m_media) {
        QString error;
        return StorageFiles::remove(source.key, error)
                || fail(tr("Can't remove '%1'.").arg(source.name));
    }
    // Files first (tape blocks from the last, so the numbers stay valid), then folders
    std::vector<std::string> blocks;
    std::vector<std::string> files;
    std::vector<std::string> folders;
    const auto key = source.key.toStdString();
    const auto prefix = key + '/';
    std::vector<zxkit::MediaEntry> items{ { key, 0, source.dir, {} } };
    if (source.dir) {
        try {
            const auto below = m_media->entries(key, true);
            items.insert(items.end(), below.begin(), below.end());
        } catch (const std::exception &e) {
            return fail(QString::fromUtf8(e.what()));
        }
    }
    for (const auto &item : items) {
        if (item.name != key && !(source.dir && item.name.starts_with(prefix)))
            continue;
        if (item.directory)
            folders.push_back(item.name);
        else if (item.name.starts_with("blocks/"sv))
            blocks.push_back(item.name);
        else
            files.push_back(item.name);
    }
    std::ranges::sort(blocks, std::greater{});
    std::ranges::sort(folders, std::greater{});
    try {
        for (const auto &name : blocks)
            m_media->remove(name);
        // A tape file's BASIC listing goes with it
        for (const auto &name : files)
            if (m_media->exists(name))
                m_media->remove(name);
        if (filesystem() == u"fat"_s)
            for (const auto &name : folders)
                m_media->remove(name);
    } catch (const std::exception &e) {
        return fail(QString::fromUtf8(e.what()));
    }
    return true;
}

bool FileManagerPanel::remove(int row)
{
    if (m_working)
        return fail(tr("Wait for the copy to finish."));
    const auto rows = targets(row);
    if (rows.empty())
        return false;
    // Removing one would remove the user's folder: it is forgotten instead
    if (std::ranges::any_of(rows, [&](int i) { return m_entries[size_t(i)].device; }))
        return fail(tr("A device folder isn't removed: forget it to stop showing it."));
    std::vector<Source> sources;
    for (auto i : rows) {
        const auto &entry = m_entries[size_t(i)];
        sources.push_back({ entry.name, entry.key, entry.dir, entry.attributes });
    }
    // Remove the later tape blocks first: removing one renumbers the ones after it
    std::ranges::sort(sources, std::greater{}, &Source::key);
    bool ok = true;
    for (const auto &source : sources)
        ok = removeSource(source) && ok;
    if (m_media && !commit())
        ok = false;
    for (auto &entry : m_entries)
        entry.marked = false;
    peerRefresh();
    return ok;
}

std::vector<uint8_t> FileManagerPanel::readEntry(const Entry &entry) const
{
    if (!m_media) {
        QString error;
        auto bytes = readHostFile(entry.key, MaxMediaBytes, error);
        if (!error.isEmpty())
            throw std::runtime_error(error.toStdString());
        return bytes;
    }
    return m_media->read(entry.key.toStdString());
}

std::vector<uint8_t> FileManagerPanel::readSource(const FileManagerPanel &from,
                                                  const Source &source) const
{
    if (!from.m_media) {
        QString error;
        auto bytes = readHostFile(source.key, MaxMediaBytes, error);
        if (!error.isEmpty())
            throw std::runtime_error(error.toStdString());
        return bytes;
    }
    return from.m_media->read(source.key.toStdString());
}

bool FileManagerPanel::putMedia(const QString &name, std::span<const uint8_t> bytes,
                                const zxkit::MediaAttributes &attributes)
{
    // Attributes the target doesn't know are refused, never dropped silently: retry with the
    // common header fields, then with the target's defaults
    std::vector<zxkit::MediaAttributes> attempts{ attributes };
    zxkit::MediaAttributes header;
    for (auto key : { "type"sv, "param1"sv, "param2"sv })
        if (const auto value = attribute(attributes, key))
            header[std::string(key)] = *value;
    if (header != attributes)
        attempts.push_back(header);
    if (!header.empty())
        attempts.emplace_back();
    QString error;
    for (const auto &attempt : attempts) {
        try {
            m_media->put(name.toStdString(), bytes, attempt);
            return true;
        } catch (const std::exception &e) {
            error = QString::fromUtf8(e.what());
        }
    }
    return fail(tr("Can't copy to '%1': %2").arg(name, error));
}

std::vector<FileManagerPanel::Source> FileManagerPanel::children(const FileManagerPanel &from,
                                                                 const Source &source) const
{
    std::vector<Source> result;
    if (!from.m_media) {
        for (const auto &file : StorageFiles::list(source.key))
            result.push_back({ file.name, file.path, file.dir });
        return result;
    }
    const auto prefix = source.key + u'/';
    QSet<QString> seen;
    for (const auto &item : from.m_media->entries(source.key.toStdString(), false)) {
        const auto name = fromStd(item.name);
        if (!name.startsWith(prefix))
            continue;
        auto rest = name.mid(prefix.size());
        const bool nested = rest.contains(u'/');
        rest = rest.section(u'/', 0, 0);
        if (seen.contains(rest))
            continue;
        seen.insert(rest);
        result.push_back({ rest, prefix + rest, nested || item.directory,
                           nested ? zxkit::MediaAttributes{} : item.attributes });
    }
    return result;
}

bool FileManagerPanel::copyIntoHost(const FileManagerPanel &from, const Source &source,
                                    const QString &folder, const QString &name, bool overwrite)
{
    if (!from.m_media) {
        QStringList errors;
        const bool copied = copyHostEntry(source.key, source.dir, folder, name, overwrite, errors);
        for (const auto &error : errors)
            fail(error);
        return copied;
    }
    QString error;
    const auto existing = StorageFiles::find(folder, name);
    if (source.dir) {
        if (existing && !existing->dir)
            return fail(tr("'%1' already exists.").arg(name));
        auto into = existing ? existing->path : StorageFiles::makeFolder(folder, name, error);
        if (into.isEmpty())
            return fail(tr("Can't create folder '%1'.").arg(name));
        bool ok = true;
        for (const auto &child : children(from, source))
            ok = copyIntoHost(from, child, into, child.name, overwrite) && ok;
        return ok;
    }
    std::vector<uint8_t> bytes;
    try {
        bytes = readSource(from, source);
    } catch (const std::exception &e) {
        return fail(QString::fromUtf8(e.what()));
    }
    if (existing) {
        if (!overwrite)
            return fail(tr("'%1' already exists.").arg(name));
        if (existing->dir)
            return fail(tr("Can't replace folder '%1'.").arg(name));
        return StorageFiles::write(existing->path, bytes, error) || fail(error);
    }
    return !StorageFiles::create(folder, name, bytes, error).isEmpty() || fail(error);
}

bool FileManagerPanel::copyInto(const FileManagerPanel &from, const Source &source,
                                const QString &destination, bool overwrite)
{
    if (source.dir) {
        const auto name = destination.toStdString();
        if (!m_media->exists(name) && m_media->entries(name, false).empty()) {
            try {
                m_media->makeDirectory(name);
            } catch (const std::exception &e) {
                return fail(QString::fromUtf8(e.what()));
            }
        }
        bool ok = true;
        for (const auto &child : children(from, source))
            ok = copyInto(from, child, destination + u'/' + child.name, overwrite) && ok;
        return ok;
    }
    std::vector<uint8_t> bytes;
    try {
        bytes = readSource(from, source);
    } catch (const std::exception &e) {
        return fail(QString::fromUtf8(e.what()));
    }
    auto name = destination;
    const auto fs = filesystem();
    zxkit::MediaAttributes attributes = from.m_media
            ? translateAttributes(from.filesystem(), fs, source.attributes)
            : hostAttributes(source.name, qint64(bytes.size()));
    if (fs == u"tape"_s) {
        // Blocks are appended, BASIC listings compiled
        if (name.startsWith(u"blocks/"_s)) {
            std::size_t blocks = 0;
            for (const auto &item : m_media->entries("blocks", false))
                blocks += item.name.starts_with("blocks/"sv);
            name = blockName(blocks, m_media->path);
            attributes.clear();
        } else if (name.startsWith(u"files/"_s) && name.endsWith(u".bas"_s, Qt::CaseInsensitive)
                   && !from.m_media) {
            name = u"basic/"_s + name.mid(6, name.size() - 10) + u".bas"_s;
            attributes.clear();
        } else if (name.startsWith(u"basic/"_s)) {
            attributes.clear();
        }
    } else if (trdos(fs)) {
        const auto slash = name.lastIndexOf(u'/');
        name = name.left(slash + 1) + trdosName(name.mid(slash + 1), attributes);
    }
    if (!name.startsWith(u"blocks/"_s)) {
        if (m_media->exists(name.toStdString()) && !overwrite)
            return fail(tr("'%1' already exists.").arg(name));
    }
    return putMedia(name, bytes, attributes);
}

QStringList FileManagerPanel::conflicts(int row, FileManagerPanel *target,
                                        const QString &newName) const
{
    QStringList result;
    if (!target)
        return result;
    const auto rows = targets(row);
    for (auto i : rows) {
        const auto &entry = m_entries[size_t(i)];
        const auto name = rows.size() == 1 && !newName.isEmpty() ? newName : entry.name;
        if (!target->m_media) {
            // What the target lists: a device folder isn't asked again for each name
            if (std::ranges::any_of(target->m_entries, [&](const Entry &item) {
                    return !item.parent && !item.device && item.name == name;
                }))
                result.append(name);
            continue;
        }
        auto key = target->mediaChild(name, entry.dir);
        if (key.startsWith(u"blocks/"_s))
            continue;
        if (trdos(target->filesystem()))
            key = target->mediaChild(trdosName(name, entry.attributes));
        for (const auto &item : target->m_entries)
            if (item.key == key && !item.parent)
                result.append(name);
    }
    return result;
}

bool FileManagerPanel::copyTo(int row, FileManagerPanel *target, bool move, const QString &newName,
                              bool overwrite)
{
    if (!target)
        return false;
    const auto rows = targets(row);
    if (rows.empty())
        return false;
    if (m_working || target->m_working)
        return fail(tr("Wait for the copy to finish."));
    if (!target->writable())
        return fail(tr("The target is read-only."));
    if (move && !writable())
        return fail(tr("The source is read-only."));
    if (move && std::ranges::any_of(rows, [&](int i) { return m_entries[size_t(i)].device; }))
        return fail(tr("A device folder can be copied, not moved."));
    const bool sameHostFolder = !m_media && !target->m_media && folder() == target->folder();
    const bool sameMediaFolder =
            sameMedia(*target) && target->mediaRoot() == mediaRoot() && !tapeBlocks();
    const bool rename = rows.size() == 1 && !newName.trimmed().isEmpty();
    if ((sameHostFolder || sameMediaFolder) && !rename)
        return fail(tr("The source and the target are the same folder."));

    std::vector<Source> sources;
    for (auto i : rows) {
        const auto &entry = m_entries[size_t(i)];
        sources.push_back({ entry.name, entry.key, entry.dir, entry.attributes });
    }
    if (!m_media && !target->m_media && (m_location.inDevice() || target->m_location.inDevice()))
        return startHostCopy(std::move(sources), target, move,
                             rename ? newName.trimmed() : QString(), overwrite);
    bool ok = true;
    for (const auto &source : sources) {
        const auto name = rename ? newName.trimmed() : source.name;
        if (!target->m_media && !m_media) {
            if (source.dir && target->m_location.inside(source.key)) {
                ok = fail(tr("Can't copy '%1' into itself.").arg(source.name));
                continue;
            }
            const bool local = !StorageFiles::isDocument(source.key)
                    && !StorageFiles::isDocument(target->folder());
            if (local) {
                const auto destination = QDir(target->folder()).filePath(name);
                if (source.key == destination) {
                    ok = fail(tr("Can't copy '%1' into itself.").arg(source.name));
                    continue;
                }
                if (move && !QFileInfo::exists(destination)
                    && QDir().rename(source.key, destination))
                    continue;
            }
            if (target->copyIntoHost(*this, source, target->folder(), name, overwrite)) {
                if (move)
                    ok = removeSource(source) && ok;
            } else {
                ok = false;
            }
            continue;
        }
        if (move && sameMedia(*target) && filesystem() != u"tape"_s) {
            try {
                m_media->rename(source.key.toStdString(),
                                target->mediaChild(name, source.dir).toStdString(), overwrite);
            } catch (const std::exception &e) {
                ok = fail(QString::fromUtf8(e.what()));
            }
            continue;
        }
        const bool copied = target->m_media
                ? target->copyInto(*this, source, target->mediaChild(name, source.dir), overwrite)
                : target->copyIntoHost(*this, source, target->folder(), name, overwrite);
        if (copied) {
            if (move)
                ok = removeSource(source) && ok;
        } else {
            ok = false;
        }
    }
    if (target->m_media && !target->commit())
        ok = false;
    if (move && m_media && !sameMedia(*target) && !commit())
        ok = false;
    for (auto &entry : m_entries)
        entry.marked = false;
    peerRefresh();
    if (target != m_peer)
        target->refresh();
    return ok;
}

bool FileManagerPanel::startHostCopy(std::vector<Source> sources, FileManagerPanel *target,
                                     bool move, const QString &newName, bool overwrite)
{
    std::vector<Source> todo;
    for (auto &source : sources) {
        if (source.dir && target->m_location.inside(source.key))
            fail(tr("Can't copy '%1' into itself.").arg(source.name));
        else
            todo.push_back(std::move(source));
    }
    if (todo.empty())
        return false;
    setWorking(true);
    target->setWorking(true);
    QPointer<FileManagerPanel> into(target);
    QtConcurrent::run([todo, folder = target->folder(), move, newName, overwrite] {
        QStringList errors;
        for (const auto &source : todo) {
            const auto name = newName.isEmpty() ? source.name : newName;
            if (!copyHostEntry(source.key, source.dir, folder, name, overwrite, errors) || !move)
                continue;
            QString error;
            if (!StorageFiles::remove(source.key, error))
                errors.append(tr("Can't remove '%1'.").arg(source.name));
        }
        return errors;
    }).then(this, [this, into](const QStringList &errors) {
        for (const auto &error : errors)
            fail(error);
        for (auto &entry : m_entries)
            entry.marked = false;
        setWorking(false);
        if (into)
            into->setWorking(false);
        peerRefresh();
        if (into && into != m_peer)
            into->refresh();
        emit copyFinished(errors.isEmpty());
    });
    return true;
}

void FileManagerPanel::setWorking(bool working)
{
    if (m_working == working)
        return;
    m_working = working;
    emit busyChanged();
}

int FileManagerPanel::moveBlock(int row, int delta)
{
    if (!tapeBlocks() || row < 0 || row >= count())
        return -1;
    const auto index = blockIndex(m_entries[size_t(row)].key);
    if (!index)
        return -1;
    const auto blocks = std::ranges::count(m_entries, u"block"_s, &Entry::kind);
    const auto target = std::clamp<qint64>(qint64(*index) + delta, 0, blocks - 1);
    if (target == qint64(*index))
        return row;
    const auto name = blockName(std::size_t(target), m_media->path);
    try {
        m_media->rename(m_entries[size_t(row)].key.toStdString(), name.toStdString(), false);
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return -1;
    }
    if (!commit())
        return -1;
    peerRefresh();
    return rowOf(name.mid(7));
}

QVariantList FileManagerPanel::attributeFields(int row) const
{
    QVariantList fields;
    if (!m_media || row < 0 || row >= count())
        return fields;
    const auto &entry = m_entries[size_t(row)];
    if (entry.dir || entry.parent)
        return fields;
    const auto fs = filesystem();
    const auto type = attribute(entry.attributes, "type"sv);
    for (const auto &[key, value] : entry.attributes) {
        QVariantMap field{ { u"key"_s, fromStd(key) },
                           { u"value"_s, qulonglong(value) },
                           { u"readOnly"_s, readOnlyAttribute(key) },
                           { u"minimum"_s, 0 },
                           { u"maximum"_s, 65535 } };
        QString label = fromStd(key);
        QString kind = u"number"_s;
        if (booleanAttribute(key)) {
            kind = u"bool"_s;
            const QHash<QString, QString> names = {
                { u"readonly"_s, tr("Read-only") },    { u"hidden"_s, tr("Hidden") },
                { u"system"_s, tr("System") },         { u"archive"_s, tr("Archive") },
                { u"protected"_s, tr("Protected") },   { u"readable"_s, tr("Readable") },
                { u"executable"_s, tr("Executable") }, { u"deletable"_s, tr("Deletable") }
            };
            label = names.value(fromStd(key), label);
        } else if (key == "type"sv) {
            kind = u"choice"_s;
            label = tr("Type");
            QVariantList choices;
            const auto add = [&](uint64_t choice, const QString &text) {
                choices.append(
                        QVariantMap{ { u"value"_s, qulonglong(choice) }, { u"text"_s, text } });
            };
            if (trdos(fs)) {
                add('B', tr("B: BASIC"));
                add('C', tr("C: Code"));
                add('D', tr("D: Data"));
                add('#', tr("#: Stream"));
                if (value != 'B' && value != 'C' && value != 'D' && value != '#')
                    add(value, QString(QChar(char16_t(value))));
            } else {
                add(0, tr("Program"));
                add(1, tr("Number array"));
                add(2, tr("Character array"));
                add(3, tr("Bytes"));
                if (value > 3)
                    add(value, tr("Type %1").arg(value));
            }
            field[u"choices"_s] = choices;
        } else if (key == "param1"sv) {
            if (trdos(fs))
                label = type == 'C' ? tr("Start address") : tr("Parameter");
            else if (type == 0)
                label = tr("Autostart line (32768: none)");
            else if (type == 3)
                label = tr("Start address");
            else
                label = tr("Array variable");
        } else if (key == "param2"sv) {
            label = type == 0 ? tr("Program length (without variables)")
                              : tr("Second parameter (usually 32768)");
        } else if (key == "pause"sv) {
            label = tr("Pause after (ms)");
        } else if (key == "length"sv) {
            label = tr("Length");
        } else if (key == "user"sv) {
            label = tr("CP/M user area");
            field[u"maximum"_s] = 15;
        } else if (key == "autostart"sv) {
            label = tr("Autostart");
        }
        field[u"label"_s] = label;
        field[u"kind"_s] = kind;
        fields.append(field);
    }
    // The type and its parameters first, then the flags; what can't change goes last
    static constexpr std::array order = { "type"sv,   "param1"sv,    "param2"sv,
                                          "length"sv, "autostart"sv, "pause"sv };
    const auto rank = [](const QVariant &field) {
        const auto map = field.toMap();
        const auto key = map[u"key"_s].toString().toStdString();
        const auto known = std::ranges::find(order, key);
        if (map[u"readOnly"_s].toBool())
            return int(order.size()) + 2;
        if (known != order.end())
            return int(known - order.begin());
        return int(order.size()) + (map[u"kind"_s] == u"bool"_s ? 0 : 1);
    };
    std::ranges::stable_sort(fields, {}, rank);
    return fields;
}

bool FileManagerPanel::setAttributes(int row, const QVariantMap &values)
{
    if (!m_media)
        return false;
    const auto rows = targets(row);
    bool ok = true;
    bool changed = false;
    for (auto i : rows) {
        const auto &entry = m_entries[size_t(i)];
        if (entry.dir)
            continue;
        zxkit::MediaAttributes changes;
        for (auto it = values.begin(); it != values.end(); ++it) {
            const auto key = it.key().toStdString();
            if (readOnlyAttribute(key) || !entry.attributes.contains(key))
                continue;
            const auto value = it.value().toULongLong();
            if (entry.attributes.at(key) != value)
                changes[key] = value;
        }
        if (changes.empty())
            continue;
        try {
            m_media->setAttributes(basicPath(entry).toStdString(), changes);
            changed = true;
        } catch (const std::exception &e) {
            ok = fail(tr("%1: %2").arg(entry.name, QString::fromUtf8(e.what())));
        }
    }
    if (changed && !commit())
        ok = false;
    peerRefresh();
    return ok;
}

QVariantMap FileManagerPanel::view(int row) const
{
    if (row < 0 || row >= count())
        return {};
    const auto &entry = m_entries[size_t(row)];
    if (entry.dir || entry.parent)
        return {};
    std::vector<uint8_t> data;
    try {
        if (m_media) {
            data = readEntry(entry);
        } else {
            QString error;
            data = readHostFile(entry.key, std::max(MaxMediaBytes / 8, MaxTextBytes), error);
            if (!error.isEmpty()) {
                fail(error);
                return {};
            }
        }
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return {};
    }
    const auto fs = filesystem();
    QString title = entry.name;
    QVariantList pages;
    // ZxKit's views, with Spectacol's titles
    const auto addPages = [&](const std::vector<zxkit::MediaViewPage> &views) {
        using Kind = zxkit::MediaViewPage::Kind;
        for (const auto &view : views) {
            const auto lines = toQt(view.lines);
            switch (view.kind) {
            case Kind::Basic: {
                QVariantList parts;
                for (const auto &line : view.listing) {
                    QVariantList row;
                    for (const auto &part : line) {
                        QVariantMap map{ { u"k"_s, partKind(part.kind) },
                                         { u"t"_s, fromStd(part.text) } };
                        // A protected number: what LIST shows instead of the value used
                        if (part.listed)
                            map[u"l"_s] = fromStd(*part.listed);
                        row.append(map);
                    }
                    parts.append(QVariant(row));
                }
                auto page = linesPage(tr("Unhacked"), lines, u"basic"_s);
                page[u"parts"_s] = parts;
                pages.append(page);
                break;
            }
            case Kind::Unfiltered:
                pages.append(linesPage(tr("Unfiltered"), lines, u"lossless"_s));
                break;
            case Kind::Usr:
            case Kind::Disassembly: {
                // With its bytes and address, for going to an address inside an instruction
                auto page = linesPage(view.kind == Kind::Usr ? tr("USR %1").arg(view.address)
                                                             : tr("Disassembly"),
                                      lines, u"disassembly"_s);
                page[u"bytes"_s] = QByteArray(reinterpret_cast<const char *>(view.bytes.data()),
                                              qsizetype(view.bytes.size()));
                page[u"origin"_s] = int(view.address);
                pages.append(page);
                break;
            }
            case Kind::Screen:
            case Kind::AsScreen:
                pages.append(QVariantMap{
                        { u"title"_s, view.kind == Kind::Screen ? tr("Screen") : tr("As screen") },
                        { u"image"_s, imageUrl(*view.screen) } });
                break;
            case Kind::Hex:
                pages.append(linesPage(tr("Hex"), lines, u"hex"_s));
                break;
            case Kind::Raw:
                pages.append(
                        linesPage(entry.key.endsWith(u".tap"_s) ? tr("Raw TAP") : tr("Raw TZX"),
                                  lines, u"hex"_s));
                break;
            case Kind::Text:
                pages.append(linesPage(tr("Text"), lines));
                break;
            case Kind::Array:
                pages.append(linesPage(tr("Array"), lines));
                break;
            case Kind::Block:
                if (!lines.isEmpty())
                    title = tr("Block %1: %2")
                                    .arg(blockIndex(entry.key).value_or(0))
                                    .arg(lines.first());
                pages.append(linesPage(tr("Block"), lines));
                break;
            }
        }
    };
    try {
        if (m_media && fs == u"tape"_s && entry.key.startsWith(u"basic/"_s)) {
            // The listing file is the editable text: show the program like the other views do
            try {
                addPages(zxkit::viewMedia(m_media->read(basicPath(entry).toStdString()),
                                          zxkit::MediaContent::Basic));
            } catch (const std::exception &) {
            }
            pages.append(linesPage(
                    tr("Editable text"),
                    textLines(QString::fromUtf8(reinterpret_cast<const char *>(data.data()),
                                                qsizetype(data.size())))));
        } else if (!m_media && entry.media) {
            // An image to open: its bytes
            pages.append(linesPage(tr("Hex"), toQt(zxkit::hexLines(data, 0)), u"hex"_s));
        } else if (m_media) {
            addPages(zxkit::viewMedia(
                    data,
                    zxkit::mediaContent(fs.toStdString(), entry.key.toStdString(), entry.attributes,
                                        data.size()),
                    zxkit::mediaLoadAddress(entry.key.toStdString(), entry.attributes),
                    entry.name.toStdString()));
        } else {
            addPages(zxkit::viewMedia(data,
                                      zxkit::fileContent(entry.name.toStdString(), data.size()), 0,
                                      entry.name.toStdString()));
        }
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return {};
    }
    QString subtitle = entry.info;
    if (entry.size >= 0)
        subtitle = (subtitle.isEmpty() ? QString() : subtitle + u", "_s)
                + tr("%n byte(s)", nullptr, int(entry.size));
    return { { u"title"_s, title },
             { u"name"_s, entry.name },
             { u"subtitle"_s, subtitle },
             { u"pages"_s, pages } };
}

QVariantMap FileManagerPanel::editable(int row) const
{
    if (row < 0 || row >= count())
        return {};
    const auto &entry = m_entries[size_t(row)];
    if (entry.dir || entry.parent)
        return {};
    try {
        // Text files: by name, or any untyped file that reads as text
        if (entry.kind == u"text"_s || entry.kind == u"file"_s) {
            if (entry.size > MaxTextBytes)
                return {};
            const auto text = zxkit::mediaText(readEntry(entry));
            if (!text)
                return {};
            QVariantMap result{ { u"title"_s, entry.name }, { u"text"_s, fromStd(text->text) } };
            if (text->bit7)
                result[u"note"_s] =
                        tr("Saving keeps 7-bit ASCII: WordStar's bit-7 formatting is dropped.");
            return result;
        }
        if (!m_media || entry.kind != u"basic"_s)
            return {};
        const auto fs = filesystem();
        if (fs == u"tape"_s) {
            auto key = entry.key;
            if (key.startsWith(u"files/"_s))
                key = u"basic/"_s + key.mid(6) + u".bas"_s;
            const auto bytes = m_media->read(key.toStdString());
            return { { u"title"_s, entry.name },
                     { u"basic"_s, true },
                     { u"text"_s,
                       QString::fromUtf8(reinterpret_cast<const char *>(bytes.data()),
                                         qsizetype(bytes.size())) } };
        }
        // TR-DOS keeps its autostart line after the program: edit tapes and +3DOS-like disks
        if (trdos(fs))
            return {};
        const auto bytes = readEntry(entry);
        return {
            { u"title"_s, entry.name },
            { u"basic"_s, true },
            { u"text"_s,
              fromStd(zxkit::BasicCodec::decompile(bytes, zxkit::BasicDialect::Spectrum128)) }
        };
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
        return {};
    }
}

bool FileManagerPanel::saveText(int row, const QString &text)
{
    if (row < 0 || row >= count())
        return false;
    const auto entry = m_entries[size_t(row)];
    auto utf8 = text.toUtf8();
    if (entry.kind != u"basic"_s) {
        // A text file keeps its encoding and line endings: CR LF on CP/M, CR on the Spectrum
        try {
            if (const auto content = zxkit::mediaText(readEntry(entry))) {
                if (!content->utf8)
                    utf8 = text.toLatin1();
                if (content->lineEnding != "\n"sv)
                    utf8.replace("\n", QByteArray::fromStdString(content->lineEnding));
            }
        } catch (const std::exception &) {
        }
    }
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t *>(utf8.constData()),
                                         std::size_t(utf8.size()));
    if (!m_media) {
        QString error;
        if (!writeHostFile(entry.key, bytes, error))
            return fail(error);
        peerRefresh();
        return true;
    }
    try {
        if (entry.kind != u"basic"_s) {
            m_media->put(entry.key.toStdString(), bytes, {});
        } else if (filesystem() == u"tape"_s) {
            auto key = entry.key;
            if (key.startsWith(u"files/"_s))
                key = u"basic/"_s + key.mid(6) + u".bas"_s;
            m_media->put(key.toStdString(), bytes, {});
        } else {
            const auto program = zxkit::BasicCodec::compile(
                    std::string_view(utf8.constData(), std::size_t(utf8.size())),
                    zxkit::BasicDialect::Spectrum128);
            m_media->put(entry.key.toStdString(), program,
                         { { "type"s, 0 }, { "param2"s, programLength(program) } });
        }
    } catch (const std::exception &e) {
        return fail(QString::fromUtf8(e.what()));
    }
    if (!commit())
        return false;
    peerRefresh();
    return true;
}

QVariantList FileManagerPanel::imageFormats() const
{
    const auto format = [](const QString &text, const QString &suffix, const QString &filesystem) {
        return QVariantMap{ { u"text"_s, text },
                            { u"suffix"_s, suffix },
                            { u"filesystem"_s, filesystem } };
    };
    return { format(tr("TZX tape"), u"tzx"_s, {}),
             format(tr("TAP tape"), u"tap"_s, {}),
             format(tr("+3 disk (DSK)"), u"dsk"_s, u"plus3"_s),
             format(tr("TR-DOS disk (TRD)"), u"trd"_s, u"trdos"_s),
             format(tr("TR-DOS disk (SCL)"), u"scl"_s, {}),
             format(tr("+D / DISCiPLE disk (MGT)"), u"mgt"_s, u"mgt"_s),
             format(tr("Opus Discovery disk (OPD)"), u"opd"_s, u"opus"_s),
             format(tr("HC BASIC 640K disk (DSK)"), u"dsk"_s, u"hc-basic-80"_s),
             format(tr("CP/M 720K disk (DSK)"), u"dsk"_s, u"cpm-720"_s),
             format(tr("Microdrive cartridge (MDR)"), u"mdr"_s, {}) };
}

bool FileManagerPanel::createImage(const QString &name, int format)
{
    if (m_media)
        return fail(tr("Create images in a host folder."));
    const auto formats = imageFormats();
    if (format < 0 || format >= formats.size())
        return false;
    const auto info = formats[format].toMap();
    const auto suffix = info[u"suffix"_s].toString();
    auto fileName = name.trimmed();
    if (fileName.isEmpty() || fileName.contains(u'/'))
        return fail(tr("Invalid name."));
    if (QFileInfo(fileName).suffix().toLower() != suffix)
        fileName += u'.' + suffix;
    if (StorageFiles::find(folder(), fileName))
        return fail(tr("'%1' already exists.").arg(fileName));
    std::vector<uint8_t> bytes;
    try {
        zxkit::MediaOptions options;
        options.filesystem = info[u"filesystem"_s].toString().toStdString();
        options.label = QFileInfo(fileName).completeBaseName().left(10).toStdString();
        bytes = zxkit::MediaArchive::create((u'.' + suffix).toStdString(), options).encode();
    } catch (const std::exception &e) {
        return fail(QString::fromUtf8(e.what()));
    }
    QString error;
    if (StorageFiles::create(folder(), fileName, bytes, error).isEmpty())
        return fail(error);
    peerRefresh();
    return true;
}

int FileManagerPanel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant FileManagerPanel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= count())
        return {};
    const auto &entry = m_entries[size_t(index.row())];
    switch (role) {
    case Name:
        return entry.name;
    case Label:
        return entry.label.isEmpty() ? entry.name : entry.label;
    case Path:
        return entry.key;
    case IsDir:
        return entry.dir;
    case IsParent:
        return entry.parent;
    case IsMedia:
        return entry.media;
    case Size:
        return entry.size;
    case Info:
        return entry.info;
    case Marked:
        return entry.marked;
    case Kind:
        return entry.parent ? u"parent"_s : entry.kind;
    case IsDevice:
        return entry.device;
    }
    return {};
}

QHash<int, QByteArray> FileManagerPanel::roleNames() const
{
    return { { Name, "name" },         { Path, "path" },        { IsDir, "isDir" },
             { IsParent, "isParent" }, { IsMedia, "isMedia" },  { Size, "size" },
             { Info, "info" },         { Marked, "marked" },    { Kind, "kind" },
             { Label, "label" },       { IsDevice, "isDevice" } };
}

QString FileManagerPanel::topmostFolder() const
{
    return m_location.root();
}

void FileManagerPanel::setTopmostFolder(const QString &newTopmostFolder)
{
    const auto previous = m_location.root();
    m_location.setRoot(newTopmostFolder);
    if (m_location.root() == previous)
        return;
    emit topmostFolderChanged();
    if (!m_location.allowed(folder()))
        setFolder(m_location.root());
    else if (!m_media)
        reload(); // ".." comes or goes
}
