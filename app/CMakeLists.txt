set(PLATFORM_SOURCES )
set(PLATFORM_LIBS )
set(PLATFORM_PROPS )
if(APPLE)
    list(APPEND PLATFORM_SOURCES
        mac/MacLogSetup.cpp
        mac/MacLogSetup.h
        mac/MacProxy.cpp
        mac/MacProxy.h
    )

    list(APPEND PLATFORM_LIBS trusty_interface)

    set(APP_PLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        "${APP_PLIST_FILE}"
    )

    list(APPEND PLATFORM_PROPS
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${APP_PLIST_FILE}"
    )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES
        win/WindowsLogSetup.cpp
    )
endif()

add_executable(amanuensis
    main.cpp
    LogSetup.cpp
    LogSetup.h
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    ${PLATFORM_SOURCES}
)

target_link_libraries(amanuensis
    core
    log
    Qt6::Gui
    Qt6::Widgets
    ${PLATFORM_LIBS}
)

target_include_directories(amanuensis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(amanuensis
    PROPERTIES
    OUTPUT_NAME Amanuensis
    ${PLATFORM_PROPS}
)

if(APPLE)
    

    find_program(CODESIGN codesign REQUIRED)
    find_program(MACDEPLOYQT macdeployqt REQUIRED)
    find_program(PLIST_BUDDY plist_buddy HINTS /usr/libexec REQUIRED)
    find_program(DSYMUTIL dsymutil REQUIRED)
    
    set(APP_PLIST_PATH "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

    set(SYMBOL_FILE "$<TARGET_BUNDLE_DIR:amanuensis>.dSYM")

    add_dependencies(amanuensis trusty)

    add_custom_command(
        TARGET amanuensis
        PRE_BUILD
        COMMAND cp ARGS -f "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" "${APP_PLIST_PATH}"
        COMMAND "${PLIST_BUDDY}" ARGS -c "Set :CFBundleIdentifier '${MACOSX_APP_IDENTIFIER}'" "${APP_PLIST_PATH}"
        COMMAND "${PLIST_BUDDY}" ARGS -c "Set :SMPrivilegedExecutables:${MACOSX_HELPER_IDENTIFIER} 'identifier \\\"${MACOSX_HELPER_IDENTIFIER}\\\" and certificate leaf = H\\\"${MACOSX_SIGNING_CERT_SHA1}\\\"'" "${APP_PLIST_PATH}"
    )

    add_custom_command(
        TARGET amanuensis
        POST_BUILD
        COMMAND mkdir ARGS -p "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/Library/LaunchServices"
        COMMAND mkdir ARGS -p "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/Resources"
        COMMAND cp ARGS -a "$<TARGET_FILE:trusty>" "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/Library/LaunchServices/"
        COMMAND cp ARGS -a "$<TARGET_FILE_DIR:trusty>/trusty-info.plist" "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/Resources/"
        COMMAND cp ARGS -a "$<TARGET_FILE_DIR:trusty>/trusty-launchd.plist" "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/Resources/"

        COMMAND "${DSYMUTIL}" ARGS "$<TARGET_FILE:amanuensis>" -o "${SYMBOL_FILE}"
        COMMAND cp ARGS -a "${SYMBOL_FILE}" "$<TARGET_BUNDLE_CONTENT_DIR:amanuensis>/MacOS/"

        COMMAND "${MACDEPLOYQT}" ARGS "$<TARGET_BUNDLE_DIR:amanuensis>" -always-overwrite "-codesign=${MACOSX_SIGNING_CERT_SHA1}"

        COMMAND "${CODESIGN}" ARGS --verbose --force --sign "${MACOSX_SIGNING_CERT_SHA1}" -r='designated => identifier \"${MACOSX_APP_IDENTIFIER}\" and certificate leaf[subject.OU] = \"bendb.com\"' --timestamp=none "$<TARGET_BUNDLE_DIR:amanuensis>"
    )
endif()
