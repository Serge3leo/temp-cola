#!
# vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-08-10 17:24:10 - Создан.
#
# Сшиваем файлы оценок числа цифр факториала с разной точностью в один файл.

set -e 

pat_select() {
    pat_start="$1"
    pat_stop="$2"
    shift 2
    notfound=true
    for f ; do
	last=`tail -1 "$f"`
	if $notfound ; then
	    if expr "$last" : "$pat_start" > /dev/null ; then
		notfound=false
	    else
		continue
	    fi
	fi
        if expr "$last" : "$pat_stop" > /dev/null ; then
            break
        fi
	wc=`wc "$f"`
        echo "# wc $f"
        echo "# $wc"
	# grep -v "^#" "$f" | head -1
        # echo "$last"
        cat "$f"
    done
}

(
    for f in out_i64/out/i64-15-* ; do
	head -1 "$f"  # Заголовки столбцов для np.loadtxt()/np.genfromtxt()
	break
    done
    cat << endhdr
# vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
#
# Интервал первых цифр (мантисса) и экспонента факториала, получены методом
# @StanislavVolodarskiy https://ru.stackoverflow.com/a/1615336/430734
#
# Отобраны следующие числа:
#  - Кратные 500'000'000 или 1'000'000'000;
#  - log10(number!) близко к целому с точность 15, 16 или 17 знаков;
#  - Случайно выбранные 2000 на 1'000'000'000 (1000 на 500'000'000).
#
#  До 11'000'000'000 - критерий точности 15 цифр, от 11'000'000'000 до
#  95'000'000'000 - 16 цифр, далее 17 цифр.

endhdr
    pat_15="^ *11500000000 "
    pat_16="^ *12000000000 "
    pat_16_17="^ *96000000000 "
    pat_select " "          "$pat_15"    out_i64/out/i64-15-*
    pat_select "$pat_16"    "$pat_16_17" out_i64/out/i64-16-*
    pat_select "$pat_16_17" "zz-for-end" out_i64/out/i64-17-*
) | bzip2 --compress -9 -v --stdout > out_i64/i64-15_17-19.txt.bz2
