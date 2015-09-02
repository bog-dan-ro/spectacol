#include "fuselistmodel.h"

#include <QCoreApplication>
#include <QEvent>
#include <QThread>

class FunctionEvent : public QEvent
{
public:
    FunctionEvent(const FuseListModel::Function &function)
        : QEvent(User)
        , m_function(function)
    {}

    void callFunction() const
    {
        m_function();
    }
private:
    FuseListModel::Function m_function;
};

FuseListModel::FuseListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void FuseListModel::callFunction(const FuseListModel::Function &func)
{
    if (QThread::currentThread() == thread())
        func();
    else
        QCoreApplication::postEvent(this, new FunctionEvent(func));
}

bool FuseListModel::event(QEvent *ev)
{
    if (ev->type() == QEvent::User) {
        static_cast<FunctionEvent*>(ev)->callFunction();
        return true;
    }
    return QObject::event(ev);
}

