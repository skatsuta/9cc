#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  // Tokenize and parse input
  user_input = argv[1];
  token = tokenize();
  Function *prog = program();

  // Assign offsets to local variables
  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += var->type->size;
      var->offset = offset;
    }
    fn->stack_size = offset;
  }

  // Generate assembly with traversing the AST
  codegen(prog);

  return 0;
}
