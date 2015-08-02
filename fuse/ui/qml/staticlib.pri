TEMPLATE = lib
CONFIG += staticlib

SRCS_PATH = $$PWD/../../$$TARGET
include(common.pri)

exists($$SRCS_PATH): {
    SOURCES+=$$SRCS_PATH/*.c
    HEADERS+=$$SRCS_PATH/*.h
}

