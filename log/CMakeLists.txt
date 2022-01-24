set(PLATFORM_SOURCES )
set(PLATFORM_COMPILE_DEFS )
if(APPLE)
    list(APPEND PLATFORM_SOURCES src/OsLogWriter.cpp)
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES src/OutputDebugStringWriter.cpp)
    list(APPEND PLATFORM_COMPILE_DEFS -D_WIN32_WINNT=${MIN_WINNT_VER})
endif()

add_library(log STATIC
    src/Log.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(log PUBLIC include)

target_compile_definitions(log PRIVATE -DLOG_LIBRARY=1 ${PLATFORM_COMPILE_DEFS})

set_target_properties(log
    PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)
