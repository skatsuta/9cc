// Define _POSIX_C_SOURCE suppress the warning against use of `strndup()`
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// token.c
//

// Kind of tokens
typedef enum {
  TK_RESERVED, // Symbol
  TK_IDENT,    // Identifier
  TK_NUM,      // Integer token
  TK_EOF,      // Token representing the end of input
} TokenKind;

// Type of tokens
typedef struct Token Token;
struct Token {
  TokenKind kind; // Type of a token
  Token *next;    // Next token
  int val;        // Value of a token if its kind is TK_NUM
  char *str;      // String of a token
  int len;        // Length of a token
};

// Type of local variables
typedef struct Var Var;
struct Var {
  Var *next;  // Next variable or NULL
  char *name; // Name of a variable
  int offset; // Offset from RBP (base pointer)
};

void error(char *fmt, ...);
bool consume(char *op);
Token *consume_ident();
void expect(char *op);
int expect_number();
char *expect_ident();
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

// Kind of nodes in an abstract syntax tree (AST)
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_EXPR_STMT, // Expression statement
  ND_BLOCK,     // Block (compound) statement { ... }
  ND_CALL,      // Function call
  ND_VAR,       // Variable
  ND_NUM,       // Integer
} NodeKind;

// Type of nodes in an abstract syntax tree (AST)
typedef struct Node Node;
struct Node {
  NodeKind kind; // Kind of a node
  Node *next;    // Next node

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  // "if", "while" or "for" statement
  Node *cond; // Condition in "if", "while" or "for"
  Node *cons; // Consequence in "if", "whle" or "for"
  Node *alt;  // Alternative in "if"
  Node *init; // Initialization in "for"
  Node *updt; // Update in "for"

  // Block or function body
  Node *body;

  // Function call or definition
  char *func_name; // Function name
  Node *args;      // Function arguments

  Var *var; // Variable itself if kind is ND_VAR
  long val; // Value of an integer if kind is ND_NUM
};

// Type of functions
typedef struct Function Function;
struct Function {
  char *name;     // Name of a function
  Node *node;     // The first statement in a function
  Var *locals;    // Local variables
  int stack_size; // Stack size

  Function *next; // Next function
};

Function *program();

//
// codegen.c
//

void codegen(Function *prog);
