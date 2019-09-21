#include "9cc.h"

// Aligns `n` to the multiple of `align`.
//
// How to compute padding is explained at
// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding.
int align_to(int n, int align) { return (n + align - 1) & ~(align - 1); }

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  // Tokenize and parse input
  user_input = argv[1];
  token = tokenize();
  Program *prog = program();

  // Assign offsets to local variables
  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += var->type->size;
      var->offset = offset;
    }
    fn->stack_size = align_to(offset, 8);
  }

  // Generate assembly with traversing the AST
  codegen(prog);

  return 0;
}
