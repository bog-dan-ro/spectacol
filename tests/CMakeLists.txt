qt_add_executable(test_online
    test_online.cpp
    ../src/zxgamesmodel.cpp ../src/zxgamesmodel.h
    ../src/zxrequests.cpp ../src/zxrequests.h
)
target_include_directories(test_online PRIVATE ../src)
target_link_libraries(test_online PRIVATE Qt6::Test Qt6::Network)
add_test(NAME online_catalogues COMMAND test_online)

qt_add_executable(test_controller_input
    test_controller_input.cpp
    ../src/controllerinput.cpp ../src/controllerinput.h
)
target_include_directories(test_controller_input PRIVATE ../src)
target_link_libraries(test_controller_input PRIVATE Qt6::Test)
add_test(NAME controller_input COMMAND test_controller_input)

qt_add_executable(test_keyboard_input test_keyboard_input.cpp)
set_source_files_properties(../src/qml/KeyboardInput.qml PROPERTIES QT_RESOURCE_ALIAS KeyboardInput.qml)
qt_add_qml_module(test_keyboard_input
    URI KeyboardInputTest
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KeyboardInputTest
    QML_FILES ../src/qml/KeyboardInput.qml
)
target_link_libraries(test_keyboard_input PRIVATE Qt6::Test Qt6::Qml)
add_test(NAME keyboard_input COMMAND test_keyboard_input)

if(NOT ANDROID AND NOT IOS)
    qt_add_executable(test_desktop_input
        test_desktop_input.cpp
        ../src/desktopinput.cpp ../src/desktopinput.h
    )
    target_include_directories(test_desktop_input PRIVATE ../src)
    target_link_libraries(test_desktop_input PRIVATE
        Qt6::Test Qt6::Quick Qt6::GuiPrivate Qt::GamepadLegacy
    )
    qt_add_qml_module(test_desktop_input
        URI DesktopInputTest
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DesktopInputTest
        QML_FILES DesktopInputWindow.qml
    )
    add_test(NAME desktop_input COMMAND test_desktop_input)
    set_tests_properties(desktop_input PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen"
    )
endif()

qt_add_executable(test_next_uart test_next_uart.cpp ../src/nextuarthooks.cpp ../src/nextuarthooks.h)
target_include_directories(test_next_uart PRIVATE ../src)
target_link_libraries(test_next_uart PRIVATE Qt6::Test Qt6::Qml Qt6::Network ZX::Kit)
qt_add_resources(test_next_uart "uart_scripts_test"
    PREFIX "/uart"
    BASE "../src/uart"
    FILES ../src/uart/esp.js ../src/uart/pi.js
)
add_test(NAME next_uart_hooks COMMAND test_next_uart)

qt_add_executable(test_next_sdcard test_next_sdcard.cpp ../src/nextsdcard.cpp ../src/nextsdcard.h)
target_include_directories(test_next_sdcard PRIVATE ../src)
target_link_libraries(test_next_sdcard PRIVATE Qt6::Test Qt6::Qml Qt6::Network ZX::Kit)
add_test(NAME next_sdcard_update COMMAND test_next_sdcard)

qt_add_executable(test_audio_output
    test_audio_output.cpp
    ../src/audiobuffer.cpp ../src/audiobuffer.h
    ../src/audiooutput.cpp ../src/audiooutput.h
)
target_include_directories(test_audio_output PRIVATE ../src)
target_link_libraries(test_audio_output PRIVATE Qt6::Test Qt6::Multimedia)
add_test(NAME audio_output COMMAND test_audio_output)

qt_add_executable(test_controller_profiles
    test_controller_profiles.cpp
    ../src/controllerprofiles.cpp ../src/controllerprofiles.h
    ../src/controllerinput.cpp ../src/controllerinput.h
)
target_include_directories(test_controller_profiles PRIVATE ../src)
target_link_libraries(test_controller_profiles PRIVATE Qt6::Test Qt6::Qml Qt::GamepadLegacy)
add_test(NAME controller_profiles COMMAND test_controller_profiles)

qt_add_executable(test_debugger_models
    test_debugger_models.cpp
    ../src/debuggermodels.cpp ../src/debuggermodels.h
)
target_include_directories(test_debugger_models PRIVATE ../src)
target_link_libraries(test_debugger_models PRIVATE Qt6::Test Qt6::Qml ZX::Kit)
add_test(NAME debugger_models COMMAND test_debugger_models)

qt_add_executable(test_zximage
    test_zximage.cpp
    ../src/zximage.cpp ../src/zximage.h
    ../src/screenfilterinput.cpp ../src/screenfilterinput.h
)
target_include_directories(test_zximage PRIVATE ../src)
target_link_libraries(test_zximage PRIVATE Qt6::Test Qt6::Quick Qt6::GuiPrivate ZX::Kit)
add_test(NAME image_updates COMMAND test_zximage)

qt_add_executable(test_files_features
    test_files_features.cpp
    ../src/pokemodel.cpp ../src/pokemodel.h
    ../src/romoverrides.cpp ../src/romoverrides.h
    ../src/debugsymbols.cpp ../src/debugsymbols.h
    ../src/folderlistmodel.cpp ../src/folderlistmodel.h
)
target_include_directories(test_files_features PRIVATE ../src)
target_link_libraries(test_files_features PRIVATE Qt6::Test Qt6::Qml ZX::Kit)
qt_add_resources(test_files_features "feature_roms"
    PREFIX "/roms"
    BASE "../src/roms"
    FILES ../src/roms/48.rom ../src/roms/128-0.rom ../src/roms/128-1.rom
)
add_test(NAME files_features COMMAND test_files_features)

qt_add_executable(test_media_icons
    test_media_icons.cpp
    ../src/spectrumscreen.cpp ../src/spectrumscreen.h
    ../src/qmlui.cpp ../src/qmlui.h
)
target_include_directories(test_media_icons PRIVATE ../src)
target_link_libraries(test_media_icons PRIVATE Qt6::Test Qt6::Quick ZX::Kit)
qt_add_resources(test_media_icons "preview_icons"
    PREFIX "/"
    BASE "../src"
    FILES
        ../src/images/binary.svg
        ../src/images/cassette-broken.svg
        ../src/images/cassette-unknown.svg
        ../src/images/cassette-wait.svg
        ../src/images/floppy.svg
        ../src/images/folder.svg
)
add_test(NAME media_icons COMMAND test_media_icons)
set_tests_properties(media_icons PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
