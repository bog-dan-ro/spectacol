TEMPLATE = subdirs
CONFIG += ordered

#!android: error("Use only on Android");

SUBDIRS += \
#    libspectrum.pro \
#    compat.pro \
#    debugger.pro \
#    machines.pro \
#    peripherals.pro \
#    pokefinder.pro \
#    z80.pro \
#    fuse.pro \
    main.pro
