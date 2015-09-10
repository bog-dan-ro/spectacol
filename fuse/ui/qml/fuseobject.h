#ifndef FUSEOBJECT_H
#define FUSEOBJECT_H

#include <QObject>
#include <functional>

class FuseObject : public QObject
{
    Q_OBJECT
public:
    explicit FuseObject(QObject *parent = 0);
    typedef std::function<void()> Function;
    void callFunction(const Function &func);
    bool event(QEvent *);

signals:

public slots:
};

#endif // FUSEOBJECT_H
