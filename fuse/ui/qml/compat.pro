TARGET = compat

SRCS_PATH = $$PWD/../../$$TARGET

include(common.pri)

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

SOURCES += \
    $$SRCS_PATH/dirname.c \
    $$SRCS_PATH/getopt.c \
    $$SRCS_PATH/getopt1.c

