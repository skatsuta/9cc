#include "9cc.h"

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
