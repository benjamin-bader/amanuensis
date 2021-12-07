find_library(COCOA Cocoa REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(SECURITY Security REQUIRED)
find_library(SERVICE_MANAGEMENT ServiceManagement REQUIRED)
find_library(SYSTEM System REQUIRED)

add_library(trusty_interface STATIC
    Bytes.h
    CFRef.h
    MessageProcessor.cpp
    MessageProcessor.h
    OsLogSink.cpp
    OsLogSink.h
    ProxyState.cpp
    ProxyState.h
    Service.cpp
    Service.h
    TLog.cpp
    TLog.h
    TrustyCommon.cpp
    TrustyCommon.h
    UnixSocket.cpp
    UnixSocket.h
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

target_include_directories(
    trusty_interface
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
