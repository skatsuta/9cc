#include "9cc.h"

// Kind of a node in an abstract syntax tree (AST)
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integers
} NodeKind;

// Class of a node in an abstract syntax tree (AST)
typedef struct Node Node;
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
      fprintf(stderr, "unexpected kind of node: %d", node->kind);
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
