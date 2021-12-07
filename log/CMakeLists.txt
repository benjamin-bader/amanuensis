set(PLATFORM_SOURCES )
if(APPLE)
    list(APPEND PLATFORM_SOURCES src/OsLogWriter.cpp)
endif()

add_library(log STATIC
    src/Log.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(log PUBLIC include)

target_compile_definitions(log PRIVATE -DLOG_LIBRARY=1)
