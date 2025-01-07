cmake_minimum_required(VERSION 3.25)

project(Spectacol
    VERSION 1.6.0.5
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

set(QT_REQUIRED_VERSION "6.7")

find_package(Qt6 ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Quick Multimedia ShaderTools)

qt_standard_project_setup(REQUIRES ${QT_REQUIRED_VERSION})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    QtGamepad
    GIT_REPOSITORY https://github.com/bog-dan-ro/qtgamepadlegacy.git
    GIT_TAG dev
)

FetchContent_MakeAvailable(QtGamepad)

macro(FetchAndBuildLibrary lib_name lib_repo lib_tag configure_flags)
    FetchContent_Declare(
        ${lib_name}_import
        GIT_REPOSITORY ${lib_repo}
        GIT_TAG ${lib_tag}
    )
    FetchContent_MakeAvailable(${lib_name}_import)

    set(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}/lib/pkgconfig)

    if (ANDROID)
        cmake_path(GET CMAKE_SYSROOT PARENT_PATH LLVM_ROOT)
        set(ENV{PATH} ${LLVM_ROOT}/bin:$ENV{PATH})
        set(ENV{AR} ${LLVM_ROOT}/bin/llvm-ar)
        set(ENV{AS} ${LLVM_ROOT}/bin/llvm-as)
        set(ENV{CC} ${LLVM_ROOT}/bin/${CMAKE_ANDROID_ARCH_TRIPLE}${ANDROID_PLATFORM_LEVEL}-clang)
        set(ENV{CXX} ${LLVM_ROOT}/bin/${CMAKE_ANDROID_ARCH_TRIPLE}${ANDROID_PLATFORM_LEVEL}-clang++)
        set(ENV{LD} ${LLVM_ROOT}/bin/ld)
        set(ENV{RANLIB} ${LLVM_ROOT}/bin/llvm-ranlib)
        set(ENV{STRIP} ${LLVM_ROOT}/bin/llvm-strip)
        separate_arguments(configure UNIX_COMMAND "./configure --host ${CMAKE_ANDROID_ARCH_TRIPLE} --with-sysroot=${CMAKE_SYSROOT}/usr --prefix=${CMAKE_BINARY_DIR} ${configure_flags}")
    else()
        separate_arguments(configure UNIX_COMMAND "./configure --prefix=${CMAKE_BINARY_DIR} ${configure_flags}")
    endif()

    execute_process(
        COMMAND ./autogen.sh
        WORKING_DIRECTORY ${${lib_name}_import_SOURCE_DIR}
    )
    execute_process(
        COMMAND ${configure}
        WORKING_DIRECTORY ${${lib_name}_import_SOURCE_DIR}
    )
    execute_process(
        COMMAND make -j16
        WORKING_DIRECTORY ${${lib_name}_import_SOURCE_DIR}
    )
    execute_process(
        COMMAND make install
        WORKING_DIRECTORY ${${lib_name}_import_SOURCE_DIR}
    )

    add_library(${lib_name} STATIC IMPORTED)
    add_dependencies(${lib_name} ${lib_name}_import)
    if(EXISTS ${CMAKE_BINARY_DIR}/lib/${lib_name}.a)
        set(lib_path ${CMAKE_BINARY_DIR}/lib/${lib_name}.a)
        set(headers_path ${CMAKE_BINARY_DIR}/include)
    else()
        set(lib_path ${${lib_name}_import_SOURCE_DIR}/lib${lib_name}.a)
        set(headers_path ${${lib_name}_import_SOURCE_DIR} )
    endif()
    set_target_properties(${lib_name}
        PROPERTIES
            IMPORTED_LOCATION ${lib_path}
            INTERFACE_INCLUDE_DIRECTORIES ${headers_path}
    )
endmacro()

FetchAndBuildLibrary(libspectrum "git://git.code.sf.net/u/bog-dan-ro/libspectrum" master "--disable-shared --with-fake-glib --without-bzip2 --without-libgcrypt --without-libaudiofile")
FetchAndBuildLibrary(fuse "git://git.code.sf.net/u/bog-dan-ro/fuse" master "--disable-shared --without-gtk --without-alsa --without-sdl --without-libxml2 --with-joystick --with-no-ui --without-png")

add_subdirectory(spectacol)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
