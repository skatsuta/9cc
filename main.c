#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  // Tokenize and parse input
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  // Output the header of assembly code
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Generate assembly with traversing the AST
  gen(node);

  // A result must be at the top of the stack, so pop it to RAX to make it a program exit code
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
