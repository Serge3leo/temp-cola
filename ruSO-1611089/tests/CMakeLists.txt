# vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-06-28 12:03:12 - Создан.
#

set(TEST_SOURCES
        charconv_repair/from_to_chars.tests.cpp
        charconv_repair/has_std.tests.cpp
    )

if (MSVC)
    add_definitions(-W4 -WX -D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-Wall -Wextra -Werror)
endif()

add_executable(SelfTest ${TEST_SOURCES})
target_link_libraries(SelfTest PRIVATE FloatFormat::FloatFormat
                                       Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(SelfTest)
