cmake_minimum_required(VERSION 3.18)

option(BUILD_TESTS "Enable unit tests" ON)
option(STATIC_LINKAGE "Build a static corelib instead of a shared corelib" OFF)
mark_as_advanced(STATIC_LINKAGE)

project(amanuensis
        VERSION 0.1.0
        DESCRIPTION "A trusty scrib"
        LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_TESTS)
    enable_testing()
    include(AddTest)
endif()

set(QT_COMPONENTS
    Core
    Core5Compat
    Gui
    LinguistTools
    Network
    Test
    Widgets
)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

find_package(Qt6 COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(Threads)

include(ext/asio)
include(ext/date)
include(ext/spdlog)

if(APPLE)
    add_subdirectory(trusty-interface)
    add_subdirectory(trusty)
    add_subdirectory(trusty-test)
endif()

add_subdirectory(core)
add_subdirectory(core-test)
add_subdirectory(app)

