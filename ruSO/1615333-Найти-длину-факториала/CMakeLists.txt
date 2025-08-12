# vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-08-05 18:18:58 - Создан.
#

cmake_minimum_required (VERSION 3.25)  # Debian 12 (11 backports), 3.20: C++23
project(float_format
    VERSION 0.1.0
    DESCRIPTION "Получение тестовых файлов для длины факториала lgamma()"
    LANGUAGES CXX)

# Параметры
set(FF_CATCH2_FROM "subdirectory" CACHE STRING "Источник Catch2")

# Зависимости: c++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Зависимости: Catch2
if ("find_package" STREQUAL FF_CATCH2_FROM)
    # First build: 0m3.294s, Clean build: 0m3.267s, Rebuild: 0m3.158s
    find_package(Catch2 3 REQUIRED)
elseif("FetchContent" STREQUAL FF_CATCH2_FROM)
    # First build: 2m2.604s, Clean build: 2m2.120s, Rebuild: 0m4.572s
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        2b60af89e23d28eefc081bc930831ee9d45ea58b # v3.0.1
    )
    FetchContent_MakeAvailable(Catch2)
elseif("subdirectory" STREQUAL FF_CATCH2_FROM)
    # First build: 2m1.610s, "Clean" build: 0m5.775s, Rebuild: 0m4.150s
    # TODO: Use `rm -rf rm -rf build/Platform-Compiler/*` for clean
    # TODO: SYSTEM EXCLUDE_FROM_ALL
    # TODO:site_name(CATCH2_SITE) или CMAKE_SYSTEM_NAME или CMAKE_SYSTEM_VERSION
    set(FF_CATCH2_DIR "${PROJECT_SOURCE_DIR}/../../contrib/Catch2-v3.x")
    get_filename_component(FF_CATCH_CXX "${CMAKE_CXX_COMPILER}" NAME)
    string(CONCAT FF_CATCH2_BIN_DIR "${FF_CATCH2_DIR}/build/"
                                    "${CMAKE_SYSTEM_NAME}-"
                                    "${CMAKE_SYSTEM_PROCESSOR}-"
                                    "${CMAKE_BUILD_TYPE}-"
                                    "${FF_CATCH_CXX}")
    add_subdirectory("${FF_CATCH2_DIR}" "${FF_CATCH2_BIN_DIR}")
    list(APPEND CMAKE_MODULE_PATH "${FF_CATCH2_DIR}/contrib")
    list(APPEND CMAKE_MODULE_PATH "${FF_CATCH2_DIR}/CMake")
elseif("single_include" STREQUAL FF_CATCH2_FROM)
    # First build: 0m10.354s, Clean build: 0m10.553s, Rebuild: 0m9.857s
    set(FF_CATCH2_DIR "${PROJECT_SOURCE_DIR}/../contrib/Catch2-v2.x")
    set(FF_CATCH2_INCLUDE "${FF_CATCH2_DIR}/single_include")
    add_library(Catch2 INTERFACE)
    add_library(Catch2::Catch2 ALIAS Catch2)
    target_include_directories(Catch2 INTERFACE ${FF_CATCH2_INCLUDE})
    target_compile_definitions(Catch2 INTERFACE -DCATCH2_SINGLE_INCLUDE)
    add_library(Catch2WithMain INTERFACE)
    add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
    target_link_libraries(Catch2WithMain INTERFACE Catch2)
    target_compile_definitions(Catch2WithMain INTERFACE -DCATCH_CONFIG_MAIN)
    list(APPEND CMAKE_MODULE_PATH "${FF_CATCH2_DIR}/contrib")
    list(APPEND CMAKE_MODULE_PATH "${FF_CATCH2_DIR}/CMake")
else()
    message("Некорректный FF_CATCH2_FROM=${FF_CATCH2_FROM}")
endif()
# unset(FF_CATCH2_FROM CACHE)  # TODO: Для отладки

## ?? Всё в тестах??
#set(CMAKE_FOLDER "tests")
#add_subdirectory(tests)


set(TEST_SOURCES
        i64_interval.cpp
    )

if (MSVC)
    add_definitions(-W4 -WX -D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-Wall -Wextra -Werror)
endif()

add_executable(SelfTest ${TEST_SOURCES})
target_link_libraries(SelfTest PRIVATE Catch2::Catch2WithMain)
if (FF_CATCH_ENABLE_COVERAGE)
    target_compile_options(SelfTest PRIVATE -Og -g)
    target_compile_options(SelfTest PRIVATE -coverage)
    target_link_options(SelfTest PRIVATE -coverage)
    # TODO: lcov/gcov Специализации шаблона неясны
    # TODO: непонятно как задать GCOV_COMMAND/GCOV_OPTIONS
    # Вместо `ctest -T Coverage` следует использовать `lcov/genhtml`
    #
    # # LLVM coverage для charconv_repair, как-то тоже не очень :(
    # # Хотя раньше, специализации шаблонов показывал.
    # if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # else()
    #     # TODO `-fprofile-list="$pdata"._profile_list`
    #     set(ENV{LLVM_PROFILE_FILE} "SelfTest.profraw")
    #     target_compile_options(SelfTest PRIVATE -fprofile-instr-generate
    #                                             -fcoverage-mapping)
    #                                             # TODO -fcoverage-mcdc)
    #     target_link_options(SelfTest PRIVATE -fprofile-instr-generate)
    # endif()
endif()

include(CTest)
include(Catch)

catch_discover_tests(SelfTest)
