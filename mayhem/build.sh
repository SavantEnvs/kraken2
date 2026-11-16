#!/usr/bin/env bash
#
# mayhem/build.sh: builds the Kraken 2 fuzz targets and the oracle (the upstream install the
# functional suite tests/e2e.sh runs against). Runs inside the commit image as `mayhem` in /mayhem.
#
#   fuzz_fast_reader     libFuzzer   (+ fuzz_fast_reader-standalone reproducer)
#   fuzz_ncbi_taxonomy   AFL         (the binary is its own reproducer)
#   fuzz_taxonomy_k2d    AFL
#   fuzz_compact_hash    AFL
#   mayhem/.oracle/      normal-flag install of upstream (gitignored), used by test.sh
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC="${SRC:-/mayhem}"
cd "$SRC"
B="$SRC/mayhem/.build"
mkdir -p "$B"/{fuzz,afl}

# The fuzzed libraries are the upstream sources compiled directly (the upstream Makefile builds in
# tree and always links OpenMP, which the base lacks for clang; src/omp_hack.cc stands in for it
# when _OPENMP is not defined, exactly as upstream intends).
LIBSRC="fast_reader mmscanner aa_translate taxonomy mmap_file omp_hack"
CXXFLAGS_COMMON="-std=c++17 -O2 -DLINEAR_PROBING -Wall -Wno-unused-function"

# 1) libFuzzer variant: project objects with SanCov (no-link), harness once with the engine and
#    once with the standalone driver.
fuzz_objs=()
for f in $LIBSRC; do
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link $CXXFLAGS_COMMON \
      -c "src/$f.cc" -o "$B/fuzz/$f.o"
  fuzz_objs+=("$B/fuzz/$f.o")
done
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$B/fuzz/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/fuzz/standalone_main.o"

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link $CXXFLAGS_COMMON \
    -c mayhem/fuzz_fast_reader.cc -o "$B/fuzz/fuzz_fast_reader.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
    "$B/fuzz/fuzz_fast_reader.o" "${fuzz_objs[@]}" "$B/fuzz/lsan_off.o" -o "$SRC/fuzz_fast_reader"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS \
    "$B/fuzz/fuzz_fast_reader.o" "${fuzz_objs[@]}" "$B/fuzz/lsan_off.o" "$B/fuzz/standalone_main.o" \
    -o "$SRC/fuzz_fast_reader-standalone"

# 2) AFL variant: the project and the harnesses with afl-clang-fast++, once.
afl_objs=()
for f in $LIBSRC; do
  afl-clang-fast++ $SANITIZER_FLAGS $DEBUG_FLAGS $CXXFLAGS_COMMON -c "src/$f.cc" -o "$B/afl/$f.o"
  afl_objs+=("$B/afl/$f.o")
done
afl-clang-fast++ $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$B/afl/lsan_off.o"
for t in fuzz_ncbi_taxonomy fuzz_taxonomy_k2d fuzz_compact_hash; do
  afl-clang-fast++ $SANITIZER_FLAGS $DEBUG_FLAGS $CXXFLAGS_COMMON -c "mayhem/$t.cc" -o "$B/afl/$t.o"
  afl-clang-fast++ $SANITIZER_FLAGS $DEBUG_FLAGS "$B/afl/$t.o" "${afl_objs[@]}" "$B/afl/lsan_off.o" \
      -o "$SRC/$t"
done

# 3) The oracle: upstream's own install, normal flags (-O3 -fopenmp with g++/libgomp, as
#    install_kraken2.sh builds it), out of tree. COVERAGE_FLAGS switches to clang without OpenMP,
#    since gcc cannot take clang's profile flags.
O="$SRC/mayhem/.oracle"
rm -rf "$O"
mkdir -p "$O/tree"
cp -r src scripts install_kraken2.sh VERSION "$O/tree/"
if [ -n "$COVERAGE_FLAGS" ]; then
  (cd "$O/tree" && env CXX=clang++ CC=clang KRAKEN2_SKIP_FOPENMP="$COVERAGE_FLAGS" LDFLAGS="$COVERAGE_FLAGS" \
      ./install_kraken2.sh "$O/install")
else
  (cd "$O/tree" && env CXX=g++ CC=gcc ./install_kraken2.sh "$O/install")
fi
