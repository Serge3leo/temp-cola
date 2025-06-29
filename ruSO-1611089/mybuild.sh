#!
# vim:set sw=4 ts=8 fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
# Лицензия: BSD
# Автор: Сергей Леонтьев (leo@sai.msu.ru)
# История:
# 2025-06-25 16:40:21 - Создан.
#

set -e

ENABLE_COVERAGE=${ENABLE_COVERAGE:--DFF_CATCH_ENABLE_COVERAGE=ON}
LCOV=${LCOV:-lcov}
GENHTML=${GENHTML:-genhtml}
EXLUDES=${EXLUDES:---exclude=*/catch2/*}
GCC_STDCPP_EXLUDES=${GCC_STDCPP_EXLUDES:---exclude=*/c++/* --exclude=c++/* }
CLANG_LIBCPP_EXLUDES=${CLANG_LIBCPP_EXLUDES:---exclude=*/v1/* --exclude=v1/*}

Cvrg_arg=false
cmpls_arg=
lcov_args=
platform_arg=
rm_arg=false
verbose=false
while getopts Cc:l:p:rv flag ; do
    case "$flag" in
    C)  Cvrg_arg=true;;
    c)	cmpls_arg="$OPTARG";;
    l)  lcov_args="$OPTARG"
	Cvrg_arg=true;;
    p)  platform_arg="$OPTARG";;
    r)	rm_arg=true;;
    v)	verbose=true;;
    *)	echo '' "Usage: $0 [-Crv] [-p platform] [-c cmpls]" \
	        "[-l lcov args] [--] [cmake_args]"
        echo '  -С - Покрытие кода;'
        echo '  -r - Удалить содержимое директории сборки;'
	echo '  -v - Расширенная печать;'
	echo '  -p platform - Платформа, по умолчанию `uname -s`;'
	echo '  -c cmpls - Список компиляторов;'
	echo '  -l lcov args - Покрытие кода, аргументы lcov.'
	echo ''
	echo 'Пример: ./mybuild.sh -c g++-mp-14 -l "--gcov-tool=gcov-mp-14' \
	     '--ignore-errors inconsistent,inconsistent"'
	exit 2
    esac
done
shift $(($OPTIND - 1))
if "$verbose" ; then
    printf "Cvrg_arg=%s rm_arg=%s platform_arg=%s cmpls_arg=%s lcov_args=%s\n" \
	   $Cvrg_arg, $rm_arg "$platform_arg" "$cmpls_arg" "$lcov_args"
    printf "CMake arguments: %s\n" "$*"
fi

default_cmpl() {
    make
    ctest
    if "$Cvrg_arg" ; then
	if "$verbose" ; then
	    echo $lcov_cmd \
		 --capture --directory tests --output-file coverage.info
	    echo $GENHTML coverage.info --output-directory out
	fi
	$lcov_cmd --capture --directory tests --output-file coverage.info
	$GENHTML --output-directory out coverage.info
    fi
}
Xcode_cmpls="''"
Xcode_args() {
    cmake_args="$cmake_args -G Xcode"
}
Xcode_cmpl() {
    open *.xcodeproj
}
Darwin_Xcode_cov="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/llvm-cov"
Darwin_cmpls="'' g++-mp-14 clang++-mp-20"
Darwin_args() {
    case "$1" in
    "''")
        lcov_cmd="$lcov_cmd --gcov-tool=$Darwin_Xcode_cov,gcov "`
			`"--ignore-errors format,format,unsupported,unused "`
		    	`"$CLANG_LIBCPP_EXLUDES"
	;;
    g++-mp-*)
	v=`expr "$1" : 'g++-mp-\(.*\)'`
        lcov_cmd="$lcov_cmd --gcov-tool=gcov-mp-$v "`
			`"--ignore-errors inconsistent,inconsistent,unused "`
	    		`"$GCC_STDCPP_EXLUDES"
	;;
    clang++-mp-*)
	v=`expr "$1" : 'clang++-mp-\(.*\)'`
        lcov_cmd="$lcov_cmd --gcov-tool=llvm-cov-mp-$v,gcov "`
		     	`"--ignore-errors format,format,unsupported,unused "`
	    		`"$CLANG_LIBCPP_EXLUDES"
	lf="-Wl,-L/opt/local/libexec/llvm-$v/lib/libc++"`
           `",-rpath,/opt/local/libexec/llvm-$v/lib/libc++"`
           `",-rpath,/opt/local/libexec/llvm-$v/lib/libunwind"
	cmake_args="$cmake_args -DCMAKE_CXX_ABI_COMPILED=FALSE "`
	                      `"-DCMAKE_EXE_LINKER_FLAGS=$lf"
    esac
}
Darwin_cmpl() {
    default_cmpl
    if "$Cvrg_arg" ; then
	open out/index.html
    fi
}
FreeBSD_cmpls="'' g++15"
FreeBSD_args() {
    case "$1" in
    "''")
        lcov_cmd="$lcov_cmd $CLANG_LIBCPP_EXLUDES"
        ;;
    g++*)
        v=`expr "$1" : 'g++\(.*\)'`
	lcov_cmd="$lcov_cmd --gcov-tool=gcov$v "`
	                  `"$GCC_STDCPP_EXLUDES"
	cmake_args="$cmake_args "`
	           `"-DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath=/usr/local/lib/gcc$v"
    esac
}
FreeBSD_cmpl() { default_cmpl; }
Linux_cmpls="'' g++-11 clang++-19"
Linux_args() {
    case "$1" in
    "''")
	lcov_cmd="$lcov_cmd --ignore-errors inconsistent,unused,unused"`
	                  `"$GCC_STDCPP_EXLUDES"
        ;;
    g++-*)
	v=`expr "$1" : 'g++-\(.*\)'`
	lcov_cmd="$lcov_cmd --gcov-tool=gcov-$v "`
	                  `"--ignore-errors inconsistent,unused,unused"`
	                  `"$GCC_STDCPP_EXLUDES"
	;;
    clang++-*)
        v=`expr "$1" : 'clang++-\(.*\)'`
        lcov_cmd="$lcov_cmd --gcov-tool=llvm-cov-$v,gcov "`
	                  `"--ignore-errors inconsistent,format,unused,unused"`
	                  `"$CLANG_LIBCPP_EXLUDES"
    esac
}
Linux_cmpl() { default_cmpl; }

b=`basename $0`
if [ -r CMakeLists.txt -a -r "$b" -a -d build ] && cmp -s "$0" "$b" ; then
    platform=${platform_arg:-`uname -s`}
    cmpls=${cmpls_arg:-`eval "echo \\$${platform}_cmpls"`}
    src_dir=true
else
    ([ -r ../../CMakeLists.txt -a -r ../../"$b" -a -d ../../build ] &&
     cmp -s "$0" ../../"$b") || {
	echo "$0: Необходимо находиться, либо в сборочном, " \
	     "либо в исходном каталоге" 1>&2
	exit 2
    }
    [ -z "$platform_arg" -a -z "$cmpls_arg" ] || {
	echo "$0: Ключи [-p platform] [-c cmpls] должны быть опущены" 1>&2
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
if "$verbose" ; then
    printf "src_dir=%s platform=%s cmpls=%s\n" \
	   $src_dir "$platform" "$cmpls"
fi
for c in $cmpls ; do
    if [ "''" = "$c" ] ; then
	dir="$platform"
	cmake_args=""
    else
	dir="$platform-$c"
	cmake_args="-DCMAKE_CXX_COMPILER=$c"
    fi
    if "$Cvrg_arg" ; then
	cmake_args="$cmake_args $ENABLE_COVERAGE"
    fi
    lcov_cmd="$LCOV $EXLUDES"
    "${platform}_args" "$c"
    if [ -n "$lcov_args" ] ; then
	lcov_cmd="$LCOV $lcov_args"
    fi
    (
        set -e
	if "$src_dir" ; then
	    mkdir -p "build/$dir"
	    cd "build/$dir"
	fi
	if "$rm_arg" ; then
	    if "$verbose" ; then
		echo "rm -rf ../../build/$dir/*"
	    fi
	    rm -rf ../../build/$dir/*
	else
	    if "$verbose" ; then
		echo "cmake $cmake_args $* ../.."
	    fi
	    cmake $cmake_args "$@" ../..
	    "${platform}_cmpl"
	fi
    )
done
exit 0
