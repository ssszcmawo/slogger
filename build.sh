#!/bin/bash
set -euo pipefail

CC=gcc
CFLAGS="-Wall -Wextra -Wpedantic -D_GNU_SOURCE -Isrc"
LDFLAGS="-lpthread"
OBJ="obj"
BIN="bin"
SRC="src"
TEST="tests"
EXAMPLES="examples"

mkdir -p $OBJ $BIN

case "${1:-demo}" in
demo)
  $CC $CFLAGS -c $SRC/slogger.c -o $OBJ/slogger.o
  $CC $CFLAGS -c $SRC/zip.c -o $OBJ/zip.o
  $CC $CFLAGS $SRC/main.c $OBJ/slogger.o $OBJ/zip.o $LDFLAGS -o $BIN/myLogger_demo
  echo "Demo ready: $BIN/myLogger_demo"
  ;;

test | tests)
  $CC $CFLAGS -c $SRC/slogger.c -o $OBJ/slogger.o
  $CC $CFLAGS -c $SRC/zip.c -o $OBJ/zip.o
  for t in $TEST/test_*.c; do
    name=$(basename "$t" .c)
    $CC $CFLAGS "$t" $OBJ/slogger.o $OBJ/zip.o $LDFLAGS -o "$BIN/$name"
    echo "Test built: $BIN/$name"
  done
  ;;

examples)
  $CC $CFLAGS -c $SRC/slogger.c -o $OBJ/slogger.o
  $CC $CFLAGS -c $SRC/zip.c -o $OBJ/zip.o
  for e in $EXAMPLES/example_*.c; do
    name=$(basename "$e" .c)
    $CC $CFLAGS "$e" $OBJ/slogger.o $OBJ/zip.o $LDFLAGS -o "$BIN/$name"
    echo "Example built: $BIN/$name"
  done
  ;;

all)
  "$0" demo && "$0" test && "$0" examples
  ;;

run)
  [ -f "$BIN/myLogger_demo" ] || "$0" demo
  ./"$BIN/myLogger_demo"
  ;;

clean)
  rm -rf $OBJ $BIN/*
  echo "Cleaned."
  ;;

*)
  echo "Usage: $0 {demo|test|examples|all|run|clean}"
  exit 1
  ;;
esac
