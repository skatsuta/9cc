#include "9cc.h"

char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Global sequence number which is used for jump labels
int label_seq = 1;
char *func_name;

void store() {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

void load() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

void gen(Node *node);

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR:
    printf("  lea rax, [rbp-%d]\n", node->var->offset);
    printf("  push rax\n");
    return;
  case ND_DEREF:
    gen(node->lhs);
    return;
  default:
    error("The left value of assignment is not a variable");
  }
}

// Generate code for a given node.
void gen(Node *node) {
  switch (node->kind) {
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
    gen_addr(node);
    load();
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    gen(node->rhs);
    store();
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    load();
    return;
  case ND_IF: {
    int seq = label_seq;
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
    label_seq++;
    return;
  }
  case ND_WHILE: {
    int seq = label_seq;
    printf(".L.begin.%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .L.end.%d\n", seq);
    gen(node->cons);
    printf("  jmp .L.begin.%d\n", seq);
    printf(".L.end.%d:\n", seq);
    label_seq++;
    return;
  }
  case ND_FOR: {
    int seq = label_seq;
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
    label_seq++;
    return;
  }
  case ND_BLOCK:
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
      printf("  pop %s\n", arg_regs[i]);
    }

    // According to x86-64 ABI, RSP must be aligned to a 16 byte boundary before
    // calling a function.
    // RAX is set to zero for a variadic function.
    int seq = label_seq;
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
    label_seq++;

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
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  default:
    fprintf(stderr, "unexpected node kind: %d", node->kind);
  }

  printf("  push rax\n");
}

void gen_func(Function *fn) {
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
    printf("  mov [rbp-%d], %s\n", vl->var->offset, arg_regs[i]);
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

void codegen(Function *prog) {
  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");

  // Emit assembly code of function definitions
  for (Function *fn = prog; fn; fn = fn->next) {
    gen_func(fn);
  }
}
