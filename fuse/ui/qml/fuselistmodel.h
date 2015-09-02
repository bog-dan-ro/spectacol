#ifndef FUSELISTMODEL_H
#define FUSELISTMODEL_H

#include <QAbstractListModel>
#include <functional>

class FuseListModel : public QAbstractListModel
{
public:
    FuseListModel(QObject *parent);
    typedef std::function<void()> Function;
    void callFunction(const Function &func);
    bool event(QEvent *);
};

#endif // FUSELISTMODEL_H
