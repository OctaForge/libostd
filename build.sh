#!/bin/sh

# fail on error
set -e

# example sources
EXAMPLES="format listdir range range_pipe signal stream1 stream2"

# assembly sources
ASM_SOURCES="jump_all_gas make_all_gas ontop_all_gas"
ASM_LIB="context.a"

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

# c++ standard
OSTD_CXXFLAGS="-std=c++1z"

# includes
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -I."

# optimization flags
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -O2"

# warnings
OSTD_CXXFLAGS="${OSTD_CXXFLAGS} -Wall -Wextra -Wshadow -Wold-style-cast"

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

# clean everything
clean() {
    echo "Cleaning..."
    for ex in ${EXAMPLES}; do
        rm -f "examples/${ex}" "examples/${ex}.o"
    done
    for as in ${ASM_SOURCES}; do
        rm -f "${as}.o"
    done
    rm -f "$ASM_LIB"
    rm -f test_runner.o test_runner
}

# call_cxx input output
call_cxx() {
    echo "CXX: $1"
    eval "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} -c -o ${2} ${1}"
}

# call_as input output
call_as() {
    echo "AS: src/asm/${1}"
    eval "${CPP} -x assembler-with-cpp \"src/asm/${1}\" | ${AS} -o \"${2}\""
}

# call_ld output file1 file2 ...
call_ld() {
    echo "LD: $1"
    eval "${CXX} ${OSTD_CPPFLAGS} ${OSTD_CXXFLAGS} ${OSTD_LDFLAGS} -o $@"
}

# call_ldlib output file1 file2 ...
call_ldlib() {
    echo "AR: $1"
    eval "${AR} rcs $@"
}

# build_example name
build_example() {
    call_cxx "examples/${1}.cc" "examples/${1}.o"
    call_ld "examples/${1}" "examples/${1}.o" "$ASM_LIB"
    rm "examples/${1}.o"
}

# build test runner
build_test_runner() {
    call_cxx test_runner.cc test_runner.o
    call_ld test_runner test_runner.o
    rm test_runner.o
}

# add_ext str ext
add_ext() {
    RET=""
    for it in $1; do
        RET="$RET ${it}${2}"
    done
    echo $RET
}

# check if cleaning
if [ "$1" = "clean" ]; then
    clean
    exit 0
fi

# build test runner
echo "Building test runner..."
build_test_runner &

# build assembly
echo "Running assembler..."
for as in $ASM_SOURCES; do
    call_as "${as}.S" "${as}.o" &
done
wait
call_ldlib "$ASM_LIB" $(add_ext "$ASM_SOURCES" ".o")
rm $(add_ext "$ASM_SOURCES" ".o")

# build examples
echo "Building examples..."
for ex in $EXAMPLES; do
    build_example "$ex" &
done
wait

# done
exit 0
