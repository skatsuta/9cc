#!/usr/bin/env bash
set -u

CC=gcc
BIN=9cc
TMP=tmp

try() {
  local expected="$1"
  local input="$2"

  ./${BIN} "${input}" > ${TMP}.s
  ${CC} -o ${TMP} ${TMP}.s
  ./${TMP}
  local actual="$?"

  if [[ "${actual}" != "${expected}" ]]; then
    echo "${input} => ${expected} expected, but got ${actual}"
    exit 1
  fi
  echo "${input} => ${actual}"
}

try 0 0
try 42 42
try 21 "5+20-4"
try 41 " 12 + 34 - 5 "
try 47 "5+6*7"
try 15 "5*(9-6)"
try 4 "(3+5)/2"
try 10 "-10+20"

echo "OK"
