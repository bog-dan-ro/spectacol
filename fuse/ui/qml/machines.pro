TARGET = machines

SRCS_PATH = $$PWD/../../$$TARGET

include(common.pri)

INCLUDEPATH += $$PWD/../../ $$PWD/../../../libspectrum

SOURCES+=$$SRCS_PATH/*.c
HEADERS+=$$SRCS_PATH/*.h
