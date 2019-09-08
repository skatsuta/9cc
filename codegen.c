#include "9cc.h"

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    // Push the value to the top of the stack
    printf("  push %ld\n", node->val);
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  ret\n");
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    // Discard the result value at the top of the stack
    printf("  add rsp, 8\n");
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

void codegen(Node *node) {
  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  for (Node *n = node; n; n = n->next) {
    gen(n);
  }

  printf("  ret\n");
}
