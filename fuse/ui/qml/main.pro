TARGET = fuse-qml
TEMPLATE = app

CONFIG += c++11

QT += qml quick gamepad multimedia svg

android: QT += androidextras

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

DEFINES += TIMER_ALWAYS_SLEEP

SOURCES += \
    ../../compat/unix/socket.c \
    ../../compat/unix/osname.c \
    ../../compat/unix/tuntap.c \
    ../../timer/native.c \
    ../../timer/timer.c \
    main.cpp \
    compat.cpp \
    fusescreen.cpp \
    fusetexture.cpp \
    keysyms.cpp \
    qmlui.cpp \
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
    fusesettings.cpp \
    folderlistmodel.cpp

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
    fusesettings.h \
    folderlistmodel.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

LIBS = $$PWD/../../libfuse.a $$PWD/../../../libspectrum/.libs/libspectrum.a -lz
!android: LIBS += -lxml2 -lpng -lbz2 -lasound -laudiofile

!android {
    QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_LFLAGS += -fsanitize=address
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/res/drawable/logo.png

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
