qt_add_executable(test_online
    test_online.cpp
    ../spectacol/zxgamesmodel.cpp ../spectacol/zxgamesmodel.h
    ../spectacol/zxrequests.cpp ../spectacol/zxrequests.h
)
target_include_directories(test_online PRIVATE ../spectacol)
target_link_libraries(test_online PRIVATE Qt6::Test Qt6::Network)
add_test(NAME online_catalogues COMMAND test_online)
