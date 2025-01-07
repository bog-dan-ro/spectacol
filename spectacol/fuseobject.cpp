/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#include "fuseobject.h"

#include <QCoreApplication>
#include <QEvent>
#include <QSemaphore>
#include <QTimer>
#include <QThread>

class FunctionEvent : public QEvent
{
public:
    explicit FunctionEvent(const FuseObject::Function &function)
        : QEvent(User)
        , m_function(function)
    {}

    void callFunction() const
    {
        m_function();
    }
private:
    FuseObject::Function m_function;
};

FuseObject::FuseObject(QObject *parent) : QObject(parent)
{
}

void FuseObject::callFunction(const Function &func)
{
    if (QThread::currentThread() == thread())
        func();
    else
        QCoreApplication::postEvent(this, new FunctionEvent(func));
}

void FuseObject::callFunctionDelayed(int ms, const Function &func)
{
    QTimer::singleShot(ms, this, func);
}

bool FuseObject::event(QEvent *ev)
{
    if (ev->type() == QEvent::User) {
        static_cast<FunctionEvent*>(ev)->callFunction();
        return true;
    }
    return QObject::event(ev);
}

