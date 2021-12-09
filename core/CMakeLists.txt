set(PLATFORM_SOURCES )
if(APPLE)
    list(APPEND PLATFORM_SOURCES )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES src/win/RegistryKey.cpp src/win/WindowsProxy.cpp)
endif()

set(SOURCES
    src/ConnectionPool.cpp
    src/Dates.cpp
    src/Errors.cpp
    src/Headers.cpp
    src/HttpMessage.cpp
    src/HttpMessageParser.cpp
    src/Proxy.cpp
    src/ProxyFactory.cpp
    src/ProxyTransaction.cpp
    src/Request.cpp
    src/Response.cpp
    src/Server.cpp
)

add_library(core SHARED
    ${SOURCES}
    ${PLATFORM_SOURCES}
)

# Ensure MOC runs on public headers
file(GLOB PUBLIC_HEADERS include/**/*.h)
target_sources(core PUBLIC ${PUBLIC_HEADERS})

target_link_libraries(
    core
    log
    Qt6::Core
    OpenSSL::SSL
    OpenSSL::Crypto
    asio::asio
    date::date
)

target_include_directories(core
    PUBLIC include
    PRIVATE src
)

target_compile_definitions(core PRIVATE -DCORE_LIBRARY=1)

if(BUILD_TESTS)
    add_test_case(core dates src/DatesTests.cpp)
    add_test_case(core http_message_parser src/HttpMessageParserTests.cpp)
    add_test_case(core request src/RequestTest.cpp)
endif()
