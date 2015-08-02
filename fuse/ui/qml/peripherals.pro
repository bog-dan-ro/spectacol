TARGET = peripherals

SRCS_PATH = $$PWD/../../$$TARGET

include(common.pri)

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

SOURCES+=$$SRCS_PATH/*.c
HEADERS+=$$SRCS_PATH/*.h

SOURCES+=$$SRCS_PATH/disk/*.c
HEADERS+=$$SRCS_PATH/disk/*.h

SOURCES+=$$SRCS_PATH/flash/*.c
HEADERS+=$$SRCS_PATH/flash/*.h

SOURCES+=$$SRCS_PATH/ide/*.c
HEADERS+=$$SRCS_PATH/ide/*.h

SOURCES+=$$SRCS_PATH/nic/*.c
HEADERS+=$$SRCS_PATH/nic/*.h
