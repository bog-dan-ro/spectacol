TARGET = fuse-qml
TEMPLATE = app

CONFIG += c++11

QT += qml quick gamepad
!no_desktop: QT += widgets

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

DEFINES+=UI_WIN32

SOURCES += main.cpp \
    compat.cpp \
    ../../compat/unix/socket.c \
    fusescreen.cpp \
    fusetexture.cpp \
    ../../compat/unix/osname.c \
    keysyms.cpp \
    qmlui.cpp \
    ../../timer/native.c \
    ../../sound/alsasound.c \
    ../../timer/timer.c \
    ../../compat/unix/tuntap.c \
    spectrumscreen.cpp \
    disassamblemodel.cpp \
    breakpointsmodel.cpp \
    fuseemulator.cpp \
    fuselistmodel.cpp \
    fuseobject.cpp \
    pokefindermodel.cpp \
    z80assembler.cpp

HEADERS += \
    fusetexture.h \
    fusescreen.h \
    qmlui.h \
    spectrumscreen.h \
    disassamblemodel.h \
    breakpointsmodel.h \
    fuseemulator.h \
    fuselistmodel.h \
    fuseobject.h \
    pokefindermodel.h \
    z80assembler.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

LIBS = $$PWD/../../libfuse.a $$PWD/../../../install/lib/libspectrum.a -lxml2 -lz -lpng -lbz2 -lasound -laudiofile

QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
QMAKE_LFLAGS += -fsanitize=address
