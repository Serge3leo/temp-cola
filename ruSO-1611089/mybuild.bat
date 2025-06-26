@echo off
rem vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
rem Лицензия: BSD
rem Автор: Сергей Леонтьев (leo@sai.msu.ru)
rem История:
rem 2025-06-26 06:48:47 - Создан.

md build\vs17_2022
cd build\vs17_2022
cmake ..\..
start .

