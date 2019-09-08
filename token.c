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

// Consumes the current token and returns true if it matches `op`, otherwise
// does nothing and returns false.
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      strncmp(token->str, op, token->len)) {
    return false;
  }

  // Advance the current token
  token = token->next;
  return true;
}

// Consumes the current token and returns an identifier if it is an identifier,
// otherwise does nothing and returns NULL.
Token *consume_ident() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Token *tok = token;
  // Advance the current token
  token = token->next;
  return tok;
}

// Advances to a next token if the next token is an expected symbol,
// otherwise reports an error.
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      strncmp(token->str, op, token->len)) {
    error_at(token->str, "Expected \"%s\", but got \"%s\".", op, token->str);
  }
  token = token->next;
}

// Advances to a next token if the next token is an integer, otherwise reports
// an error.
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "Expected an integer, but got a non-integer.");
  }
  int val = token->val;
  token = token->next;
  return val;
}

// Creates a new token and links it to the current token `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool start_with(char *s, char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

bool is_alphanum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

// Reads a reserved symbol or keyword from `p`. If no reserved token is found,
// it returns NULL.
char *read_reserved(char *p) {
  // Keywords
  char *kw[] = {"return", "if", "else", "while", "for"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (start_with(p, kw[i]) && !is_alphanum(p[len])) {
      return kw[i];
    }
  }

  // Multi-letter punctuators
  char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
    if (start_with(p, ops[i])) {
      return ops[i];
    }
  }

  return NULL;
}

// Tokenizes an input string `p` and returns the first token.
Token *tokenize() {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Whitespaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Keywords or multi-letter punctuators
    char *kw = read_reserved(p);
    if (kw) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    // Identifiers
    if (is_alpha(*p)) {
      char *name = p;
      while (is_alphanum(*p)) {
        p++;
      }
      cur = new_token(TK_IDENT, cur, name, p - name);
      continue;
    }

    // Single-letter punctuators
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // Numbers
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *prev = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - prev;
      continue;
    }

    error_at(p, "Could not tokenize the string.");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
