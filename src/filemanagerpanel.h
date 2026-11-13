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

#include <QAbstractListModel>
#include <QDateTime>
#include <QPointer>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>

#include <zxkit/media/MediaArchive.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

class FileManagerMedia;

/// One panel of the twin-panel file manager. It shows a host folder, or a folder inside a
/// tape, disk, microdrive or NEX image through ZxKit's MediaArchive, or inside a FAT card image
/// (the Next's SD card) through FatVolume. Images inside images open too. Changes are written
/// back at once: atomically to an image file, in place to a card, into the image holding a
/// nested one. Operations apply to the marked entries, or to the given row when nothing is
/// marked. Failures are reported through failed().
class FileManagerPanel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY locationChanged FINAL)
    Q_PROPERTY(QString location READ location NOTIFY locationChanged FINAL)
    Q_PROPERTY(QString mediaPath READ mediaPath NOTIFY locationChanged FINAL)
    Q_PROPERTY(QString mediaFolder READ mediaFolder NOTIFY locationChanged FINAL)
    Q_PROPERTY(QString filesystem READ filesystem NOTIFY locationChanged FINAL)
    Q_PROPERTY(bool inMedia READ inMedia NOTIFY locationChanged FINAL)
    Q_PROPERTY(bool writable READ writable NOTIFY locationChanged FINAL)
    Q_PROPERTY(bool canMakeFolder READ canMakeFolder NOTIFY locationChanged FINAL)
    Q_PROPERTY(bool tapeBlocks READ tapeBlocks NOTIFY locationChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int markedCount READ markedCount NOTIFY markedChanged FINAL)
    Q_PROPERTY(FileManagerPanel *peer READ peer WRITE setPeer NOTIFY peerChanged FINAL)

public:
    enum Roles {
        Name = Qt::DisplayRole,
        Path = Qt::UserRole,
        IsDir,
        IsParent,
        IsMedia,
        Size,
        Info,
        Marked,
        Kind,
        Label // what a row shows: the name, or a tape block's number and contents
    };
    Q_ENUM(Roles)

    explicit FileManagerPanel(QObject *parent = nullptr);
    ~FileManagerPanel() override;

    /// The host folder: the one shown, or the one holding the open media image
    QString folder() const
    {
        return m_folder;
    }
    void setFolder(const QString &folder);
    /// The folder, then the image and the folder inside it, for the panel's title
    QString location() const;
    QString mediaPath() const;
    QString mediaFolder() const
    {
        return m_mediaFolder;
    }
    /// MediaArchive::filesystem() of the open image, empty for host folders
    QString filesystem() const;
    bool inMedia() const
    {
        return bool(m_media);
    }
    bool writable() const;
    bool canMakeFolder() const;
    /// In a tape's blocks/ folder, whose blocks can be reordered
    bool tapeBlocks() const;
    int count() const
    {
        return int(m_entries.size());
    }
    int markedCount() const;
    FileManagerPanel *peer() const
    {
        return m_peer;
    }
    void setPeer(FileManagerPanel *peer);

    /// Opens a folder or media image. False for a plain file (view it instead).
    Q_INVOKABLE bool open(int row, const QString &filesystem = {});
    /// The parent folder (leaving the image from its root); returns the row of the folder or
    /// image left, to keep the cursor on it.
    Q_INVOKABLE int up();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleMark(int row);
    Q_INVOKABLE void markAll(bool marked);
    /// The names an operation on `row` acts on, for confirmations
    Q_INVOKABLE QStringList targetNames(int row) const;
    /// The host file to load in the emulator: a host entry, or the top image; empty inside a
    /// card or a nested image
    Q_INVOKABLE QString hostPath(int row) const;
    Q_INVOKABLE bool sameFile(const QString &a, const QString &b) const;
    Q_INVOKABLE void copyText(const QString &text) const;
    /// A viewer's screen (a PNG data URL)
    Q_INVOKABLE void copyImage(const QString &dataUrl) const;
    /// Saves a viewer's screen as `name`.png in this panel's host folder; returns its path
    Q_INVOKABLE QString saveImage(const QString &dataUrl, const QString &name);
    /// A viewer's disassembly again, restarting at `address` inside an instruction
    Q_INVOKABLE QStringList disassemble(const QByteArray &bytes, int origin, int address) const;
    /// A typed address: decimal (23803) or hex ($5CFB, #5CFB, 0x5CFB, 5CFBh, or with A to F);
    /// -1 when it is none
    Q_INVOKABLE int parseAddress(const QString &text) const;
    Q_INVOKABLE int rowOf(const QString &name) const;

    Q_INVOKABLE bool makeFolder(const QString &name);
    Q_INVOKABLE bool rename(int row, const QString &newName);
    /// Removes files and, recursively, folders
    Q_INVOKABLE bool remove(int row);
    /// The target names that already exist in `target`
    Q_INVOKABLE QStringList conflicts(int row, FileManagerPanel *target,
                                      const QString &newName = {}) const;
    /// Copies (or moves) into the target panel's location. `newName` renames a single entry.
    Q_INVOKABLE bool copyTo(int row, FileManagerPanel *target, bool move = false,
                            const QString &newName = {}, bool overwrite = false);
    /// Moves a tape block by `delta` positions; returns its new row, or -1
    Q_INVOKABLE int moveBlock(int row, int delta);

    /// The editable attributes: {key, label, kind ("bool", "number", "choice"), value,
    /// readOnly, minimum, maximum, choices: [{value, text}]}
    Q_INVOKABLE QVariantList attributeFields(int row) const;
    Q_INVOKABLE bool setAttributes(int row, const QVariantMap &values);

    /// The viewer's pages: {title, pages: [{title, lines} or {title, image}]}
    Q_INVOKABLE QVariantMap view(int row) const;
    /// {title, text} for BASIC programs and host text files; empty when not editable
    Q_INVOKABLE QVariantMap editable(int row) const;
    Q_INVOKABLE bool saveText(int row, const QString &text);

    /// The image formats a new image can have: [{text, suffix, filesystem}]
    Q_INVOKABLE QVariantList imageFormats() const;
    Q_INVOKABLE bool createImage(const QString &name, int format);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void locationChanged();
    void countChanged();
    void markedChanged();
    void peerChanged();
    void failed(const QString &message);
    /// Several filesystems read the image: open(row, profile) with one of the choices,
    /// [{profile, files}]
    void filesystemNeeded(int row, const QVariantList &choices, const QString &message);
    /// A host file was changed: the emulator may have to read it again (the SD card)
    void mediaWritten(const QString &path);

private:
    struct Entry
    {
        QString name;
        QString key; // the host path, or the media entry path ("files/NAME")
        qint64 size = -1;
        bool dir = false;
        bool parent = false;
        bool media = false;
        bool marked = false;
        QString kind;
        QString info;
        QString label;
        zxkit::MediaAttributes attributes;
    };
    // An image holding the open one: where it was, and the entry
    struct Parent
    {
        std::shared_ptr<FileManagerMedia> media;
        QString folder;
        QString name;
    };
    struct Source
    {
        QString name; // the leaf name
        QString key;
        bool dir = false;
        zxkit::MediaAttributes attributes;
    };

    void reload();
    void listHost();
    void listMedia();
    bool fail(const QString &message) const;
    bool commit();
    bool openImage(int row, const QString &filesystem);
    std::shared_ptr<FileManagerMedia> peerMedia(const QString &key) const;
    std::vector<int> targets(int row) const;
    std::vector<uint8_t> readEntry(const Entry &entry) const;
    std::vector<uint8_t> readSource(const FileManagerPanel &from, const Source &source) const;
    QString mediaChild(const QString &name, bool folder = false) const;
    QString mediaRoot() const;
    bool sameMedia(const FileManagerPanel &other) const;
    void peerRefresh();
    bool putMedia(const QString &name, std::span<const uint8_t> bytes,
                  const zxkit::MediaAttributes &attributes);
    bool copyInto(const FileManagerPanel &from, const Source &source, const QString &name,
                  bool overwrite);
    bool removeSource(const Source &source);
    QString basicPath(const Entry &entry) const;

    QString m_folder;
    std::shared_ptr<FileManagerMedia> m_media;
    QString m_mediaFolder; // "" is the image's root
    std::vector<Parent> m_parents; // outermost first
    std::vector<Entry> m_entries;
    QPointer<FileManagerPanel> m_peer;
};
