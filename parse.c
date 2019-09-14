#include "9cc.h"

// All local variable instances created during parsing are accumulated to this
// list.
VarList *locals;

// Finds a local variable by name. If the name is not found in local variables,
// it returns NULL.
Var *find_var(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len &&
        !strncmp(tok->str, var->name, tok->len)) {
      return var;
    }
  }
  return NULL;
}

Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = expr;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num(int val, Token *tok) {
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

// Creates a list of variables.
VarList *new_var_list(Var *var) {
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  return vl;
}

// Creates a new local variable with the given name.
Var *new_var(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;

  VarList *vl = new_var_list(var);
  vl->next = locals;
  locals = vl;
  return var;
}

// Creates a new local variable node with the given name.
Node *new_var_node(Var *var, Token *tok) {
  Node *node = new_node(ND_VAR, tok);
  node->var = var;
  return node;
}

bool at_eof(void) { return token->kind == TK_EOF; }

// Function declarations
Function *program();
Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *func_args();

// program = function*
Function *program() {
  Function head = {};
  Function *cur = &head;

  while (!at_eof()) {
    cur->next = function();
    cur = cur->next;
  }

  return head.next;
}

// Reads function parameters and returns a list of them.
VarList *read_func_params() {
  if (consume(")")) {
    return NULL;
  }

  VarList *head = new_var_list(new_var(expect_ident()));
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = new_var_list(new_var(expect_ident()));
    cur = cur->next;
  }

  return head;
}

// function = ident "(" params? ")" "{" stmt* "}"
// params   = ident ("," ident)*
Function *function() {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params();
  expect("{");

  Node head = {};
  Node *cur = &head;
  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// Parses an expression statement and creates a new ND_EXPR_STMT node.
Node *read_expr_stmt() {
  Token *tok = token;
  return new_unary(ND_EXPR_STMT, expr(), tok);
}

// stmt = "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr ";" expr ";" expr ")" stmt
//      | "return" expr ";"
//      | "{" stmt* "}"
//      | expr ";"
Node *stmt() {
  Token *tok;

  // Parse "if"-"else" statement
  if ((tok = consume("if"))) {
    Node *node = new_node(ND_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->cons = stmt();
    if (consume("else")) {
      node->alt = stmt();
    }
    return node;
  }

  // Parse "while" statement
  if ((tok = consume("while"))) {
    Node *node = new_node(ND_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->cons = stmt();
    return node;
  }

  // Parse "for" statement
  if ((tok = consume("for"))) {
    Node *node = new_node(ND_FOR, tok);
    expect("(");
    if (!consume(";")) {
      node->init = read_expr_stmt();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->updt = read_expr_stmt();
      expect(")");
    }
    node->cons = stmt();
    return node;
  }

  // Parse "return" statement
  if ((tok = consume("return"))) {
    Node *node = new_unary(ND_RETURN, expr(), tok);
    expect(";");
    return node;
  }

  // Parse block (compound) statement
  if ((tok = consume("{"))) {
    Node head = {};
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
  }

  // Parse expression statement
  Node *node = read_expr_stmt();
  expect(";");
  return node;
}

// expr = assign
Node *expr() { return assign(); }

// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  Token *tok = consume("=");
  if (tok) {
    node = new_binary(ND_ASSIGN, node, assign(), tok);
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();
  Token *tok;

  for (;;) {
    if ((tok = consume("=="))) {
      node = new_binary(ND_EQ, node, relational(), tok);
    } else if ((tok = consume("!="))) {
      node = new_binary(ND_NE, node, relational(), tok);
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();
  Token *tok;

  for (;;) {
    if ((tok = consume("<"))) {
      node = new_binary(ND_LT, node, add(), tok);
    } else if ((tok = consume("<="))) {
      node = new_binary(ND_LE, node, add(), tok);
    } else if ((tok = consume(">"))) {
      node = new_binary(ND_LT, add(), node, tok);
    } else if ((tok = consume(">="))) {
      node = new_binary(ND_LE, add(), node, tok);
    } else {
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if ((tok = consume("+"))) {
      node = new_binary(ND_ADD, node, mul(), tok);
    } else if ((tok = consume("-"))) {
      node = new_binary(ND_SUB, node, mul(), tok);
    } else {
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();
  Token *tok;

  for (;;) {
    if ((tok = consume("*"))) {
      node = new_binary(ND_MUL, node, unary(), tok);
    } else if ((tok = consume("/"))) {
      node = new_binary(ND_DIV, node, unary(), tok);
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-")? unary | "&" unary | "*" unary | primary
Node *unary() {
  Token *tok;
  if ((tok = consume("+"))) {
    return unary();
  } else if ((tok = consume("-"))) {
    return new_binary(ND_SUB, new_num(0, tok), unary(), tok);
  } else if ((tok = consume("&"))) {
    return new_unary(ND_ADDR, unary(), tok);
  } else if ((tok = consume("*"))) {
    return new_unary(ND_DEREF, unary(), tok);
  } else {
    return primary();
  }
}

// primary = "(" expr ")" | ident func-args? | num
Node *primary() {
  // Assume "(" expr ")" if next token is "("
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok;

  // Consume if the token is an identifier
  if ((tok = consume_ident())) {
    // Parse a function call
    if (consume("(")) {
      Node *node = new_node(ND_CALL, tok);
      node->func_name = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    // Parse a variable
    Var *var = find_var(tok);
    if (!var) {
      var = new_var(strndup(tok->str, tok->len));
    }
    return new_var_node(var, tok);
  }

  tok = token;
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected expression");
  }
  return new_num(expect_number(), tok);
}

// func-args = "(" (assign ("," assign)*)? ")"
Node *func_args() {
  if (consume(")")) {
    return NULL;
  }

  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }

  expect(")");
  return head;
}
