#!/bin/bash
set -euo pipefail

CC=gcc
CFLAGS="-Wall -Wextra -Wpedantic -D_GNU_SOURCE -Isrc"
LDFLAGS="-lpthread"
OBJ="obj"
BIN="bin"
SRC="src"
TEST="tests"

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

all) "$0" demo && "$0" test ;;

run)
  [ -f "$BIN/myLogger_demo" ] || "$0" demo
  ./"$BIN/myLogger_demo"
  ;;

clean)
  rm -rf $OBJ $BIN/*
  echo "Cleaned."
  ;;

*)
  echo "Usage: $0 {demo|test|all|run|clean}"
  exit 1
  ;;
esac
