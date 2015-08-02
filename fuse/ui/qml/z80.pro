TARGET = z80

SRCS_PATH = $$PWD/../../$$TARGET

include(common.pri)

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

HEADERS += \
    $$SRCS_PATH/z80_checks.h \
    $$SRCS_PATH/z80_macros.h \
    $$SRCS_PATH/z80.h

SOURCES += \
    $$SRCS_PATH/z80_ops.c \
    $$SRCS_PATH/z80.c

