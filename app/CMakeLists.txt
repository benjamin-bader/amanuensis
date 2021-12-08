set(PLATFORM_SOURCES )
if(APPLE)
    list(APPEND PLATFORM_SOURCES
        mac/MacLogSetup.cpp
        mac/MacLogSetup.h
        mac/MacProxy.cpp
        mac/MacProxy.h
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
)

target_include_directories(amanuensis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(APPLE)

    set_target_properties(
        amanuensis

        PROPERTIES

        MACOSX_BUNDLE TRUE
        MACOSC_BUNDLE_INFO_PLIST Info.plist
    )

endif()