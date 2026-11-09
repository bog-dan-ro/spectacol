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

#include "desktopinput.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGamepadManager>
#include <QGuiApplication>
#include <QInputDevice>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QWindow>

#ifdef Q_OS_LINUX
#include <linux/input.h>
#endif

using namespace Qt::StringLiterals;

namespace {
QByteArray readDeviceFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll().trimmed() : QByteArray{};
}

#ifdef Q_OS_LINUX
bool hasKey(const QList<QByteArray> &words, int key)
{
    constexpr int bitsPerWord = sizeof(unsigned long) * 8;
    const int word = words.size() - 1 - key / bitsPerWord;
    if (word < 0)
        return false;
    bool valid = false;
    const auto bits = words[word].toULongLong(&valid, 16);
    return valid && (bits & (quint64(1) << (key % bitsPerWord)));
}
#endif
} // namespace

DesktopInput::DesktopInput(QObject *parent) : DesktopInput(u"/sys/class/input"_s, parent) { }

DesktopInput::DesktopInput(const QString &inputDirectory, QObject *parent)
    : QObject(parent), m_inputDirectory(inputDirectory)
{
    if (!supported())
        return;

    m_physicalKeyboardPresent = hasPhysicalKeyboard(m_inputDirectory);
    qGuiApp->installEventFilter(this);
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &DesktopInput::scheduleInputPanel);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            &DesktopInput::scheduleInputPanel);
    connect(qGuiApp->inputMethod(), &QInputMethod::cursorRectangleChanged, this,
            &DesktopInput::textChanged);
    connect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this,
            &DesktopInput::scheduleInputPanel);
    // udev hotplug notifications are not reliable inside a Flatpak sandbox.
    m_devicePoll.setInterval(1000);
    connect(&m_devicePoll, &QTimer::timeout, this, &DesktopInput::refreshDevices);
    m_devicePoll.start();
    scheduleInputPanel();
}

bool DesktopInput::supported()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return false;
#else
    return true;
#endif
}

bool DesktopInput::hasPhysicalKeyboard(const QString &inputDirectory)
{
#ifdef Q_OS_LINUX
    const QDir inputs(inputDirectory);
    for (const auto &entry : inputs.entryList({ u"event*"_s }, QDir::Dirs)) {
        const QString device = inputs.filePath(entry + u"/device/"_s);
        // Steam Input's virtual keyboard and the Deck controller's keyboard
        // interface must not suppress the on-screen keyboard.
        if (QFileInfo(device).canonicalFilePath().contains(u"/virtual/"_s)
            || readDeviceFile(device + u"id/bustype"_s).toUInt(nullptr, 16) == BUS_VIRTUAL
            || readDeviceFile(device + u"id/vendor"_s).toUInt(nullptr, 16) == 0x28de)
            continue;
        const auto words = readDeviceFile(device + u"capabilities/key"_s).simplified().split(' ');
        if (hasKey(words, KEY_Q) && hasKey(words, KEY_A) && hasKey(words, KEY_Z)
            && hasKey(words, KEY_SPACE) && hasKey(words, KEY_ENTER))
            return true;
    }
    return false;
#else
    Q_UNUSED(inputDirectory);
    for (const auto *device : QInputDevice::devices()) {
        if (device->type() == QInputDevice::DeviceType::Keyboard)
            return true;
    }
    return false;
#endif
}

bool DesktopInput::physicalKeyboardPresent() const
{
    return m_physicalKeyboardPresent;
}

void DesktopInput::refreshDevices()
{
    const bool present = hasPhysicalKeyboard(m_inputDirectory);
    if (present == m_physicalKeyboardPresent)
        return;
    m_physicalKeyboardPresent = present;
    emit devicesChanged();
    scheduleInputPanel();
}

void DesktopInput::scheduleInputPanel()
{
    // The focus object's input-method queries settle after focus delivery.
    QTimer::singleShot(0, this, &DesktopInput::updateInputPanel);
}

void DesktopInput::updateInputPanel()
{
    if (!supported())
        return;
    show();
    emit textChanged();
}

bool DesktopInput::isVisible() const
{
    return m_visible;
}

void DesktopInput::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    m_visible = visible;
    emit visibleChanged();
}

void DesktopInput::show()
{
    const bool editable = QInputMethod::queryFocusObject(Qt::ImEnabled, {}).toBool();
    setVisible(supported() && needsVirtualKeyboard() && editable
               && qGuiApp->applicationState() == Qt::ApplicationActive);
}

bool DesktopInput::needsVirtualKeyboard() const
{
    return !m_physicalKeyboardPresent
            || !QGamepadManager::instance()->connectedGamepads().isEmpty();
}

void DesktopInput::hide()
{
    setVisible(false);
}

QString DesktopInput::text() const
{
    const auto text = QInputMethod::queryFocusObject(Qt::ImSurroundingText, {}).toString();
    const auto hints = QInputMethod::queryFocusObject(Qt::ImHints, {}).toInt();
    return hints & Qt::ImhHiddenText ? QString(text.size(), u'\u2022') : text;
}

void DesktopInput::commitText(const QString &text)
{
    if (!m_visible || !qGuiApp->focusObject())
        return;
    QInputMethodEvent event;
    event.setCommitString(text);
    QCoreApplication::sendEvent(qGuiApp->focusObject(), &event);
    emit textChanged();
}

void DesktopInput::sendKey(int key)
{
    if (!m_visible)
        return;
    QPointer<QObject> target = qGuiApp->focusObject();
    if (!target)
        return;
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &press);
    if (target) {
        QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
        QCoreApplication::sendEvent(target, &release);
    }
    emit textChanged();
}

bool DesktopInput::eventFilter(QObject *object, QEvent *event)
{
    if (object != qGuiApp->focusWindow()
        || (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease))
        return false;

    const auto *keyEvent = static_cast<QKeyEvent *>(event);
    const int key = keyEvent->key();
    const bool pressed = event->type() == QEvent::KeyPress;
    const bool navigationEvent = keyEvent->nativeScanCode() == 0;
    if (m_dismissedKeys.contains(key)) {
        if (!pressed)
            m_dismissedKeys.remove(key);
        return true;
    }
    if (m_visible) {
        if (navigationEvent && key == Qt::Key_Backspace) {
            if (pressed)
                sendKey(Qt::Key_Backspace);
            return true;
        }
        if (key == Qt::Key_Escape || key == Qt::Key_Back) {
            if (pressed) {
                m_dismissedKeys.insert(key);
                hide();
            }
            return true;
        }
        if (navigationEvent
            && ((key >= Qt::Key_Left && key <= Qt::Key_Down) || key == Qt::Key_Return
                || key == Qt::Key_Select || key == Qt::Key_Enter)) {
            emit navigationKey(key == Qt::Key_Select || key == Qt::Key_Enter ? Qt::Key_Return : key,
                               pressed);
            return true;
        }
    } else if (pressed && navigationEvent && needsVirtualKeyboard()
               && (key == Qt::Key_Return || key == Qt::Key_Select)
               && QInputMethod::queryFocusObject(Qt::ImEnabled, {}).toBool()) {
        m_dismissedKeys.insert(key);
        show();
        return true;
    }
    return false;
}
