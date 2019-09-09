#!/usr/bin/env bash
set -u

CC=gcc
BIN=9cc
TMP=tmp
TMP2=tmp2

cat <<EOF | ${CC} -xc -c -o ${TMP2}.o -
int ret3() { return 3; }
int ret5() { return 5; }
EOF

assert() {
  local expected="$1"
  local input="$2"

  ./${BIN} "${input}" > ${TMP}.s
  ${CC} -o ${TMP} ${TMP}.s ${TMP2}.o

  ./${TMP}
  local actual="$?"

  if [[ "${actual}" != "${expected}" ]]; then
    echo "${input} => ${expected} expected, but got ${actual}"
    exit 1
  fi
  echo "${input} => ${actual}"
}

# Arithmetic operations
assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 41 'return 12 + 34 - 5 ;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 10 'return -10+20;'
assert 10 'return - -10;'
assert 10 'return - - (-10+20);'

# Equality/Inequality operators
assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

# Multiple statements
assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

# Assignments
assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'
assert 3 'foo=3; return foo;'
assert 6 'foo123 = 1; bar = 2 + 3; return foo123 + bar;'

# Block statements
assert 3 '{1; {2;} return 3;}'

# "if" statements
assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'
assert 3 'if (0) return 2; else return 3; return 4;'
assert 3 'if (1-1) return 2; else return 3; return 4;'
assert 2 'if (1) return 2; else return 3; return 4;'
assert 2 'if (2-1) return 2; else return 3; return 4;'

# "while" statements
assert 10 'i=0; while(i<10) i=i+1; return i;'
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

# "for" statements
assert 55 'i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;'
assert 3 'for (;;) return 3; return 5;'

# Function calls
assert 3 'return ret3();'
assert 5 'return ret5();'

echo 'OK'
