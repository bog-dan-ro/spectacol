TARGET = fuse-ui

SRCS_PATH = $$PWD/../../

include(common.pri)

INCLUDEPATH += $$PWD/../../../libspectrum

DEFINES+=UI_WIN32

SOURCES+=$$SRCS_PATH/*.c
HEADERS+=$$SRCS_PATH/*.h
