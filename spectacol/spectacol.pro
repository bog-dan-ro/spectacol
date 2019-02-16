TARGET = fuse-qml
TEMPLATE = app

CONFIG += c++11 #object_parallel_to_source

QT += qml quick gamepad multimedia svg

android: QT += androidextras

SCRIPTS_DIR = $$PWD/../build
android {
    LIBSPECTRUM_INSTALL_DIR = $$OUT_PWD/$$ANDROID_ARCHITECTURE
    FUSE_BUILD_DIR = $$OUT_PWD/build/$$ANDROID_ARCHITECTURE/fuse
    FUSE_BUILD_COMMAND = $$SCRIPTS_DIR/android.sh -a $$ANDROID_ARCHITECTURE -n $$NDK_ROOT -o $$OUT_PWD
} else {
    LIBSPECTRUM_INSTALL_DIR = $$OUT_PWD/desktop
    FUSE_BUILD_DIR = $$OUT_PWD/build/desktop/fuse
    FUSE_BUILD_COMMAND = $$SCRIPTS_DIR/desktop.sh -o $$OUT_PWD
}

INCLUDEPATH += $$FUSE_BUILD_DIR $$LIBSPECTRUM_INSTALL_DIR/include $$PWD/../fuse

DEFINES += TIMER_ALWAYS_SLEEP

SOURCES += \
    ../fuse/compat/unix/socket.c \
    ../fuse/compat/unix/osname.c \
    ../fuse/compat/unix/tuntap.c \
    ../fuse/timer/native.c \
    ../fuse/timer/timer.c \
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
    folderlistmodel.cpp \
    fusetape.cpp \
    pokemodel.cpp \
    fusetapedata.cpp \
    fuserecording.cpp

HEADERS += \
    $$FUSE_BUILD_DIR/config.h \
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
    folderlistmodel.h \
    fusetape.h \
    pokemodel.h \
    fusetapedata.h \
    fuserecording.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

LIBS = $$FUSE_BUILD_DIR/libfuse.a $$LIBSPECTRUM_INSTALL_DIR/lib/libspectrum.a -lz
!android: LIBS += -lbz2

buildFuseTarget.target = $$FUSE_BUILD_DIR/config.h
buildFuseTarget.commands = $$FUSE_BUILD_COMMAND

PRE_TARGETDEPS += $$FUSE_BUILD_DIR/config.h

QMAKE_EXTRA_TARGETS += buildFuseTarget

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
android: include($$PWD/3rdparty/android_openssl/openssl.pri)
