# I don't know if setting these policies is okay but I did it anyway to get rid
# of warnings
# https://cmake.org/cmake/help/latest/policy/CMP0135.html
cmake_policy(SET CMP0135 NEW)
# https://cmake.org/cmake/help/latest/policy/CMP0077.html
cmake_policy(SET CMP0077 NEW)

# I found out that FetchContent_MakeAvailable does add_subdirectory by default
# (which I did manually for all of the libs anyway) so I will roll with it until
# I find any problems
include(FetchContent)

set(DEP_LIBS "")

if (USE_CPPUTILS)
    FetchContent_Declare(
        cpputils
        URL https://github.com/A-Ih/cpputils/archive/refs/heads/main.zip
        )
    message("Fetching cpputils...")
    FetchContent_MakeAvailable(cpputils)
    message("Done")
    list(APPEND DEP_LIBS cpputils::cpputils)
endif()

if (USE_GTEST)
    FetchContent_Declare(
        googletest
        URL https://github.com/A-Ih/googletest/archive/refs/heads/main.zip
        )
    message("Fetching googletest...")
    FetchContent_MakeAvailable(googletest)
    message("Done")
    list(APPEND DEP_LIBS gtest_main)
endif()

if(USE_FMT)
    FetchContent_Declare(
        fmt
        URL https://github.com/A-Ih/fmt/archive/refs/heads/master.zip
        )
    message("Fetching fmt...")
    FetchContent_MakeAvailable(fmt)
    message("Done")
    list(APPEND DEP_LIBS fmt::fmt)
endif()

if(USE_RANGEV3)
    FetchContent_Declare(
        range-v3
        URL https://github.com/A-Ih/range-v3/archive/refs/heads/master.zip
        )
    message("Fetching range-v3...")
    FetchContent_MakeAvailable(range-v3)
    message("Done")
    list(APPEND DEP_LIBS range-v3::range-v3)
endif()

if(USE_RE2)
    FetchContent_Declare(
        re2
        URL https://github.com/A-Ih/re2/archive/refs/heads/main.zip
        )
    message("Fetching re2...")
    set(RE2_BUILD_TESTING OFF)
    FetchContent_MakeAvailable(re2)
    message("Done")
    list(APPEND DEP_LIBS re2::re2)
endif()

if(USE_JSON)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/archive/refs/heads/master.zip
        )
    message("Fetching json...")
    FetchContent_MakeAvailable(json)
    message("Done")
    list(APPEND DEP_LIBS nlohmann_json::nlohmann_json)
endif()

if(USE_SPDLOG)
    FetchContent_Declare(
        spdlog
        URL https://github.com/A-Ih/spdlog/archive/refs/heads/v1.x.zip
        )
    message("Fetching spdlog...")
    FetchContent_MakeAvailable(spdlog)
    message("Done")
    list(APPEND DEP_LIBS spdlog::spdlog)
endif()
