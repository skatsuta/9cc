#include "9cc.h"

// User input source code
char *user_input;

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

    if (ispunct(*p)) {
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
