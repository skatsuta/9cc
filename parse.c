#include "9cc.h"

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

bool at_eof(void) { return token->kind == TK_EOF; }

// Function declarations
Node *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = stmt*
Node *program() {
  Node head = {};
  Node *cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  return head.next;
}

// stmt = ("return")? expr ";"
Node *stmt() {
  NodeKind kind = consume("return") ? ND_RETURN : ND_EXPR_STMT;
  Node *node = new_unary(kind, expr());
  expect(";");
  return node;
}

// expr = equality
Node *expr() { return equality(); }

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_binary(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_binary(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<")) {
      node = new_binary(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_binary(ND_LE, node, add());
    } else if (consume(">")) {
      node = new_binary(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_binary(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_binary(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_binary(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_binary(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_binary(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-")? unary | primary
Node *unary() {
  if (consume("+")) {
    return unary();
  } else if (consume("-")) {
    return new_binary(ND_SUB, new_num(0), unary());
  } else {
    return primary();
  }
}

// primary = "(" expr ")" | num
Node *primary() {
  // Assume "(" expr ")" if next token is "("
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // Otherwise it should be an integer
  return new_num(expect_number());
}
