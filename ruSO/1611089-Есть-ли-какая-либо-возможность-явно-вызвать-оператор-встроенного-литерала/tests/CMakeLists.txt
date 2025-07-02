# vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-06-28 12:03:12 - Создан.
#

set(TEST_SOURCES
        charconv_repair/from_to_chars-tests.cpp
        charconv_repair/has_std-tests.cpp
    )

if (MSVC)
    add_definitions(-W4 -WX -D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-Wall -Wextra -Werror)
endif()

add_executable(SelfTest ${TEST_SOURCES})
target_link_libraries(SelfTest PRIVATE FloatFormat::FloatFormat
                                       Catch2::Catch2WithMain)
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
