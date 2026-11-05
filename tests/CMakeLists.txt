qt_add_executable(test_online
    test_online.cpp
    ../spectacol/zxgamesmodel.cpp ../spectacol/zxgamesmodel.h
    ../spectacol/zxrequests.cpp ../spectacol/zxrequests.h
)
target_include_directories(test_online PRIVATE ../spectacol)
target_link_libraries(test_online PRIVATE Qt6::Test Qt6::Network)
add_test(NAME online_catalogues COMMAND test_online)

qt_add_executable(test_controller_input
    test_controller_input.cpp
    ../spectacol/controllerinput.cpp ../spectacol/controllerinput.h
)
target_include_directories(test_controller_input PRIVATE ../spectacol)
target_link_libraries(test_controller_input PRIVATE Qt6::Test)
add_test(NAME controller_input COMMAND test_controller_input)

qt_add_executable(test_keyboard_input test_keyboard_input.cpp)
set_source_files_properties(../spectacol/qml/KeyboardInput.qml PROPERTIES QT_RESOURCE_ALIAS KeyboardInput.qml)
qt_add_qml_module(test_keyboard_input
    URI KeyboardInputTest
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KeyboardInputTest
    QML_FILES ../spectacol/qml/KeyboardInput.qml
)
target_link_libraries(test_keyboard_input PRIVATE Qt6::Test Qt6::Qml)
add_test(NAME keyboard_input COMMAND test_keyboard_input)

qt_add_executable(test_next_uart test_next_uart.cpp ../spectacol/nextuarthooks.cpp ../spectacol/nextuarthooks.h)
target_include_directories(test_next_uart PRIVATE ../spectacol)
target_link_libraries(test_next_uart PRIVATE Qt6::Test Qt6::Qml Qt6::Network ZX::Kit)
qt_add_resources(test_next_uart "uart_scripts_test"
    PREFIX "/uart"
    BASE "../spectacol/uart"
    FILES ../spectacol/uart/esp.js ../spectacol/uart/pi.js
)
add_test(NAME next_uart_hooks COMMAND test_next_uart)

qt_add_executable(test_next_sdcard test_next_sdcard.cpp ../spectacol/nextsdcard.cpp ../spectacol/nextsdcard.h)
target_include_directories(test_next_sdcard PRIVATE ../spectacol)
target_link_libraries(test_next_sdcard PRIVATE Qt6::Test Qt6::Qml Qt6::Network ZX::Kit)
add_test(NAME next_sdcard_update COMMAND test_next_sdcard)

qt_add_executable(test_audio_output
    test_audio_output.cpp
    ../spectacol/audiobuffer.cpp ../spectacol/audiobuffer.h
    ../spectacol/audiooutput.cpp ../spectacol/audiooutput.h
)
target_include_directories(test_audio_output PRIVATE ../spectacol)
target_link_libraries(test_audio_output PRIVATE Qt6::Test Qt6::Multimedia)
add_test(NAME audio_output COMMAND test_audio_output)

qt_add_executable(test_controller_profiles
    test_controller_profiles.cpp
    ../spectacol/controllerprofiles.cpp ../spectacol/controllerprofiles.h
    ../spectacol/controllerinput.cpp ../spectacol/controllerinput.h
)
target_include_directories(test_controller_profiles PRIVATE ../spectacol)
target_link_libraries(test_controller_profiles PRIVATE Qt6::Test Qt6::Qml Qt::GamepadLegacy)
add_test(NAME controller_profiles COMMAND test_controller_profiles)

qt_add_executable(test_debugger_models
    test_debugger_models.cpp
    ../spectacol/debuggermodels.cpp ../spectacol/debuggermodels.h
)
target_include_directories(test_debugger_models PRIVATE ../spectacol)
target_link_libraries(test_debugger_models PRIVATE Qt6::Test Qt6::Qml ZX::Kit)
add_test(NAME debugger_models COMMAND test_debugger_models)

qt_add_executable(test_zximage test_zximage.cpp ../spectacol/zximage.cpp ../spectacol/zximage.h)
target_include_directories(test_zximage PRIVATE ../spectacol)
target_link_libraries(test_zximage PRIVATE Qt6::Test Qt6::Quick Qt6::GuiPrivate ZX::Kit)
add_test(NAME image_updates COMMAND test_zximage)

qt_add_executable(test_files_features
    test_files_features.cpp
    ../spectacol/pokemodel.cpp ../spectacol/pokemodel.h
    ../spectacol/romoverrides.cpp ../spectacol/romoverrides.h
    ../spectacol/debugsymbols.cpp ../spectacol/debugsymbols.h
    ../spectacol/folderlistmodel.cpp ../spectacol/folderlistmodel.h
)
target_include_directories(test_files_features PRIVATE ../spectacol)
target_link_libraries(test_files_features PRIVATE Qt6::Test Qt6::Qml ZX::Kit)
qt_add_resources(test_files_features "feature_roms"
    PREFIX "/roms"
    BASE "../spectacol/roms"
    FILES ../spectacol/roms/48.rom ../spectacol/roms/128-0.rom ../spectacol/roms/128-1.rom
)
add_test(NAME files_features COMMAND test_files_features)

qt_add_executable(test_media_icons
    test_media_icons.cpp
    ../spectacol/spectrumscreen.cpp ../spectacol/spectrumscreen.h
    ../spectacol/qmlui.cpp ../spectacol/qmlui.h
)
target_include_directories(test_media_icons PRIVATE ../spectacol)
target_link_libraries(test_media_icons PRIVATE Qt6::Test Qt6::Quick ZX::Kit)
qt_add_resources(test_media_icons "preview_icons"
    PREFIX "/"
    BASE "../spectacol"
    FILES
        ../spectacol/images/binary.svg
        ../spectacol/images/cassette-broken.svg
        ../spectacol/images/cassette-unknown.svg
        ../spectacol/images/cassette-wait.svg
        ../spectacol/images/floppy.svg
        ../spectacol/images/folder.svg
)
add_test(NAME media_icons COMMAND test_media_icons)
set_tests_properties(media_icons PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
