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
