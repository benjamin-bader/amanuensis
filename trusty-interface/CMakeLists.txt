find_library(COCOA Cocoa REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(SECURITY Security REQUIRED)
find_library(SERVICE_MANAGEMENT ServiceManagement REQUIRED)
find_library(SYSTEM System REQUIRED)

add_library(trusty_interface STATIC
    src/MessageProcessor.cpp
    src/OsLogSink.cpp
    src/ProxyState.cpp
    src/Service.cpp
    src/TLog.cpp
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
    spdlog::spdlog
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
