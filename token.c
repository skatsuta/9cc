#include "9cc.h"

// Source code file name
char *filename;
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

// Reports an error message in the following format and exit.
//
// file.c:10: x = y + 1;
//                ^ <error message here>
void verror_at(char *loc, char *fmt, va_list ap) {
  // Find a line containing `loc`
  char *line = loc;
  while (user_input < line && line[-1] != '\n') {
    line--;
  }

  char *end = loc;
  while (*end != '\n') {
    end++;
  }

  // Get a line number
  int line_num = 1;
  for (char *p = user_input; p < line; p++) {
    if (*p == '\n') {
      line_num++;
    }
  }

  // Print out the line
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // Show the error message
  int pos = loc - line + indent;
  fprintf(stderr, "%*s^ ", pos, ""); // Print leading whitespaces
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->str, fmt, ap);
}

// Returns its token if the current token matches a given string s which is a
// reserved keyword or an operator, otherwise returns NULL.
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len ||
      strncmp(token->str, s, token->len)) {
    return NULL;
  }
  return token;
}

// Consumes the current token and returns true if it matches `op`, otherwise
// does nothing and returns false.
Token *consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      strncmp(token->str, op, token->len)) {
    return NULL;
  }

  Token *t = token;
  // Advance the current token
  token = token->next;
  return t;
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
void expect(char *s) {
  if (!peek(s)) {
    error_tok(token, "Expected \"%s\"", s);
  }
  token = token->next;
}

// Returns an integer and advances to a next token if the current token is an
// integer, otherwise reports an error.
int expect_number() {
  if (token->kind != TK_NUM) {
    error_tok(token, "Expected an integer, but got a non-integer.");
  }
  int val = token->val;
  token = token->next;
  return val;
}

// Returns an identifier and advances to a next token if the current token is an
// identifier, otherwise reports an error.
char *expect_ident() {
  if (token->kind != TK_IDENT) {
    error_tok(token, "Expected an integer, but got a non-integer.");
  }
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
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
  char *kw[] = {"return", "if",   "else",   "while",  "for",
                "int",    "char", "struct", "sizeof", "typedef"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (start_with(p, kw[i]) && !is_alphanum(p[len])) {
      return kw[i];
    }
  }

  // Multi-letter punctuators
  char *ops[] = {"==", "!=", "<=", ">=", "->"};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
    if (start_with(p, ops[i])) {
      return ops[i];
    }
  }

  return NULL;
}

char get_escape_char(char c) {
  switch (c) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case 'e':
    return 27;
  case '0':
    return 0;
  default:
    return c;
  }
}

Token *read_string_literal(Token *cur, char *start) {
  char *p = start + 1;
  char buf[1024];
  int len = 0;

  for (;;) {
    if (len == sizeof(buf)) {
      error_at(start, "string literal too large");
    }
    if (*p == '\0') {
      error_at(start, "unclosed string literal");
    }
    if (*p == '"') {
      break;
    }

    if (*p == '\\') {
      p++;
      buf[len] = get_escape_char(*p);
    } else {
      buf[len] = *p;
    }
    len++;
    p++;
  }

  Token *tok = new_token(TK_STR, cur, start, p - start + 1);
  tok->contents = strndup(buf, len);
  tok->cont_len = len + 1;
  return tok;
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

    // Skip a line comment
    if (start_with(p, "//")) {
      p += 2;
      while (*p != '\n') {
        p++;
      }
      continue;
    }

    // Skip a block comment
    if (start_with(p, "/*")) {
      char *start = strstr(p + 2, "*/");
      if (!start) {
        error_at(p, "unclosed block comment");
      }
      p = start + 2;
      continue;
    }

    // String literals
    if (*p == '"') {
      cur = read_string_literal(cur, p);
      p += cur->len;
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
