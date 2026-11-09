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

#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class DesktopInput : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool supported READ supported CONSTANT FINAL)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QString text READ text NOTIFY textChanged FINAL)
    Q_PROPERTY(
            bool physicalKeyboardPresent READ physicalKeyboardPresent NOTIFY devicesChanged FINAL)

public:
    explicit DesktopInput(QObject *parent = nullptr);
    explicit DesktopInput(const QString &inputDirectory, QObject *parent = nullptr);

    static bool supported();
    static bool hasPhysicalKeyboard(const QString &inputDirectory);
    bool physicalKeyboardPresent() const;
    void refreshDevices();
    bool isVisible() const;
    QString text() const;
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void commitText(const QString &text);
    Q_INVOKABLE void sendKey(int key);

signals:
    void devicesChanged();
    void visibleChanged();
    void textChanged();
    void navigationKey(int key, bool pressed);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    bool needsVirtualKeyboard() const;
    void updateInputPanel();
    void scheduleInputPanel();
    void setVisible(bool visible);

    QString m_inputDirectory;
    QTimer m_devicePoll;
    QSet<int> m_dismissedKeys;
    bool m_physicalKeyboardPresent = true;
    bool m_visible = false;
};
