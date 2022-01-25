set(PLATFORM_SOURCES )
set(PLATFORM_COMPILE_DEFS )
if(APPLE)
    list(APPEND PLATFORM_SOURCES )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES )
    list(APPEND PLATFORM_COMPILE_DEFS -D_WIN32_WINNT=${MIN_WINNT_VER})
endif()

set(SOURCES
    src/AsioConnection.cpp
    src/ConnectionPool.cpp
    src/Errors.cpp
    src/Headers.cpp
    src/HttpMessage.cpp
    src/HttpMessageParser.cpp
    src/Proxy.cpp
    src/Request.cpp
    src/Response.cpp
    src/Server.cpp
    src/Transaction.cpp
)

add_library(core STATIC
    ${SOURCES}
    ${PLATFORM_SOURCES}
)

# Ensure MOC runs on public headers
file(GLOB PUBLIC_HEADERS include/**/*.h)
target_sources(core PRIVATE ${PUBLIC_HEADERS})

target_link_libraries(
    core
    log
    Qt6::Core
    OpenSSL::SSL
    OpenSSL::Crypto
    asio::asio
)

target_include_directories(core
    PUBLIC include
    PRIVATE src
)

target_compile_definitions(core
    PRIVATE
    -DCORE_LIBRARY
    ${PLATFORM_COMPILE_DEFS}
)

#set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(BUILD_TESTS)
    add_test_case(core headers src/HeadersTests.cpp)
    add_test_case(core http_message_parser src/HttpMessageParserTests.cpp)
    add_test_case(core request src/RequestTest.cpp)
endif()
