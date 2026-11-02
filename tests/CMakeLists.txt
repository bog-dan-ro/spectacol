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
qt_add_resources(test_keyboard_input "keyboard_input"
    PREFIX "/"
    BASE "../spectacol/qml"
    FILES ../spectacol/qml/KeyboardInput.qml
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
