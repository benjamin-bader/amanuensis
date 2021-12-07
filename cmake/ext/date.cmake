include(FetchContent)

FetchContent_Declare(
    date_ext
    URL https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.tar.gz
    URL_MD5 78902f47f7931a3ae8a320e0dea1f20a
)

FetchContent_MakeAvailable(date_ext)
