# vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-06-25 03:06:18 - Создан.
#

cmake_minimum_required (VERSION 3.25)  # Debian 12 (11 backports), 3.20: C++23
project(float_format
    VERSION 0.1.0
    DESCRIPTION "Шаблоны для корректной работы с float128_t, long double"
    LANGUAGES CXX)

# TODO add_library(project INTERFACE file1.hpp file2.hpp)
# https://stackoverflow.com/questions/60604249/how-to-make-a-header-only-library-with-cmake#comment122246327_60604878

add_library(FloatFormat INTERFACE include/float_format/charconv_repair.hpp)
add_library(FloatFormat::FloatFormat ALIAS FloatFormat)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_include_directories(FloatFormat INTERFACE include)
if(CMAKE_COMPILER_IS_GNUCXX)
    # charconv_repair.hpp использует quadmath для GNU g++
    target_link_libraries(FloatFormat INTERFACE -lquadmath)
endif()

set(CATCH2_FROM "subdirectory" CACHE STRING
    "Использовать ../contrib/Catch2-v3.x")
if ("find_package" STREQUAL CATCH2_FROM)
    # First build: 0m3.294s, Clean build: 0m3.267s, Rebuild: 0m3.158s
    find_package(Catch2 3 REQUIRED)
elseif("FetchContent" STREQUAL CATCH2_FROM)
    # First build: 2m2.604s, Clean build: 2m2.120s, Rebuild: 0m4.572s
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        2b60af89e23d28eefc081bc930831ee9d45ea58b # v3.0.1
    )
    FetchContent_MakeAvailable(Catch2)
elseif("subdirectory" STREQUAL CATCH2_FROM)
    # First build: 2m1.610s, "Clean" build: 0m5.775s, Rebuild: 0m4.150s
    # TODO: Use `rm -rf rm -rf build/Platform-Compiler/*` for clean
    # TODO: SYSTEM EXCLUDE_FROM_ALL
    # TODO:site_name(CATCH2_SITE) или CMAKE_SYSTEM_NAME или CMAKE_SYSTEM_VERSION
    set(CATCH2_DIR "${PROJECT_SOURCE_DIR}/../contrib/Catch2-v3.x")
    get_filename_component(CATCH_CXX "${CMAKE_CXX_COMPILER}" NAME)
    string(CONCAT CATCH2_BIN_DIR "${CATCH2_DIR}/build/"
                                 "${CMAKE_SYSTEM_NAME}-"
                                 "${CMAKE_SYSTEM_PROCESSOR}-"
                                 "${CMAKE_BUILD_TYPE}-"
                                 "${CATCH_CXX}")
    add_subdirectory("${CATCH2_DIR}" "${CATCH2_BIN_DIR}")
    list(APPEND CMAKE_MODULE_PATH "${CATCH2_DIR}/contrib")
elseif("single_include" STREQUAL CATCH2_FROM)
    # First build: 0m10.354s, Clean build: 0m10.553s, Rebuild: 0m9.857s
    set(CATCH2_DIR "${PROJECT_SOURCE_DIR}/../contrib/Catch2-v2.x")
    set(CATCH2_INCLUDE "${CATCH2_DIR}/single_include")
    add_library(Catch2 INTERFACE)
    add_library(Catch2::Catch2 ALIAS Catch2)
    target_include_directories(Catch2 INTERFACE ${CATCH2_INCLUDE})
    target_compile_definitions(Catch2 INTERFACE -DCATCH2_SINGLE_INCLUDE)
    add_library(Catch2WithMain INTERFACE)
    add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
    target_link_libraries(Catch2WithMain INTERFACE Catch2)
    target_compile_definitions(Catch2WithMain INTERFACE -DCATCH_CONFIG_MAIN)
    list(APPEND CMAKE_MODULE_PATH "${CATCH2_DIR}/contrib")
else()
    message("Некорректный CATCH2_FROM=${CATCH2_FROM}")
endif()
unset(CATCH2_FROM CACHE)  # Для отладки

set(CMAKE_FOLDER "tests")
add_subdirectory(tests)
include(CTest)
