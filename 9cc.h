#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// token.c
//

// Kinds of tokens
typedef enum {
  TK_RESERVED, // Symbol
  TK_NUM,      // Integer token
  TK_EOF,      // Token representing the end of input
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Type of a token
  Token *next;    // Next token
  int val;        // Value of a token if its kind is TK_NUM
  char *str;      // String of a token
  int len;        // Length of a token
};

void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

// Kind of a node in an abstract syntax tree (AST)
typedef enum {
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LT,     // <
  ND_LE,     // <=
  ND_RETURN, // "return"
  ND_NUM,    // Integer
} NodeKind;

// Class of a node in an abstract syntax tree (AST)
typedef struct Node Node;
struct Node {
  NodeKind kind; // Kind of a node
  Node *next;    // Next node
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  long val;      // Value of an integer if kind is ND_NUM
};

Node *program();

//
// codegen.c
//

void codegen(Node *node);
