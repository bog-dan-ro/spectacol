TARGET = fuse-qml
TEMPLATE = app

CONFIG += c++11

QT += qml quick

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

DEFINES+=UI_WIN32

SOURCES += main.cpp \
    compat.cpp \
    ../../compat/unix/socket.c \
    fusescreen.cpp \
    fusetexture.cpp \
    ../../compat/unix/osname.c \
    keysyms.cpp \
    joystick.cpp \
    qmlui.cpp \
    ../../timer/native.c \
    ../../sound/alsasound.c \
    ../../timer/timer.c \
    ../../compat/unix/tuntap.c \
    spectrumscreen.cpp

HEADERS += \
    fusetexture.h \
    fusescreen.h \
    qmlui.h \
    spectrumscreen.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

LIBS = $$PWD/../../libfuse.a $$PWD/../../../install/lib/libspectrum.a -lxml2 -lz -lpng -lbz2 -lasound
