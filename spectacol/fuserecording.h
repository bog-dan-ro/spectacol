#ifndef FUSERECORDING_H
#define FUSERECORDING_H

#include "fuseobject.h"

class FuseEmulator;
class FuseRecording : public FuseObject
{
    Q_OBJECT
    Q_PROPERTY(Type type MEMBER m_type NOTIFY typeChanged)

public:
    enum Type {
        None,
        Playback,
        Recording
    };
    Q_ENUMS(Type)

public:
    FuseRecording(FuseEmulator *parent = nullptr);
    void setStopVisible(bool visible);

public slots:
    void start();
    void continueRecording();
    void rollback();
    void rollbackTo();
    void finalise();
    void stop();

signals:
    void typeChanged(Type type);

private:
    FuseEmulator *m_emulator;
    Type m_type = None;
};

#endif // FUSERECORDING_H
