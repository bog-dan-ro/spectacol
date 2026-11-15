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

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

/// Fills a read-only TextEdit's document (TextEdit::textDocument) with one of the file viewer's
/// source pages and styles it: a BASIC listing's keywords, numbers, strings and comments with
/// wrapped lines hanging right of their line number, the lossless listing's escapes, or plain
/// text. One block is one line of the page.
class FileViewDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged
                       FINAL)
    /// A page of FileManagerPanel::view(): {lines, style, parts}
    Q_PROPERTY(QVariantMap page READ page WRITE setPage NOTIFY pageChanged FINAL)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY pageChanged FINAL)
    Q_PROPERTY(QColor keywordColor MEMBER m_keyword NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor numberColor MEMBER m_number NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor stringColor MEMBER m_string NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor controlColor MEMBER m_control NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor mutedColor MEMBER m_muted NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor protectedColor MEMBER m_protected NOTIFY colorsChanged FINAL)

public:
    explicit FileViewDocument(QObject *parent = nullptr);

    QQuickTextDocument *document() const
    {
        return m_document;
    }
    void setDocument(QQuickTextDocument *document);
    QVariantMap page() const
    {
        return m_page;
    }
    void setPage(const QVariantMap &page);
    int lineCount() const;

    /// The line a document position is in
    Q_INVOKABLE int lineOf(int position) const;
    /// Where a line starts and ends in the document, for cursors and selections
    Q_INVOKABLE int lineStart(int line) const;
    Q_INVOKABLE int lineEnd(int line) const;

signals:
    void documentChanged();
    void pageChanged();
    void colorsChanged();

private:
    void rebuild();

    QPointer<QQuickTextDocument> m_document;
    QVariantMap m_page;
    QColor m_keyword;
    QColor m_number;
    QColor m_string;
    QColor m_control;
    QColor m_muted;
    QColor m_protected;
};
