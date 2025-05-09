qt_add_executable(spectacol main.cpp)
qt_add_resources(spectacol "resources"
    PREFIX "/"
    FILES
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
        images/gamepad.png
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
        roms/disciple.rom
        roms/gluck.rom
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

        z80_opcodes.csv
)

qt_add_shaders(spectacol "spectacol_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /
    FILES
        shaders/mirror.frag
        shaders/mirror.vert
        shaders/simple.frag
        shaders/simple.vert
)

set_source_files_properties(
    qml/private/TextSizes.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(spectacol
    URI Spectacol
    VERSION 1.0
    SOURCES
        ${fuse_import_SOURCE_DIR}/compat/unix/socket.c
        ${fuse_import_SOURCE_DIR}/compat/unix/osname.c
        ${fuse_import_SOURCE_DIR}/compat/unix/tuntap.c
        ${fuse_import_SOURCE_DIR}/timer/native.c
        ${fuse_import_SOURCE_DIR}/timer/timer.c
        breakpointsmodel.cpp breakpointsmodel.h
        compat.cpp
        disassamblemodel.cpp disassamblemodel.h
        folderlistmodel.cpp folderlistmodel.h
        fuseemulator.cpp fuseemulator.h
        fuselistmodel.cpp fuselistmodel.h
        fuseobject.cpp fuseobject.h
        fuserecording.cpp fuserecording.h
        zxscreen.cpp zxscreen.h
        fusesettings.cpp fusesettings.h
        fusetape.cpp fusetape.h
        fusetapedata.cpp fusetapedata.h
        zximage.cpp zximage.h
        keysyms.cpp
        pokefindermodel.cpp pokefindermodel.h
        pokemodel.cpp pokemodel.h
        qmlui.cpp qmlui.h
        spectrumscreen.cpp spectrumscreen.h
        z80assembler.cpp z80assembler.h
        zxgamesmodel.cpp zxgamesmodel.h
        zxrequests.cpp zxrequests.h

    QML_FILES
        qml/AboutPage.qml
        qml/BreakpointsView.qml
        qml/BrowseTapePage.qml
        qml/CalibrateGamepad.qml
        qml/CoverFlow.qml
        qml/CoverFlowFiles.qml
        qml/CustomizeGamepadPage.qml
        qml/DebuggerPage.qml
        qml/DisassembleView.qml
        qml/FancyBaseList.qml
        qml/FancyList.qml
        qml/FancyPane.qml
        qml/FancyText.qml
        qml/FancyTextField.qml
        qml/FileBrowserPage.qml
        qml/FiltersPage.qml
        qml/FuseMenu.qml
        qml/GetFileBrowserPage.qml
        qml/GetListPage.qml
        qml/Joysticks.qml
        qml/MachineTypePage.qml
        qml/Main.qml
        qml/MenuView.qml
        qml/MessagePage.qml
        qml/OnScreen48Keyboard.qml
        qml/OnScreenButton.qml
        qml/OnScreenCursorJoystick.qml
        qml/OnScreenJoystick.qml
        qml/OnScreenJoystickButton.qml
        qml/OptionsGeneralPage.qml
        qml/OptionsPeripheralsPage.qml
        qml/OptionsScreenPage.qml
        qml/OptionsSoundPage.qml
        qml/PokeFinderPage.qml
        qml/PokeManagerPage.qml
        qml/RegistersView.qml
        qml/SaveFilePage.qml
        qml/SaveSnapshotPage.qml
        qml/SaveTapePage.qml
        qml/SearchOnlinePage.qml
        qml/StandardGamepad.qml
        qml/TapesFileBrowserPage.qml
        qml/Utils.js
        qml/ZxMenuItem.qml
        qml/private/TextSizes.qml
)

target_link_libraries(spectacol PRIVATE fuse libspectrum Qt::Quick Qt::Multimedia Qt::GamepadLegacy Qt::GuiPrivate Qt::ShaderTools z)
target_compile_definitions(spectacol PRIVATE TIMER_ALWAYS_SLEEP)
