include(FetchContent)

FetchContent_Declare(
    date_ext
    URL https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.tar.gz
    URL_MD5 78902f47f7931a3ae8a320e0dea1f20a
)

FetchContent_GetProperties(date_ext)
if(NOT date_ext_POPULATED)
    FetchContent_Populate(date_ext)
    add_subdirectory("${date_ext_SOURCE_DIR}" "${date_ext_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

set_target_properties(date PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)
