#!/bin/sh

# fail on error
set -e

# example sources
EXAMPLES="format listdir range range_pipe signal"
EXAMPLES="${EXAMPLES} stream1 stream2 coroutine1 coroutine2"

# assembly sources
ASM_SOURCE_DIR="src/asm"
ASM_SOURCES="jump_all_gas make_all_gas ontop_all_gas"

# c++ sources
CXX_SOURCE_DIR="src"
CXX_SOURCES="context_stack"

# output lib
OSTD_LIB="libostd"
OSTD_DYNLIB=""

# default opts
BUILD_EXAMPLES="true"
BUILD_TESTSUITE="true"
BUILD_LIB="mixed"
BUILD_CFG="debug"
VERBOSE="false"
CLEAN="false"

print_help() {
    echo "$1 [options]"
    echo "Available options:"
    echo "  [no-]examples   - (do not) build examples (default: build)"
    echo "  [no-]test-suite - (do not) build test suite (default: build)"
    echo "  static-lib      - static libostd"
    echo "  shared-lib      - shared libostd"
    echo "  mixed-lib       - static + shared libostd (default)"
    echo "  release         - release build (strip, no -g)"
    echo "  debug           - debug build (default, no strip, -g)"
    echo "  verbose         - print entire commands"
    echo "  clean           - remove temporary files and exit"
    echo "  help            - print this"
}

for arg in "$@"; do
    case "$arg" in
       examples) BUILD_EXAMPLES="true" ;;
       no-examples) BUILD_EXAMPLES="false" ;;
       test-suite) BUILD_TESTSUITE="true" ;;
       no-test-suite) BUILD_TESTSUITE="false" ;;
       static-lib) BUILD_LIB="static" ;;
       shared-lib) BUILD_LIB="shared" ;;
       mixed-lib) BUILD_LIB="mixed" ;;
       release) BUILD_CFG="release" ;;
       debug) BUILD_CFG="debug" ;;
       verbose) VERBOSE="true" ;;
       clean) CLEAN="true" ;;
       help) print_help "$0"; exit 0 ;;
       *) ;;
    esac
    shift
done

if [ "$BUILD_LIB" = "shared" ]; then
    OSTD_LIB="${OSTD_LIB}.so"
    OSTD_DYNLIB=""
else
    OSTD_LIB="${OSTD_LIB}.a"
    OSTD_DYNLIB="${OSTD_LIB}.so"
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
if [ -z "$CROSS" ]; then
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

# -fPIC for shared libs
if [ "$BUILD_LIB" = "shared" ]; then
    OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -fPIC"
fi

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
OSTD_LDFLAGS=""

# custom linker flags
if [ ! -z "$LDFLAGS" ]; then
    OSTD_LDFLAGS="${OSTD_LDFLAGS} ${LDFLAGS}"
fi

# assembler flags
OSTD_ASFLAGS=""

# custom assembler flags
if [ ! -z "$LDFLAGS" ]; then
    OSTD_LDFLAGS="${OSTD_LDFLAGS} ${LDFLAGS}"
fi

#
# BUILD LOGIC
#

evalv() {
    if [ "$VERBOSE" = "true" ]; then
        echo "$@"
    fi
    eval "$@"
}

echoq() {
    if [ "$VERBOSE" = "false" ]; then
        echo "$@"
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
        evalv "rm -f \"${ASM_SOURCE_DIR}/${as}_dyn.o\""
    done
    for cs in ${CXX_SOURCES}; do
        evalv "rm -f \"${CXX_SOURCE_DIR}/${cs}.o\""
        evalv "rm -f \"${CXX_SOURCE_DIR}/${cs}_dyn.o\""
    done
    evalv "rm -f \"$OSTD_LIB\""
    if [ "$BUILD_LIB" = "mixed" ]; then
        evalv "rm -f \"$OSTD_DYNLIB\""
    fi
    evalv "rm -f test_runner.o test_runner"
}

# call_cxx input output
call_cxx() {
    echoq "CXX: $1"
    evalv "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} -c -o \"${2}\" \"${1}\""
    if [ "$BUILD_LIB" = "mixed" ] && [ "x$3" = "xlib" ]; then
        echoq "CXX (dynamic): $1"
        DYNOBJ=$(echo "$2" | sed 's/\.o/_dyn\.o/g')
        evalv "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} -fPIC -c -o \"${DYNOBJ}\" \"${1}\""
    fi
}

# call_as input output
call_as() {
    echoq "AS: $1"
    evalv "${CPP} -x assembler-with-cpp \"${1}\" | ${AS} -o \"${2}\""
    if [ "$BUILD_LIB" = "mixed" ]; then
        cp "$2" "$(echo $2 | sed 's/\.o/_dyn\.o/')"
    fi
}

# call_ld output file1 file2 ...
call_ld() {
    echoq "LD: $1"
    evalv "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} ${OSTD_LDFLAGS} -o $@"
    if [ "$BUILD_CFG" = "release" ]; then
        echoq "STRIP: $1"
        evalv "$STRIP \"$1\""
    fi
}

# call_ldlib output file1 file2 ...
call_ldlib() {
    if [ "$BUILD_LIB" = "shared" ]; then
        call_ld "$@" "-shared"
    else
        echoq "AR: $1"
        evalv "${AR} rcs $@"
    fi
    if [ "$BUILD_LIB" = "mixed" ]; then
        OUT=$(echo "$1" | sed 's/\.a/\.so/')
        shift
        OBJS="$@"
        DYN_OBJ=$(echo "$OBJS" | sed 's/\.o/_dyn\.o/g')
        call_ld "$OUT" "$DYN_OBJ" "-fPIC -shared"
    fi
}

# build_example name
build_example() {
    call_cxx "examples/${1}.cc" "examples/${1}.o"
    call_ld "examples/${1}" "examples/${1}.o" "$OSTD_LIB"
    rm -f "examples/${1}.o"
}

# build test runner
build_test_runner() {
    call_cxx test_runner.cc test_runner.o
    call_ld test_runner test_runner.o "$OSTD_LIB"
    rm -f test_runner.o
}

# add_sfx_pfx str sfx pfx
add_sfx_pfx() {
    RET=""
    for it in $1; do
        RET="$RET ${3}${it}${2}"
    done
    echo $RET
}

# check if cleaning
if [ "$CLEAN" = "true" ]; then
    clean
    exit 0
fi

# build assembly
echo "Building the library..."
for as in $ASM_SOURCES; do
    call_as "${ASM_SOURCE_DIR}/${as}.S" "${ASM_SOURCE_DIR}/${as}.o" &
done
for cs in $CXX_SOURCES; do
    call_cxx "${CXX_SOURCE_DIR}/${cs}.cc" "${CXX_SOURCE_DIR}/${cs}.o" lib &
done
wait

ASM_OBJ=$(add_sfx_pfx "$ASM_SOURCES" ".o" "$ASM_SOURCE_DIR/")
CXX_OBJ=$(add_sfx_pfx "$CXX_SOURCES" ".o" "$CXX_SOURCE_DIR/")
CXX_DYNOBJ=$(add_sfx_pfx "$CXX_SOURCES" ".o" "$CXX_SOURCE_DIR/dyn_")

call_ldlib "$OSTD_LIB" "$ASM_OBJ" "$CXX_OBJ"
evalv "rm -f $ASM_OBJ"
evalv "rm -f $CXX_OBJ"

# build test runner
if [ "$BUILD_TESTSUITE" = "true" ]; then
    echo "Building test runner..."
    build_test_runner &
fi

# build examples
if [ "$BUILD_EXAMPLES" = "true" ]; then
    echo "Building examples..."
    for ex in $EXAMPLES; do
        build_example "$ex" &
    done
fi

# wait for remaining tasks
wait

# done
exit 0
