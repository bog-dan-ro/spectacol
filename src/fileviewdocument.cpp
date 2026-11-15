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

#include "fileviewdocument.h"

#include <QFontMetricsF>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace Qt::StringLiterals;

namespace {
QTextCharFormat coloured(const QColor &colour, bool italic = false)
{
    QTextCharFormat format;
    if (colour.isValid())
        format.setForeground(colour);
    format.setFontItalic(italic);
    return format;
}
} // namespace

FileViewDocument::FileViewDocument(QObject *parent) : QObject(parent)
{
    connect(this, &FileViewDocument::colorsChanged, this, &FileViewDocument::rebuild);
}

void FileViewDocument::setDocument(QQuickTextDocument *document)
{
    if (m_document == document)
        return;
    m_document = document;
    emit documentChanged();
    rebuild();
}

void FileViewDocument::setPage(const QVariantMap &page)
{
    m_page = page;
    rebuild();
    emit pageChanged();
}

int FileViewDocument::lineCount() const
{
    const auto *text = m_document ? m_document->textDocument() : nullptr;
    return text ? text->blockCount() : 0;
}

int FileViewDocument::lineOf(int position) const
{
    const auto *text = m_document ? m_document->textDocument() : nullptr;
    if (!text)
        return 0;
    const auto block = text->findBlock(position);
    return block.isValid() ? block.blockNumber() : std::max(0, text->blockCount() - 1);
}

int FileViewDocument::lineStart(int line) const
{
    const auto *text = m_document ? m_document->textDocument() : nullptr;
    const auto block = text ? text->findBlockByNumber(line) : QTextBlock();
    return block.isValid() ? block.position() : 0;
}

int FileViewDocument::lineEnd(int line) const
{
    const auto *text = m_document ? m_document->textDocument() : nullptr;
    const auto block = text ? text->findBlockByNumber(line) : QTextBlock();
    return block.isValid() ? block.position() + block.length() - 1 : 0;
}

void FileViewDocument::rebuild()
{
    auto *text = m_document ? m_document->textDocument() : nullptr;
    if (!text)
        return;
    const auto style = m_page.value(u"style"_s).toString();
    const auto lines = m_page.value(u"lines"_s).toStringList();
    const auto parts = m_page.value(u"parts"_s).toList();
    const QTextCharFormat plain;
    const auto muted = coloured(m_muted);

    text->setUndoRedoEnabled(false);
    QTextCursor cursor(text);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();

    QTextBlockFormat blockFormat;
    if (style == u"basic"_s) {
        // A wrapped line continues right of its line number
        const qreal column = QFontMetricsF(text->defaultFont()).horizontalAdvance(u"0000 "_s);
        blockFormat.setLeftMargin(column);
        blockFormat.setTextIndent(-column);
    }
    cursor.setBlockFormat(blockFormat);

    const auto newLine = [&cursor, &blockFormat](qsizetype index) {
        if (index > 0)
            cursor.insertBlock(blockFormat);
    };
    if (style == u"basic"_s) {
        const QHash<QString, QTextCharFormat> formats{
            { u"keyword"_s, coloured(m_keyword) }, { u"number"_s, coloured(m_number) },
            { u"string"_s, coloured(m_string) },   { u"comment"_s, coloured(m_muted, true) },
            { u"control"_s, coloured(m_control) }, { u"code"_s, coloured(m_muted, true) }
        };
        auto decoy = muted;
        decoy.setFontStrikeOut(true);
        auto used = coloured(m_protected);
        used.setFontWeight(QFont::Bold);
        for (qsizetype i = 0; i < parts.size(); ++i) {
            newLine(i);
            const auto line = parts[i].toList();
            for (qsizetype j = 0; j < line.size(); ++j) {
                const auto part = line[j].toMap();
                const auto partText = part.value(u"t"_s).toString();
                if (j == 0) {
                    cursor.insertText(partText.leftJustified(4) + u' ', muted);
                } else if (part.contains(u"l"_s)) {
                    // A protected number: LIST's decoy struck through, then the value used
                    const auto listed = part.value(u"l"_s).toString();
                    if (!listed.isEmpty()) {
                        cursor.insertText(listed, decoy);
                        cursor.insertText(u" "_s, plain);
                    }
                    cursor.insertText(partText, used);
                } else {
                    cursor.insertText(partText, formats.value(part.value(u"k"_s).toString()));
                }
            }
        }
    } else if (style == u"lossless"_s) {
        // \{KEYWORD} tokens, \xHH bytes, strings and !raw data
        static const QRegularExpression pattern(
                uR"((\\\{[^}]*\})|(\\x[0-9A-Fa-f]{2})|("[^"]*"?)|(!raw .*))"_s);
        const auto keyword = coloured(m_keyword);
        const auto string = coloured(m_string);
        for (qsizetype i = 0; i < lines.size(); ++i) {
            newLine(i);
            const auto &line = lines[i];
            qsizetype at = 0;
            for (auto match = pattern.globalMatch(line); match.hasNext();) {
                const auto found = match.next();
                cursor.insertText(line.mid(at, found.capturedStart() - at), plain);
                cursor.insertText(found.captured(),
                                  found.hasCaptured(1)           ? keyword
                                          : found.hasCaptured(3) ? string
                                                                 : muted);
                at = found.capturedEnd();
            }
            cursor.insertText(line.mid(at), plain);
        }
    } else {
        cursor.setCharFormat(plain);
        cursor.insertText(lines.join(u'\n'), plain);
    }
    cursor.endEditBlock();
}
