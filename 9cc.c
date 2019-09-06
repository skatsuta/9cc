#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Kinds of tokens
typedef enum {
  TK_RESERVED, // Symbol
  TK_NUM, // Integer token
  TK_EOF, // Token representing the end of input
} TokenKind;

typedef struct Token Token;

// Token type
struct Token {
  TokenKind kind; // Type of a token
  Token *next; // Next token
  int val; // Value of a token if its kind is TK_NUM
  char *str; // String of a token
};

// Current token
Token *token;

// Prints an error.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// User input program
char *user_input;

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s^ ", pos, ""); // Print leading whitespaces
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Advances to a next token and returns true if the next token is an expected symbol,
// otherwise false.
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    return false;
  }
  token = token->next;
  return true;
}

// Advances to a next token if the next token is an expected symbol,
// otherwise reports an error.
void expect(char op) {
  char c = token->str[0];
  if (token->kind != TK_RESERVED || c != op) {
    error_at(token->str, "Expected '%c', but got '%c'.", op, c);
  }
  token = token->next;
}

// Advances to a next token if the next token is an integer, otherwise reports an error.
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "Expected an integer, but got a non-integer.");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// Creates a new token and links it to the current token `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// Tokenizes an input string `p` and returns the first token.
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "Could not tokenize the string.");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "The number of arguments is invalid.\n");
    return 1;
  }

  // Store user input for error reporting
  user_input = argv[1];

  // Tokenize
  token = tokenize(user_input);

  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // The first token must be a number, so check it and print the first mov operation
  printf("  mov rax, %d\n", expect_number());

  // Consume `+ <num>` or `- <num>` tokens and output assembly lines
  while (!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    }

    expect('-');
    printf("  sub rax, %d\n", expect_number());
  }

  printf("  ret\n");
  return 0;
}
