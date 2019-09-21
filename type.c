#include "9cc.h"

Type *char_type = &(Type){.kind = TYPE_CHAR, .size = 1};
Type *int_type = &(Type){.kind = TYPE_INT, .size = 8};

bool is_integer(Type *type) {
  return type->kind == TYPE_CHAR || type->kind == TYPE_INT;
}

Type *pointer_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = TYPE_PTR;
  type->size = 8;
  type->base = base;
  return type;
}

Type *array_of(Type *base, int len) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = TYPE_ARRAY;
  type->size = base->size * len;
  type->base = base;
  type->array_len = len;
  return type;
}

void add_type(Node *node) {
  if (!node || node->type) {
    return;
  }

  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->cons);
  add_type(node->alt);
  add_type(node->init);
  add_type(node->updt);

  for (Node *n = node->body; n; n = n->next) {
    add_type(n);
  }
  for (Node *n = node->args; n; n = n->next) {
    add_type(n);
  }

  switch (node->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_PTR_DIFF:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_CALL:
  case ND_NUM:
    node->type = int_type;
    return;
  case ND_PTR_ADD:
  case ND_PTR_SUB:
  case ND_ASSIGN:
    node->type = node->lhs->type;
    return;
  case ND_VAR:
    node->type = node->var->type;
    return;
  case ND_ADDR:
    if (node->lhs->type->kind == TYPE_ARRAY) {
      node->type = pointer_to(node->lhs->type->base);
    } else {
      node->type = pointer_to(node->lhs->type);
    }
    return;
  case ND_DEREF:
    if (!node->lhs->type->base) {
      error_tok(node->tok, "invalid pointer dereference");
    }
    node->type = node->lhs->type->base;
    return;
  default:
    // This section is meaningless but added to suppress -Wswitch compiler
    // warning
    break;
  }
}
