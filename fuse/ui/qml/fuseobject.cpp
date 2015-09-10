#include "fuseobject.h"

#include <QCoreApplication>
#include <QEvent>
#include <QThread>

class FunctionEvent : public QEvent
{
public:
    FunctionEvent(const FuseObject::Function &function)
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

void FuseObject::callFunction(const FuseObject::Function &func)
{
    if (QThread::currentThread() == thread())
        func();
    else
        QCoreApplication::postEvent(this, new FunctionEvent(func));
}

bool FuseObject::event(QEvent *ev)
{
    if (ev->type() == QEvent::User) {
        static_cast<FunctionEvent*>(ev)->callFunction();
        return true;
    }
    return QObject::event(ev);
}

