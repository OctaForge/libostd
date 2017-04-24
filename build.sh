#!/bin/sh

# fail on error
set -e

# example sources
EXAMPLES="format listdir range range_pipe signal"
EXAMPLES="${EXAMPLES} stream1 stream2 coroutine1 coroutine2 concurrency"

# assembly sources
ASM_SOURCE_DIR="src/asm"
ASM_SOURCES="jump_all_gas make_all_gas ontop_all_gas"

# c++ sources
CXX_SOURCE_DIR="src"
CXX_SOURCES="context_stack io concurrency"

# output lib
OSTD_LIB="libostd"

# default opts
BUILD_EXAMPLES="yes"
BUILD_TESTSUITE="yes"
BUILD_STATIC="yes"
BUILD_SHARED="no"
BUILD_CFG="debug"
VERBOSE="no"
CLEAN="no"

print_help() {
cat << EOF
$1 [options]
Available options:
  [no-]examples   - (do not) build examples (default: yes)
  [no-]test-suite - (do not) build test suite (default: yes)
  [no-]static-lib - (do not) build static libostd (default: yes)
  [no-]shared-lib - (do not) build shared libostd (default: no)
  release         - release build (strip, no -g)
  debug           - debug build (default, no strip, -g)
  verbose         - print entire commands
  clean           - remove generated files and exit
  help            - print this and exit
EOF
}

for arg in "$@"; do
    case "$arg" in
       examples) BUILD_EXAMPLES="yes" ;;
       no-examples) BUILD_EXAMPLES="no" ;;
       test-suite) BUILD_TESTSUITE="yes" ;;
       no-test-suite) BUILD_TESTSUITE="no" ;;
       static-lib) BUILD_STATIC="yes" ;;
       no-static-lib) BUILD_STATIC="no" ;;
       shared-lib) BUILD_SHARED="yes" ;;
       no-shared-lib) BUILD_SHARED="no" ;;
       release) BUILD_CFG="release" ;;
       debug) BUILD_CFG="debug" ;;
       verbose) VERBOSE="yes" ;;
       clean) CLEAN="yes" ;;
       help) print_help "$0"; exit 0 ;;
       *) ;;
    esac
    shift
done

OSTD_STATIC_LIB="${OSTD_LIB}.a"
OSTD_SHARED_LIB="${OSTD_LIB}.so"
OSTD_DEFAULT_LIB="$OSTD_SHARED_LIB"

if [ "$BUILD_STATIC" = "yes" ]; then
    OSTD_DEFAULT_LIB="$OSTD_STATIC_LIB"
fi

# compiler
if [ -z "$CXX" ]; then
    CXX="c++"
fi

# preprocessor
if [ -z "$CPP" ]; then
    CPP="cpp"
fi

# assembler
if [ -z "$AS" ]; then
    AS="as"
fi

# ar
if [ -z "$AR" ]; then
    AR="ar"
fi

# ar
if [ -z "$STRIP" ]; then
    STRIP="strip"
fi

# cross builds
if [ ! -z "$CROSS" ]; then
    CXX="${CROSS}${CXX}"
    CPP="${CROSS}${CPP}"
    AS="${CROSS}${AS}"
    AR="${CROSS}${AR}"
    STRIP="${CROSS}${STRIP}"
fi

# c++ standard
OSTD_CXXFLAGS="-std=c++1z"

# includes
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -I."

# optimization flags
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -O2"

# warnings
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -Wall -Wextra -Wshadow -Wold-style-cast"

# -g for debug builds
if [ "$BUILD_CFG" = "debug" ]; then
    OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -g"
fi

# custom cxxflags
if [ ! -z "$CXXFLAGS" ]; then
    OSTD_CXXFLAGS="${OSTD_CXXFLAGS} ${CXXFLAGS}"
fi

# preprocessor flags
OSTD_CPPFLAGS=""

# custom cppflags
if [ ! -z "$CPPFLAGS" ]; then
    OSTD_CPPFLAGS="${OSTD_CPPFLAGS} ${CPPFLAGS}"
fi

# linker flags
OSTD_LDFLAGS="-pthread"

# custom linker flags
if [ ! -z "$LDFLAGS" ]; then
    OSTD_LDFLAGS="${OSTD_LDFLAGS} ${LDFLAGS}"
fi

# assembler flags
OSTD_ASFLAGS=""

# custom assembler flags
if [ ! -z "$ASFLAGS" ]; then
    OSTD_ASFLAGS="${OSTD_ASFLAGS} ${ASFLAGS}"
fi

#
# BUILD LOGIC
#

evalv() {
    if [ "$VERBOSE" = "yes" ]; then
        echo "$*"
    fi
    eval "$*"
}

echoq() {
    if [ "$VERBOSE" = "no" ]; then
        echo "$*"
    fi
}

# clean everything
clean() {
    echo "Cleaning..."
    for ex in ${EXAMPLES}; do
        evalv "rm -f \"examples/${ex}\" \"examples/${ex}.o\""
    done
    for as in ${ASM_SOURCES}; do
        evalv "rm -f \"${ASM_SOURCE_DIR}/${as}.o\""
    done
    for cs in ${CXX_SOURCES}; do
        evalv "rm -f \"${CXX_SOURCE_DIR}/${cs}.o\""
        evalv "rm -f \"${CXX_SOURCE_DIR}/${cs}_dyn.o\""
    done
    evalv "rm -f \"$OSTD_STATIC_LIB\""
    evalv "rm -f \"$OSTD_SHARED_LIB\""
    evalv "rm -f test_runner.o test_runner"
}

# call_cxx input output [shared]
call_cxx() {
    FLAGS="${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS}"
    if [ "x$3" = "xshared" ]; then
        echoq "CXX (shared): $1"
        FLAGS="${FLAGS} -fPIC"
    else
        echoq "CXX: $1"
    fi
    evalv "${CXX} ${FLAGS} -c -o \"${2}\" \"${1}\""
}

# call_as input output
call_as() {
    echoq "AS: $1"
    evalv "${CPP} -x assembler-with-cpp \"${1}\" | \
        ${AS} ${OSTD_ASFLAGS} -o \"${2}\""
}

# call_ld output file1 file2 ...
call_ld() {
    echoq "LD: $1"
    evalv "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} -o $@ ${OSTD_LDFLAGS}"
    if [ "$BUILD_CFG" = "release" ]; then
        echoq "STRIP: $1"
        evalv "$STRIP \"$1\""
    fi
}

# call_ldlib output file1 file2 ...
call_ldlib() {
    LIBTYPE="$1"
    shift
    if [ "$LIBTYPE" = "shared" ]; then
        call_ld "$@" "-shared"
    else
        echoq "AR: $1"
        evalv "${AR} rcs $@"
    fi
}

# build_example name
build_example() {
    call_cxx "examples/${1}.cc" "examples/${1}.o"
    call_ld "examples/${1}" "examples/${1}.o" "$OSTD_DEFAULT_LIB"
    rm -f "examples/${1}.o"
}

# build test runner
build_test_runner() {
    OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -DOSTD_DEFAULT_LIB='\"${OSTD_DEFAULT_LIB}\"'"
    call_cxx test_runner.cc test_runner.o
    call_ld test_runner test_runner.o "$OSTD_DEFAULT_LIB"
    rm -f test_runner.o
}

# check if cleaning
if [ "$CLEAN" = "yes" ]; then
    clean
    exit 0
fi

# build library

# built as files are compiled, used at link
ASM_OBJ=""
CXX_OBJ=""
CXX_DYNOBJ=""

echo "Building the library..."
for as in $ASM_SOURCES; do
    call_as "${ASM_SOURCE_DIR}/${as}.S" "${ASM_SOURCE_DIR}/${as}.o" &
    ASM_OBJ="${ASM_OBJ} ${ASM_SOURCE_DIR}/${as}.o"
done
for cs in $CXX_SOURCES; do
    if [ "$BUILD_STATIC" = "yes" ]; then
        call_cxx "${CXX_SOURCE_DIR}/${cs}.cc" "${CXX_SOURCE_DIR}/${cs}.o" &
        CXX_OBJ="${CXX_OBJ} ${CXX_SOURCE_DIR}/${cs}.o"
    fi
    if [ "$BUILD_SHARED" = "yes" ]; then
        call_cxx "${CXX_SOURCE_DIR}/${cs}.cc" \
            "${CXX_SOURCE_DIR}/${cs}_dyn.o" shared &
        CXX_DYNOBJ="${CXX_DYNOBJ} ${CXX_SOURCE_DIR}/${cs}_dyn.o"
    fi
done
wait

if [ "$BUILD_STATIC" = "yes" ]; then
    call_ldlib static "$OSTD_STATIC_LIB" "$ASM_OBJ" "$CXX_OBJ"
    evalv "rm -f $CXX_OBJ"
fi
if [ "$BUILD_SHARED" = "yes" ]; then
    call_ldlib shared "$OSTD_SHARED_LIB" "$ASM_OBJ" "$CXX_DYNOBJ"
    evalv "rm -f $CXX_DYNOBJ"
fi
evalv "rm -f $ASM_OBJ"

# build test runner
if [ "$BUILD_TESTSUITE" = "yes" ]; then
    echo "Building test runner..."
    build_test_runner &
fi

# build examples
if [ "$BUILD_EXAMPLES" = "yes" ]; then
    echo "Building examples..."
    for ex in $EXAMPLES; do
        build_example "$ex" &
    done
fi

# wait for remaining tasks
wait

# done
exit 0
