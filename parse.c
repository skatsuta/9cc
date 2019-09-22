#include "9cc.h"

// All local and global variable instances created during parsing are
// accumulated to this list.
VarList *locals;
VarList *globals;
VarList *scope;

// Finds a variable by name. If a variable with the name is not found,
// it returns NULL.
Var *find_var(Token *tok) {
  for (VarList *vl = scope; vl; vl = vl->next) {
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

// Creates a new local or global variable with the given name, based on
// `is_local` flag.
Var *new_var(char *name, Type *type, bool is_local) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->type = type;
  var->is_local = is_local;

  // Add to a list of variables in the current scope
  VarList *sc = new_var_list(var);
  sc->next = scope;
  scope = sc;

  return var;
}

// Creates a new local variable with the given name.
Var *new_local_var(char *name, Type *type) {
  Var *var = new_var(name, type, true);

  VarList *vl = new_var_list(var);
  vl->next = locals;
  locals = vl;
  return var;
}

// Creates a new global variable with the given name.
Var *new_global_var(char *name, Type *type) {
  Var *var = new_var(name, type, false);

  VarList *vl = new_var_list(var);
  vl->next = globals;
  globals = vl;
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
Function *function();
Type *basetype();
void global_var();
Node *stmt();
Node *stmt_inner();
Node *declaration();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();
Node *stmt_expr();
Node *func_args();

// Determines whether the next top-level item is a function or a global variable
// by looking ahead input tokens.
bool is_function() {
  Token *tok = token;
  basetype();
  bool is_func = consume_ident() && consume("(");
  token = tok;
  return is_func;
}

// program = (global-var | function)*
Program *program() {
  Function head = {};
  Function *cur = &head;
  globals = NULL;

  while (!at_eof()) {
    if (is_function()) {
      cur->next = function();
      cur = cur->next;
    } else {
      global_var();
    }
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->globals = globals;
  prog->fns = head.next;
  return prog;
}

// basetype = ("char" | "int") "*"*
Type *basetype() {
  Type *type;
  if (consume("char")) {
    type = char_type;
  } else {
    expect("int");
    type = int_type;
  }

  while (consume("*")) {
    type = pointer_to(type);
  }
  return type;
}

Type *read_type_suffix(Type *base) {
  if (!consume("[")) {
    return base;
  }

  // Parse array declaration
  int size = expect_number();
  expect("]");
  base = read_type_suffix(base);
  return array_of(base, size);
}

// global-var = basetype ident ("[" num "]")* ";"
void global_var() {
  Type *type = basetype();
  char *name = expect_ident();
  type = read_type_suffix(type);
  expect(";");
  new_global_var(name, type);
}

VarList *read_func_param() {
  Type *type = basetype();
  char *name = expect_ident();
  type = read_type_suffix(type);
  return new_var_list(new_local_var(name, type));
}

// Reads function parameters and returns a list of them.
VarList *read_func_params() {
  if (consume(")")) {
    return NULL;
  }

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

// function = basetype ident "(" params? ")" "{" stmt* "}"
// params   = param ("," param)*
// param    = basetype ident
Function *function() {
  // Initialie a list of local variables
  locals = NULL;

  // Start parsing a function
  basetype();
  char *name = expect_ident();

  // Parse function arguments
  expect("(");
  VarList *sc = scope;
  VarList *params = read_func_params();

  // Parse function body
  expect("{");
  Node head = {};
  Node *cur = &head;
  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  scope = sc;

  // Create a Function node
  Function *fn = calloc(1, sizeof(Function));
  fn->name = name;
  fn->params = params;
  fn->node = head.next;
  fn->locals = locals;

  return fn;
}

// Parses an expression statement and creates a new ND_EXPR_STMT node.
Node *read_expr_stmt() {
  Token *tok = token;
  return new_unary(ND_EXPR_STMT, expr(), tok);
}

Node *stmt() {
  Node *node = stmt_inner();
  add_type(node);
  return node;
}

// Returns true if the next token represents a type.
bool is_type_name() { return peek("char") || peek("int"); }

// stmt = "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr ";" expr ";" expr ")" stmt
//      | "return" expr ";"
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
Node *stmt_inner() {
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

    VarList *sc = scope;
    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }
    scope = sc;

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
  }

  if (is_type_name()) {
    return declaration();
  }

  // Parse expression statement
  Node *node = read_expr_stmt();
  expect(";");
  return node;
}

// declaration = basetype ident ("[" num "]")* ("=" expr)? ";"
Node *declaration() {
  Token *tok = token;
  Type *type = basetype();
  char *name = expect_ident();
  type = read_type_suffix(type);
  Var *var = new_local_var(name, type);

  if (consume(";")) {
    return new_node(ND_NULL, tok);
  }

  expect("=");
  Node *lhs = new_var_node(var, tok);
  Node *rhs = expr();
  expect(";");
  Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
  return new_unary(ND_EXPR_STMT, node, tok);
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

Node *new_add(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    return new_binary(ND_ADD, lhs, rhs, tok);
  } else if (lhs->type->base && is_integer(rhs->type)) {
    return new_binary(ND_PTR_ADD, lhs, rhs, tok);
  } else if (is_integer(lhs->type) && rhs->type->base) {
    return new_binary(ND_PTR_ADD, rhs, lhs, tok);
  }

  error_tok(tok, "invalid operands");
  // Never reach here
  return NULL;
}

Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    return new_binary(ND_SUB, lhs, rhs, tok);
  } else if (lhs->type->base && is_integer(rhs->type)) {
    return new_binary(ND_PTR_SUB, lhs, rhs, tok);
  } else if (lhs->type->base && rhs->type->base) {
    return new_binary(ND_PTR_DIFF, lhs, rhs, tok);
  }

  error_tok(tok, "invalid operands");
  // Never reach here
  return NULL;
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if ((tok = consume("+"))) {
      node = new_add(node, mul(), tok);
    } else if ((tok = consume("-"))) {
      node = new_sub(node, mul(), tok);
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

// unary = ("+" | "-" | "&" | "*" | "sizeof")? unary
//       | postfix
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
  } else if ((tok = consume("sizeof"))) {
    Node *node = unary();
    add_type(node);
    return new_num(node->type->size, tok);
  } else {
    return postfix();
  }
}

// postfix = primary ("[" expr "]")*
Node *postfix() {
  Node *node = primary();
  Token *tok;

  while ((tok = consume("["))) {
    // x[y] is short for *(x+y)
    Node *idx = new_add(node, expr(), tok);
    expect("]");
    node = new_unary(ND_DEREF, idx, tok);
  }

  return node;
}

char *new_label() {
  // Since it's a static variable, it's incremented with every function call
  static int cnt = 0;
  char buf[20];
  sprintf(buf, ".L.data.%d", cnt);
  cnt++;
  return strndup(buf, 20);
}

// primary = stmt-expr
//         | "(" expr ")"
//         | ident func-args?
//         | str
//         | num
Node *primary() {
  Token *tok;

  // Assume "(" expr ")" if next token is "("
  if ((tok = consume("("))) {
    if (consume("{")) {
      return stmt_expr(tok);
    }

    Node *node = expr();
    expect(")");
    return node;
  }

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
      error_tok(tok, "undefined variable");
    }
    return new_var_node(var, tok);
  }

  tok = token;
  if (tok->kind == TK_STR) {
    token = token->next;

    Type *type = array_of(char_type, tok->cont_len);
    Var *var = new_global_var(new_label(), type);
    var->contents = tok->contents;
    var->cont_len = tok->cont_len;
    return new_var_node(var, tok);
  }

  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected expression");
  }

  return new_num(expect_number(), tok);
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
//
// Statement expression is a GNU C extension.
Node *stmt_expr(Token *tok) {
  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = stmt();
  Node *cur = node->body;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  expect(")");

  if (cur->kind != ND_EXPR_STMT) {
    error_tok(cur->tok, "statement expression returning void is not supported");
  }
  memcpy(cur, cur->lhs, sizeof(Node));
  return node;
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
