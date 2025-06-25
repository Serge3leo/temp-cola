# vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
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

#add_library(mylib) # initialized below
#add_library(mylib::mylib ALIAS mylib)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# charconv_repair.hpp использует quadmath для GNU g++
if(CMAKE_COMPILER_IS_GNUCXX)
    add_link_options(-lquadmath)
endif()

add_executable(test_charconv_repair test_charconv_repair.cpp)
target_include_directories(test_charconv_repair PRIVATE catch2)

if(MSVC)
    target_compile_options(test_charconv_repair PRIVATE
	/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(test_charconv_repair PRIVATE -Wall -Wextra -Werror)
endif()


