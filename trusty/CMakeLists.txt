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
    log
    ${SYSTEM_CONFIG}
)

add_executable(trusty
    main.cpp
)

target_link_libraries(
    trusty
    trusty-lib
)

set_target_properties(
    trusty
    PROPERTIES
    OUTPUT_NAME ${MACOSX_HELPER_IDENTIFIER}
)

find_program(PLIST_BUDDY
    NAME PlistBuddy
    HINTS /usr/libexec
    REQUIRED
)

add_custom_command(
    TARGET trusty
    PRE_BUILD
    COMMAND cp ARGS -f "${CMAKE_CURRENT_SOURCE_DIR}/trusty-info.plist" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND cp ARGS -f "${CMAKE_CURRENT_SOURCE_DIR}/trusty-launchd.plist" "${CMAKE_CURRENT_BINARY_DIR}/"
)

add_custom_command(
    TARGET trusty
    PRE_LINK
    COMMAND "${PLIST_BUDDY}" -c "Set :SMAuthorizedClients:0 'identifier \\\"${MACOSX_APP_IDENTIFIER}\\\" and certificate leaf = H\\\"${MACOSX_SIGNING_CERT_SHA1}\\\"'" "${CMAKE_CURRENT_BINARY_DIR}/trusty-info.plist"
)

target_link_options(
    trusty
    PRIVATE
    LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_BINARY_DIR}/trusty-info.plist
    LINKER:-sectcreate,__TEXT,__launchd_plist,${CMAKE_CURRENT_BINARY_DIR}/trusty-launchd.plist
)

find_program(CODESIGN codesign REQUIRED)

add_custom_command(
    TARGET trusty
    POST_BUILD
    COMMAND "${CODESIGN}"
    ARGS --force --sign "${MACOSX_SIGNING_CERT_SHA1}" -r='designated => identifier \"${MACOSX_HELPER_IDENTIFIER}\" and certificate leaf = H\"${MACOSX_SIGNING_CERT_SHA1}\"' --timestamp=none "$<TARGET_FILE:trusty>"
)
