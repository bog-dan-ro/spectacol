qt_add_executable(spectacol main.cpp)
target_compile_definitions(spectacol PRIVATE SPECTACOL_VERSION="${PROJECT_VERSION}")
if(ANDROID)
    target_sources(spectacol PRIVATE androidintegration.cpp androidintegration.h)
    # Preserve the highest version code used by the old per-ABI release scheme.
    set(SPECTACOL_ANDROID_VERSION_CODE 1641605 CACHE STRING "Android APK/AAB version code")
    set_target_properties(spectacol PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_VERSION_CODE "${SPECTACOL_ANDROID_VERSION_CODE}"
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml"
    )
    include(3rdparty/android_openssl/android_openssl.cmake)
    add_android_openssl_libraries(spectacol)
endif()
qt_add_resources(spectacol "resources"
    PREFIX "/"
    FILES
        android/res/drawable/icon.png
        images/arrow-down-left.svg
        images/arrow-down-right.svg
        images/arrow-down.svg
        images/arrow-left.svg
        images/arrow-right.svg
        images/arrow-up-left.svg
        images/arrow-up-right.svg
        images/arrow-up.svg
        images/binary.svg
        images/cassette-broken.svg
        images/cassette-fast.svg
        images/cassette-tape.svg
        images/cassette-unknown.svg
        images/cassette-wait.svg
        images/controller-icon.svg
        images/draw-fire.svg
        images/fastforward.svg
        images/floppy.svg
        images/folder.svg
        images/gamepad.svg
        images/keyboard-icon.svg
        images/pause.svg
        images/play.svg

        qtquickcontrols2.conf

        roms/128-0.rom
        roms/128-1.rom
        roms/128p-0.rom
        roms/128p-1.rom
        roms/256s-0.rom
        roms/256s-1.rom
        roms/256s-2.rom
        roms/256s-3.rom
        roms/48.rom
        roms/cobra1_boot.rom
        roms/cobra1_sys-0.rom
        roms/cobra1_sys-1.rom
        roms/cobra1_sys-2.rom
        roms/cobra1_sys-3.rom
        roms/cobra2_ts_boot.rom
        roms/disciple.rom
        roms/gluck.rom
        roms/hc-if1.rom
        roms/hc2000-basic.rom
        roms/hc2000-cpm.rom
        roms/if1-1.rom
        roms/if1-2.rom
        roms/mf1.rom
        roms/mf128.rom
        roms/mf3.rom
        roms/plus2-0.rom
        roms/plus2-1.rom
        roms/plus3-0.rom
        roms/plus3-1.rom
        roms/plus3-2.rom
        roms/plus3-3.rom
        roms/plus3e-0.rom
        roms/plus3e-1.rom
        roms/plus3e-2.rom
        roms/plus3e-3.rom
        roms/plusd.rom
        roms/se-0.rom
        roms/se-1.rom
        roms/speccyboot-1.4.rom
        roms/tc2048.rom
        roms/tc2068-0.rom
        roms/tc2068-1.rom
        roms/trdos.rom
)

qt_add_shaders(spectacol "spectacol_shaders"
    GLSL "100es,120,150,310es,450"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /
    FILES
        shaders/mirror.frag
        shaders/mirror.vert
)

set_source_files_properties(
    qml/private/TextSizes.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(spectacol
    URI Spectacol
    VERSION 1.0
    SOURCES
        audiobuffer.cpp audiobuffer.h
        audiooutput.cpp audiooutput.h
        breakpointsmodel.cpp breakpointsmodel.h
        controllerinput.cpp controllerinput.h
        controllerprofiles.cpp controllerprofiles.h
        debuggermodels.cpp debuggermodels.h
        debugsymbols.cpp debugsymbols.h
        disassamblemodel.cpp disassamblemodel.h
        disks.h disks.cpp
        folderlistmodel.cpp folderlistmodel.h
        nextsdcard.h nextsdcard.cpp
        nextuarthooks.cpp nextuarthooks.h
        pokefindermodel.cpp pokefindermodel.h
        pokemodel.cpp pokemodel.h
        qmlui.cpp qmlui.h
        romoverrides.cpp romoverrides.h
        screenfilters.h
        sdcardfiles.h sdcardfiles.cpp
        spectrumscreen.cpp spectrumscreen.h
        z80assembler.cpp z80assembler.h
        zxemulator.cpp zxemulator.h
        zxgamesmodel.cpp zxgamesmodel.h
        zximage.cpp zximage.h
        zxrecording.cpp zxrecording.h
        zxrequests.cpp zxrequests.h
        zxscreen.cpp zxscreen.h
        zxsettings.cpp zxsettings.h
        zxtape.cpp zxtape.h
        zxtapedata.cpp zxtapedata.h

    QML_FILES
        qml/AboutPage.qml
        qml/GamepadDiagram.qml
        qml/BreakpointsView.qml
        qml/BrowseTapePage.qml
        qml/CalibrateGamepad.qml
        qml/CoverFlow.qml
        qml/CoverFlowFiles.qml
        qml/CustomizeGamepadPage.qml
        qml/ControllerProfilesPage.qml
        qml/OptionsControlsPage.qml
        qml/DebuggerPage.qml
        qml/DebuggerGamepad.qml
        qml/FormDialog.qml
        qml/HostPathDialog.qml
        qml/RomsPage.qml
        qml/DebugSymbolsView.qml
        qml/CompactFilesView.qml
        qml/NavigationComboBox.qml
        qml/DebuggerMemoryView.qml
        qml/DebuggerStateView.qml
        qml/DisassembleView.qml
        qml/DisksEjectPage.qml
        qml/DisksInsertPage.qml
        qml/DrivesList.qml
        qml/FancyBaseList.qml
        qml/FancyList.qml
        qml/FancyPane.qml
        qml/FancyText.qml
        qml/FancyTextField.qml
        qml/FileBrowserPage.qml
        qml/FiltersPage.qml
        qml/ZxMenu.qml
        qml/Joysticks.qml
        qml/MachineTypePage.qml
        qml/Main.qml
        qml/MenuView.qml
        qml/MessagePage.qml
        qml/NextSdCardPage.qml
        qml/NextUartPage.qml
        qml/FilePickerPage.qml
        qml/SdCardPage.qml
        qml/KeyboardInput.qml
        qml/OnScreen48Keyboard.qml
        qml/OnScreenButton.qml
        qml/OnScreenCursorJoystick.qml
        qml/OnScreenJoystick.qml
        qml/OnScreenJoystickButton.qml
        qml/OptionsGeneralPage.qml
        qml/OptionsPeripheralsPage.qml
        qml/OptionsScreenPage.qml
        qml/OptionsBordersPage.qml
        qml/OptionsSoundPage.qml
        qml/PokeFinderPage.qml
        qml/PokeManagerPage.qml
        qml/RegistersView.qml
        qml/SaveFilePage.qml
        qml/RzxControlsPage.qml
        qml/SaveSnapshotPage.qml
        qml/SaveTapePage.qml
        qml/NewTapePage.qml
        qml/SearchOnlinePage.qml
        qml/StandardGamepad.qml
        qml/TapesFileBrowserPage.qml
        qml/Theme.qml
        qml/HintBar.qml
        qml/PageActions.qml
        qml/Hud.qml
        qml/Sheet.qml
        qml/SectionTitle.qml
        qml/SettingRow.qml
        qml/SettingsList.qml
        qml/OptionList.qml
        qml/DesktopMenuBar.qml
        qml/Utils.js
        qml/ZxMenuItem.qml
        qml/private/TextSizes.qml

)

target_link_libraries(spectacol PRIVATE
    ZX::VideoFilters
    Qt::Quick
    Qt::Multimedia
    Qt::GamepadLegacy
    Qt::GuiPrivate
    Qt::ShaderTools)

qt_add_resources(spectacol "uart_scripts"
    PREFIX "/uart"
    BASE "uart"
    FILES uart/esp.js uart/pi.js
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(spectacolAppId "eu.licentia.games.spectacol")
    set_target_properties(spectacol PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    install(TARGETS spectacol
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    )

    # FetchContent's Qt module also has SDK install rules. The Runtime component
    # selects only the libraries, QML module and controller backends we ship.
    install(TARGETS GamepadLegacy GamepadLegacyQuickPrivate zxkit zxkit_video_filters
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    )
    install(TARGETS GamepadLegacyQuickPrivateplugin
        LIBRARY DESTINATION qml/QtGamepadLegacy COMPONENT Runtime
    )
    get_target_property(gamepadQmlDir GamepadLegacyQuickPrivate QT_QML_MODULE_OUTPUT_DIRECTORY)
    install(FILES "${gamepadQmlDir}/qmldir"
        DESTINATION qml/QtGamepadLegacy COMPONENT Runtime
    )
    foreach(backend IN ITEMS QEvdevGamepadBackendPlugin QSdl2GamepadBackendPlugin)
        if(TARGET ${backend})
            install(TARGETS ${backend}
                LIBRARY DESTINATION plugins/legacygamepads COMPONENT Runtime
            )
        endif()
    endforeach()

    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/${spectacolAppId}.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications COMPONENT Runtime
    )
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/${spectacolAppId}.metainfo.xml"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo COMPONENT Runtime
    )
    install(FILES android/res/drawable/icon.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/192x192/apps
        RENAME "${spectacolAppId}.png" COMPONENT Runtime
    )
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/${spectacolAppId}.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps COMPONENT Runtime
    )
    install(FILES roms/README.copyright
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol
        RENAME ROMs.txt COMPONENT Runtime
    )
    get_target_property(zxkitSourceDir ZX::Kit SOURCE_DIR)
    install(FILES "${zxkitSourceDir}/LICENSE"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol
        RENAME COPYING COMPONENT Runtime
    )
    install(FILES "${qtgamepad_SOURCE_DIR}/LICENSE.LGPLv3"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol
        RENAME QtGamepadLegacy.LGPLv3 COMPONENT Runtime
    )
    install(FILES "${zxkitSourceDir}/src/video/filters/README.md"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol
        RENAME VideoFilters.md COMPONENT Runtime
    )
    install(FILES "${zxkitSourceDir}/src/video/filters/xbrz/License.txt"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol
        RENAME xBRZ.txt COMPONENT Runtime
    )
    install(FILES "${zxkitSourceDir}/LICENSES/ayumi.txt"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spectacol COMPONENT Runtime
    )
endif()
