#!
# vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-06-25 16:40:21 - Создан.
#

set -e

default_cmpl() {
    make
    ./test_charconv_repair
}
Xcode_add_cmpls=
Xcode_add_opts() {
    printf -- "-G Xcode"
}
Xcode_cmpl() {
    open *.xcodeproj
}
Darwin_clang_ver=20
Darwin_add_cmpls="g++-mp-14 clang++-mp-$Darwin_clang_ver"
Darwin_add_opts() {
    case "$1" in
     clang++-mp-*)
	lf=-Wl,-L/opt/local/libexec/llvm-%d/lib/libc++
	lf=$lf,-rpath,/opt/local/libexec/llvm-%d/lib/libc++
	lf=$lf,-rpath,/opt/local/libexec/llvm-%d/lib/libunwind
	printf -- "-DCMAKE_CXX_ABI_COMPILED=FALSE
	    -DCMAKE_EXE_LINKER_FLAGS=$lf" \
	    $Darwin_clang_ver $Darwin_clang_ver $Darwin_clang_ver
	;;
     g++*)
	printf -- "-DWANT_SINGLE_INCLUDE:BOOL=1"
	;;
    esac
}
Darwin_cmpl() { default_cmpl; }
FreeBSD_gcc_ver=15
FreeBSD_add_cmpls="g++$FreeBSD_gcc_ver"
FreeBSD_add_opts() {
    case "$1" in
     g++*)
	printf -- "-DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath=/usr/local/lib/gcc%d
	    -DWANT_SINGLE_INCLUDE:BOOL=1" \
	    $FreeBSD_gcc_ver
	;;
    esac
}
FreeBSD_cmpl() { default_cmpl; }
Linux_add_cmpls="g++-11 clang++-19"
Linux_add_opts() { true; }
Linux_cmpl() { default_cmpl; }

if [ "--clean" = "$1" ] ; then
    clean=true
    shift
else
    clean=false
fi
b=`basename $0`
if cmp -s "$0" "$b" ; then
    platform=${1:-`uname -s`}
    cmpls=${2:-`eval "echo \'\' \\$${platform}_add_cmpls"`}
    src_dir=true
else
    cmp -s "$0" ../../"$b" || {
	echo "$0: Необходимо находиться, либо в сборочном, " \
	     "либо в исходном каталоге" 1>&2
	exit 2
    }
    d=$(basename `pwd`)
    if platform=`expr "$d" : "\([^-]*\)-.*"` ; then
	cmpls=`expr "$d" : "[^-]*-\(.*\)"`
    else
	platform=$d
	cmpls="''"
    fi
    src_dir=false
fi
for c in $cmpls ; do
    if [ "''" = "$c" ] ; then
	dir="$platform"
	args=""
    else
	dir="$platform-$c"
	args="-DCMAKE_CXX_COMPILER=$c"
    fi
    args="$args "`"${platform}_add_opts" "$c"` || echo "Use empty"
    (
        set -e
	if $src_dir ; then
	    mkdir -p "build/$dir"
	    cd "build/$dir"
	fi
	if $clean ; then
	    rm -r *
	else
	    cmake $args ../..
	    "${platform}_cmpl"
	fi
    )
done
exit 0
