#include "9cc.h"

Type *char_type = &(Type){.kind = TYPE_CHAR, .size = 1, .align = 1};
Type *int_type = &(Type){.kind = TYPE_INT, .size = 8, .align = 8};

bool is_integer(Type *type) {
  return type->kind == TYPE_CHAR || type->kind == TYPE_INT;
}

// Aligns `n` to the multiple of `align`.
//
// How to compute padding is explained at
// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding.
int align_to(int n, int align) { return (n + align - 1) & ~(align - 1); }

Type *new_type(TypeKind kind, int size, int align) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  type->size = size;
  type->align = align;
  return type;
}

Type *pointer_to(Type *base) {
  Type *type = new_type(TYPE_PTR, 8, 8);
  type->base = base;
  return type;
}

Type *array_of(Type *base, int len) {
  Type *type = new_type(TYPE_ARRAY, base->size * len, base->align);
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
  case ND_MEMBER:
    node->type = node->member->type;
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
  case ND_STMT_EXPR: {
    // Get the last statement
    Node *last = node->body;
    while (last->next) {
      last = last->next;
    }

    node->type = last->type;
    return;
  }
  default:
    // This section is meaningless but added to suppress -Wswitch compiler
    // warning
    break;
  }
}
