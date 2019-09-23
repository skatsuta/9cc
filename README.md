# 9cc

Small C compiler designed in https://www.sigbus.info/compilerbook (Japanese).


## Development on macOS

This repository is being developed on macOS, while the above book explains how to write a compiler for Linux. To solve this mismatch, Docker is used to emulate compilation and testing for Linux on macOS.

This repository's `Makefile` is configured to support this situation, and if your OS is macOS and you have Docker for Mac installed, `make test` automatically runs tests inside a Linux container where GCC is installed.

```bash
$ sw_vers
ProductName:    Mac OS X
ProductVersion: 10.13.6
BuildVersion:   17G8030

$ make test
docker container run \
                --rm \
                --mount type=bind,source=/Users/skatsuta/src/github.com/skatsuta/9cc,target=/src,consistency=delegated \
                --workdir /src \
                gcc make test
cc -std=c11 -g -static   -c -o token.o token.c
cc -std=c11 -g -static   -c -o main.o main.c
cc -std=c11 -g -static   -c -o parse.o parse.c
cc -std=c11 -g -static   -c -o codegen.o codegen.c
cc -o 9cc token.o main.o parse.o codegen.o
./test.sh
...(snip)...
OK
```


## Currently supported syntax of C

Currently this compiler supports the following subset of C language syntax:

```
program       = (global-var | function)*
basetype      = ("char" | "int" | struct-decl | typedef-name) "*"*
struct-decl   = "struct" ident
              | "struct" ident? "{" struct-member* "}"
struct-member = basetype ident ("[" num "]")* ";"
global-var    = basetype ident ("[" num "]")* ";"
function      = basetype ident "(" params? ")" "{" stmt* "}"
params        = param ("," param)*
param         = basetype ident
stmt          = "if" "(" expr ")" stmt ("else" stmt)?
              | "while" "(" expr ")" stmt
              | "for" "(" expr ";" expr ";" expr ")" stmt
              | "return" expr ";"
              | "{" stmt* "}"
              | "typedef" basetype ident ("[" num "]")* ";"
              | declaration
              | expr ";"
declaration   = basetype ident ("[" num "]")* ("=" assign)? ";"
              | basetype ";"
expr          = assign
assign        = equality ("=" assign)?
equality      = relational ("==" relational | "!=" relational)*
relational    = add ("<" add | "<=" add | ">" add | ">=" add)*
add           = mul ("+" mul | "-" mul)*
mul           = unary ("*" unary | "/" unary)*
unary         = ("+" | "-" | "&" | "*" | "sizeof")? unary
              | postfix
postfix       = primary ("[" expr "]" | "." ident | "->" ident)*
primary       = stmt-expr
              | "(" expr ")"
              | ident func-args?
              | str
              | num
stmt-expr     = "(" "{" stmt stmt* "}" ")"
func-args     = "(" (assign ("," assign)*)? ")"
```
