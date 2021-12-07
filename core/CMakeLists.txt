set(PLATFORM_SOURCES )
if(APPLE)
    list(APPEND PLATFORM_SOURCES
        mac/OsLogSink.cpp
        mac/OsLogSink.h
    )
endif()

add_library(core SHARED
    asiofwd.h
    common.h
    ConnectionPool.cpp
    ConnectionPool.h
    Dates.cpp
    Dates.h
    Errors.cpp
    Errors.h
    global.h
    Headers.cpp
    Headers.h
    HttpMessage.cpp
    HttpMessage.h
    HttpMessageParser.cpp
    HttpMessageParser.h
    Listenable.h
    Logging.cpp
    Logging.h
    ObjectPool.h
    Proxy.cpp
    Proxy.h
    ProxyFactory.cpp
    ProxyFactory.h
    ProxyTransaction.cpp
    ProxyTransaction.h
    Request.cpp
    Request.h
    Response.cpp
    Response.h
    Server.cpp
    Server.h
    Transaction.h
    ${PLATFORM_SOURCES}
)

set(PLATFORM_LIBS )
if(APPLE)
    list(APPEND PLATFORM_LIBS trusty_interface)
endif()

target_link_libraries(
    core
    Qt6::Core
    OpenSSL::SSL
    OpenSSL::Crypto
    asio::asio
    spdlog::spdlog
    date::date
    ${PLATFORM_LIBS}
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
