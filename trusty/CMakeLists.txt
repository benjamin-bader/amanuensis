find_library(SYSTEM_CONFIG SystemConfiguration REQUIRED)

add_library(trusty-lib
    STATIC
    ClientConnection.cpp
    ClientConnection.h
    Server.cpp
    Server.h
    TrustyService.cpp
    TrustyService.h
)

target_link_libraries(
    trusty-lib

    PUBLIC
    trusty_interface
    ${SYSTEM_CONFIG}
)

add_executable(trusty
    main.cpp
)

target_link_libraries(
    trusty
    trusty-lib
)

target_link_options(
    trusty
    PRIVATE
    LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/trusty-info.plist
    LINKER:-sectcreate,__TEXT,__launchd_plist,${CMAKE_CURRENT_SOURCE_DIR}/trusty-launchd.plist
)
