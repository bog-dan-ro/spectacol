#ifndef FUSETAPE_H
#define FUSETAPE_H

#include "fuseobject.h"

class FuseTape : public FuseObject
{
    Q_OBJECT

public:
    explicit FuseTape(QObject *parent = 0);

public slots:
    void open(QString filePath);
    void togglePlay();
    void rewind();
    void clear();
    void write(QString filePath);

private:

};

#endif // FUSETAPE_H
