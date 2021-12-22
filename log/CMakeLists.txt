set(PLATFORM_SOURCES )
if(APPLE)
    list(APPEND PLATFORM_SOURCES src/OsLogWriter.cpp)
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES src/OutputDebugStringWriter.cpp)
endif()

add_library(log STATIC
    src/Log.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(log PUBLIC include)

target_compile_definitions(log PRIVATE -DLOG_LIBRARY=1)

set_target_properties(log
    PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)
