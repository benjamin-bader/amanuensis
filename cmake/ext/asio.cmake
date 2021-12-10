include(FetchContent)

FetchContent_Declare(
    asio_ext
    GIT_REPOSITORY "https://github.com/chriskohlhoff/asio"
    GIT_TAG asio-1-21-0 
)

FetchContent_GetProperties(asio_ext)

if(NOT asio_ext_POPULATED AND NOT TARGET asio)
    FetchContent_Populate(asio_ext)

    add_library(asio STATIC
        ${asio_ext_SOURCE_DIR}/asio/src/asio.cpp
        ${asio_ext_SOURCE_DIR}/asio/src/asio_ssl.cpp
    )
    add_library(asio::asio ALIAS asio)
    target_include_directories(asio PUBLIC ${asio_ext_SOURCE_DIR}/asio/include)

    find_package(Threads)
    target_link_libraries(asio PRIVATE Threads::Threads)
    target_link_libraries(asio PRIVATE OpenSSL::SSL OpenSSL::Crypto)

    target_compile_features(asio PUBLIC cxx_std_11)
    target_compile_definitions(asio PUBLIC -DASIO_STANDALONE -DASIO_SEPARATE_COMPILATION)
    set_target_properties(asio PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
