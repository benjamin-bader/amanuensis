set(PLATFORM_SOURCES )
set(PLATFORM_LIBS )
if(APPLE)
    list(APPEND PLATFORM_SOURCES
        mac/MacLogSetup.cpp
        mac/MacLogSetup.h
        mac/MacProxy.cpp
        mac/MacProxy.h
    )

    list(APPEND PLATFORM_LIBS trusty_interface)
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES
        win/WindowsLogSetup.cpp
    )
endif()

add_executable(
    amanuensis
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
)

if(APPLE)
    set_target_properties(amanuensis
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSC_BUNDLE_INFO_PLIST Info.plist
    )

endif()
