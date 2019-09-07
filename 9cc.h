#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//
// token.c
//

// Kinds of tokens
typedef enum {
  TK_RESERVED, // Symbol
  TK_NUM, // Integer token
  TK_EOF, // Token representing the end of input
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Type of a token
  Token *next; // Next token
  int val; // Value of a token if its kind is TK_NUM
  char *str; // String of a token
};

bool consume(char op);
void expect(char op);
int expect_number();
Token *tokenize(char *p);

extern char *user_input;
extern Token *token;
