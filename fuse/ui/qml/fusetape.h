#ifndef FUSETAPE_H
#define FUSETAPE_H

#include "fuseobject.h"

class FuseTape : public FuseObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasTape MEMBER m_hasTape NOTIFY hasTapeChanged)

public:
    explicit FuseTape(QObject *parent = 0);
    void updateBrowseData();

public slots:
    void open(QString filePath);
    void togglePlay();
    void rewind();
    void clear();
    void write(QString filePath);

signals:
    void hasTapeChanged(bool hasTape);

private:
    bool m_hasTape = false;
};

#endif // FUSETAPE_H
