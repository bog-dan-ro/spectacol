cmake_minimum_required(VERSION 3.25)

project(Spectacol
    VERSION 1.6.0.5
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

set(QT_REQUIRED_VERSION "6.11")

include(FetchContent)

FetchContent_Declare(
    QtGamepad
    GIT_REPOSITORY https://github.com/bog-dan-ro/qtgamepadlegacy.git
    GIT_TAG e89b1e0dd903fc638857a344be0ebef0a04495ed
)

# QtGamepadLegacy is built with Qt's internal build system, so it must find Qt
# before we do: otherwise Qt's BuildInternals can't promote the 3rd party targets
# (e.g. Threads::Threads) we already imported to global, and Qt6QmlConfig skips
# loading Qt6QmlBuildInternals (qt_internal_add_qml_module).
FetchContent_MakeAvailable(QtGamepad)

if(ANDROID)
    # The fetched Qt module's JAR/plugins live here, rather than in the Qt SDK.
    # androiddeployqt searches these prefixes when resolving module metadata.
    list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_BINARY_DIR}")
endif()

find_package(Qt6 ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Quick Multimedia ShaderTools GuiPrivate)

qt_standard_project_setup(REQUIRES ${QT_REQUIRED_VERSION})

if(NOT TARGET ZX::Kit)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../include/zxkit/zxkit.h")
        FetchContent_Declare(
            ZxKit
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        )
    else()
        FetchContent_Declare(
            ZxKit
            GIT_REPOSITORY https://github.com/bog-dan-ro/ZxKit.git
            GIT_TAG master
        )
    endif()
    FetchContent_MakeAvailable(ZxKit)
endif()

add_subdirectory(src)

option(SPECTACOL_BUILD_TESTS "Build Spectacol's offline catalogue and input tests" OFF)
if(SPECTACOL_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
