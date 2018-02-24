TEMPLATE = subdirs

LIBSPECTRUM_INSTALL_DIR =
FUSE_BUILD_DIR =
SCRIPTS_DIR = $$PWD/build

android {
    LIBSPECTRUM_INSTALL_DIR = $$OUT_PWD/$$ANDROID_ARCHITECTURE
    FUSE_BUILD_DIR = $$OUT_PWD/build/$$ANDROID_ARCHITECTURE
} else {
    LIBSPECTRUM_INSTALL_DIR = $$OUT_PWD/desktop
    FUSE_BUILD_DIR = $$OUT_PWD/build/desktop
}

!exists($$LIBSPECTRUM_INSTALL_DIR/lib/libspectrum.a) || !exists($$FUSE_BUILD_DIR/fuse/libfuse.a) {
    android : system($$SCRIPTS_DIR/android.sh -a $$ANDROID_ARCHITECTURE -p $$ANDROID_PLATFORM -n $$NDK_ROOT -o $$OUT_PWD)
    else: system($$SCRIPTS_DIR/desktop.sh -o $$OUT_PWD)
}

SUBDIRS += spectacol

OTHER_FILES += \
    build/android.sh \
    build/desktop.sh
