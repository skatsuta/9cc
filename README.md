# 9cc

Small C compiler designed in https://www.sigbus.info/compilerbook (Japanese).


## Development on macOS

This repository is being developed on macOS, while the above book explains a compiler for Linux. To solve this mismatch, Docker is used to emulate compilation and testing for Linux on macOS.

This repository's `Makefile` is configured for supporting this situation, and if your OS is macOS and you have Docker for Mac installed, `make test` runs tests on a Linux container where GCC is installed.

```bash
$ make test
docker container run \
                --rm \
                --mount type=bind,source=/Users/skatsuta/src/github.com/skatsuta/9cc,target=/src,consistency=delegated \
                -w /src gcc make test
cc -std=c11 -g -static   -c -o token.o token.c
cc -std=c11 -g -static   -c -o main.o main.c
cc -std=c11 -g -static   -c -o parse.o parse.c
cc -std=c11 -g -static   -c -o codegen.o codegen.c
cc -o 9cc token.o main.o parse.o codegen.o
./test.sh
...(snip)...
OK
```


## Current status of supported syntax

The current status of supported syntax in C is as follows:

```
program    = stmt*
stmt       = expr ";"
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? unary | primary
primary    = "(" expr ")" | num
```
