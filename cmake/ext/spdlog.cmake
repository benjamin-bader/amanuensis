include(ExternalProject)

set(spdlog_URL https://github.com/gabime/spdlog/archive/refs/tags/v1.9.2.tar.gz)
set(spdlog_URL_MD5 cee7f3d31178a00791d7a22c6738df6d)
set(spdlog_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/spdlog_ext/src/spdlog_ext-build)
set(spdlog_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/spdlog_ext/src/spdlog_ext/include)

if(WIN32)
    message(ERROR "Not yet supported")
    set(spdlog_STATIC_LIBS )
else()
    set(spdlog_STATIC_LIBS ${spdlog_BUILD_DIR}/libspdlog.a)
endif()

ExternalProject_add(
    spdlog_ext
    PREFIX spdlog_ext
    URL ${spdlog_URL}
    URL_MD5 ${spdlog_URL_MD5}
    BUILD_BYPRODUCTS ${spdlog_STATIC_LIBS}
    INSTALL_COMMAND ""
    CMAKE_CACHE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
        -DBULID_TESTS:BOOL=OFF
        -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DSPDLOG_INSTALL:BOOL=OFF
)

file(MAKE_DIRECTORY ${spdlog_INCLUDE_DIR})

add_library(spdlog::spdlog STATIC IMPORTED)
add_dependencies(spdlog::spdlog spdlog_ext)

set_target_properties(spdlog::spdlog
    PROPERTIES
    IMPORTED_LOCATION ${spdlog_STATIC_LIBS}
    INTERFACE_INCLUDE_DIRECTORIES ${spdlog_INCLUDE_DIR}
)
