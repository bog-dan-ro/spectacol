TARGET = libspectrum

SRCS_PATH = $$PWD/../../../$$TARGET

include(common.pri)

HEADERS += \
    $$SRCS_PATH/config.h \
    $$SRCS_PATH/internals.h \
    $$SRCS_PATH/libspectrum.h \
    $$SRCS_PATH/tape_block.h

SOURCES += \
    $$SRCS_PATH/bzip2.c \
    $$SRCS_PATH/creator.c \
    $$SRCS_PATH/crypto.c \
    $$SRCS_PATH/csw.c \
    $$SRCS_PATH/dck.c \
    $$SRCS_PATH/ide.c \
    $$SRCS_PATH/libspectrum.c \
    $$SRCS_PATH/make-perl.c \
    $$SRCS_PATH/memory.c \
    $$SRCS_PATH/microdrive.c \
    $$SRCS_PATH/plusd.c \
    $$SRCS_PATH/pzx_read.c \
    $$SRCS_PATH/rzx.c \
    $$SRCS_PATH/sna.c \
    $$SRCS_PATH/snap_accessors.c \
    $$SRCS_PATH/snapshot.c \
    $$SRCS_PATH/snp.c \
    $$SRCS_PATH/sp.c \
    $$SRCS_PATH/symbol_table.c \
    $$SRCS_PATH/szx.c \
    $$SRCS_PATH/tap.c \
    $$SRCS_PATH/tape_accessors.c \
    $$SRCS_PATH/tape_block.c \
    $$SRCS_PATH/tape_set.c \
    $$SRCS_PATH/tape.c \
    $$SRCS_PATH/timings.c \
    $$SRCS_PATH/tzx_read.c \
    $$SRCS_PATH/tzx_write.c \
    $$SRCS_PATH/utilities.c \
    $$SRCS_PATH/warajevo_read.c \
    $$SRCS_PATH/wav.c \
    $$SRCS_PATH/z80.c \
    $$SRCS_PATH/z80em.c \
    $$SRCS_PATH/zlib.c \
    $$SRCS_PATH/zxs.c \
    $$SRCS_PATH/myglib/garray.c \
    $$SRCS_PATH/myglib/ghash.c \
    $$SRCS_PATH/myglib/gslist.c
