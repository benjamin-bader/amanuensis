find_library(COCOA Cocoa REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(SECURITY Security REQUIRED)
find_library(SERVICE_MANAGEMENT ServiceManagement REQUIRED)
find_library(SYSTEM System REQUIRED)

add_library(trusty_interface STATIC
    src/MessageProcessor.cpp
    src/ProxyState.cpp
    src/Service.cpp
    src/TrustyCommon.cpp
    src/UnixSocket.cpp
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

set_target_properties(
    trusty_interface
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(BUILD_TESTS)
    add_test_case(trusty_interface bytes src/BytesTest.cpp)
    add_test_case(trusty_interface message_processor src/MessageProcessorTest.cpp)
    add_test_case(trusty_interface proxy_state src/ProxyStateTest.cpp)
endif()
