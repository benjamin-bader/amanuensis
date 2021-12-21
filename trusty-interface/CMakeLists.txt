find_library(COCOA Cocoa REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(SECURITY Security REQUIRED)
find_library(SERVICE_MANAGEMENT ServiceManagement REQUIRED)
find_library(SYSTEM System REQUIRED)

add_library(trusty_interface STATIC
    src/CFLog.cpp
    src/Message.cpp
    src/ProxyState.cpp
    src/TrustyCommon.cpp
)

target_include_directories(trusty_interface
    PUBLIC include
    PRIVATE src
)

target_link_libraries(
    trusty_interface
    PUBLIC
    log
    ${COCOA}
    ${FOUNDATION}
    ${SECURITY}
    ${SERVICE_MANAGEMENT}
    ${SYSTEM}
)

target_compile_definitions(
    trusty_interface
    PRIVATE
    -DTRUSTY_INTERFACE_LIBRARY
    -DSPDLOG_ENABLE_SYSLOG
)

set_target_properties(trusty_interface
    PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

if(BUILD_TESTS)
    add_test_case(trusty_interface proxy_state src/ProxyStateTest.cpp)
endif()
