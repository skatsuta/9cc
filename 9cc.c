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

// Kind of a node in an abstract syntax tree (AST)
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integers
} NodeKind;

typedef struct Node Node;

// Class of a node in an abstract syntax tree (AST)
struct Node {
  NodeKind kind; // Kind of a node
  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side
  int val; // Value of an integer if kind is ND_NUM
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *mul();
Node *primary();

Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume('+')) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume('-')) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = primary();

  for (;;) {
    if (consume('*')) {
      node = new_node(ND_MUL, node, primary());
    } else if (consume('/')) {
      node = new_node(ND_DIV, node, primary());
    } else {
      return node;
    }
  }
}

Node *primary() {
  // Assume "(" expr ")" if the next token is "("
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // Otherwise it should be an integer
  return new_node_num(expect_number());
}

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    default:
      error("unexpected kind of node: %d", node->kind);
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "The number of arguments is invalid.\n");
    return 1;
  }

  // Tokenize and parse input
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Generate code traversing the AST
  gen(node);

  // The top of stack should be an evaluated value of the given expression,
  // so load it to RAX and make it the return value.
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
