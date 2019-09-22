#include "9cc.h"

char *arg_regs_1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *arg_regs_8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Global sequence number which is used for jump labels
int label_seq = 1;
char *func_name;

void store(Type *type) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (type->size == 1) {
    printf("  mov [rax], dil\n");
  } else {
    printf("  mov [rax], rdi\n");
  }
  printf("  push rdi\n");
}

void load(Type *type) {
  printf("  pop rax\n");
  if (type->size == 1) {
    printf("  movsx rax, byte ptr [rax]\n");
  } else {
    printf("  mov rax, [rax]\n");
  }
  printf("  push rax\n");
}

void gen(Node *node);

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    Var *var = node->var;
    if (var->is_local) {
      printf("  lea rax, [rbp-%d]\n", var->offset);
      printf("  push rax\n");
    } else {
      printf("  push offset %s\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen(node->lhs);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs);
    printf("  pop rax\n");
    printf("  add rax, %d\n", node->member->offset);
    printf("  push rax\n");
    return;
  default:
    error_tok(node->tok, "not a lvalue");
  }
}

void gen_lval(Node *node) {
  if (node->type->kind == TYPE_ARRAY) {
    error_tok(node->tok, "not a lvalue");
  }
  gen_addr(node);
}

// Generate code for a given node.
void gen(Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    // Push the value to the top of the stack
    printf("  push %ld\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    // Discard the result value at the top of the stack
    printf("  add rsp, 8\n");
    return;
  case ND_VAR:
  case ND_MEMBER:
    gen_addr(node);
    if (node->type->kind != TYPE_ARRAY) {
      load(node->type);
    }
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    store(node->type);
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->type->kind != TYPE_ARRAY) {
      load(node->type);
    }
    return;
  case ND_IF: {
    int seq = label_seq;
    label_seq++;
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    if (node->alt) {
      printf("  je .L.else.%d\n", seq);
      gen(node->cons);
      printf("  jmp .L.end.%d\n", seq);
      printf(".L.else.%d:\n", seq);
      gen(node->alt);
    } else {
      printf("  je .L.end.%d\n", seq);
      gen(node->cons);
    }
    printf(".L.end.%d:\n", seq);
    return;
  }
  case ND_WHILE: {
    int seq = label_seq;
    label_seq++;
    printf(".L.begin.%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.end.%d\n", seq);
    gen(node->cons);
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.end.%d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = label_seq;
    label_seq++;
    if (node->init) {
      gen(node->init);
    }
    printf(".L.begin.%d:\n", seq);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .L.end.%d\n", seq);
    }
    gen(node->cons);
    if (node->updt) {
      gen(node->updt);
    }
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.end.%d:\n", seq);
    return;
  }
  case ND_BLOCK:
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      gen(n);
    }
    return;
  case ND_CALL: {
    // Push arguments onto the stack
    int n_args = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen(arg);
      n_args++;
    }

    // Set arguments in reverse order
    for (int i = n_args - 1; i >= 0; i--) {
      printf("  pop %s\n", arg_regs_8[i]);
    }

    // According to x86-64 ABI, RSP must be aligned to a 16 byte boundary before
    // calling a function.
    // RAX is set to zero for a variadic function.
    int seq = label_seq;
    label_seq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", seq);
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->func_name);
    printf("  jmp .L.end.%d\n", seq);
    printf(".L.call.%d:\n", seq);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->func_name);
    printf("  add rsp, 8\n");
    printf(".L.end.%d:\n", seq);
    printf("  push rax\n");

    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", func_name);
    return;
  default:
    // This section is meaningless but added to suppress -Wswitch compiler
    // warning
    break;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_PTR_ADD:
    printf("  imul rdi, %d\n", node->type->base->size);
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
    printf("  imul rdi, %d\n", node->type->base->size);
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_DIFF:
    printf("  sub rax, rdi\n");
    printf("  cqo\n");
    printf("  mov rdi, %d\n", node->lhs->type->size);
    printf("  idiv rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzx rax, al\n");
    break;
  default:
    fprintf(stderr, "unexpected node kind: %d", node->kind);
  }

  printf("  push rax\n");
}

void load_arg(Var *var, int idx) {
  char *reg = var->type->size == 1 ? arg_regs_1[idx] : arg_regs_8[idx];
  printf("  mov [rbp-%d], %s\n", var->offset, reg);
}

// Emits text segment.
void emit_text(Program *prog) {
  printf(".text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    func_name = fn->name;

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    // Push arguments onto the stack
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next) {
      load_arg(vl->var, i);
      i++;
    }

    // Emit assembly code of function body statements
    for (Node *node = fn->node; node; node = node->next) {
      gen(node);
    }

    // Epilogue
    printf(".L.return.%s:\n", func_name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}

// Emits data segment.
void emit_data(Program *prog) {
  printf(".data\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf("%s:\n", var->name);

    if (!var->contents) {
      printf("  .zero %d\n", var->type->size);
      continue;
    }

    for (int i = 0; i < var->cont_len; i++) {
      printf("  .byte %d\n", var->contents[i]);
    }
  }
}

void codegen(Program *prog) {
  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");
  emit_data(prog);
  emit_text(prog);
}
