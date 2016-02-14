TARGET = fuse-qml
TEMPLATE = app

CONFIG += c++11

QT += qml quick gamepad multimedia

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

SOURCES += main.cpp \
    compat.cpp \
    ../../compat/unix/socket.c \
    fusescreen.cpp \
    fusetexture.cpp \
    ../../compat/unix/osname.c \
    keysyms.cpp \
    qmlui.cpp \
    ../../timer/native.c \
    ../../timer/timer.c \
    ../../compat/unix/tuntap.c \
    spectrumscreen.cpp \
    disassamblemodel.cpp \
    breakpointsmodel.cpp \
    fuseemulator.cpp \
    fuselistmodel.cpp \
    fuseobject.cpp \
    pokefindermodel.cpp \
    z80assembler.cpp \
    zxgamesmodel.cpp \
    zxrequests.cpp \
    xbrz.cpp

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
    z80assembler.h \
    zxgamesmodel.h \
    zxrequests.h \
    xbrz.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

LIBS = $$PWD/../../libfuse.a $$PWD/../../../libspectrum/.libs/libspectrum.a -lz
!android: LIBS += -lxml2 -lpng -lbz2 -lasound -laudiofile

QMAKE_CFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
QMAKE_CXXFLAGS += -fopenmp

!android {
    QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_LFLAGS += -fsanitize=address
}
