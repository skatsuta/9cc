#include "9cc.h"

char *read_file(char *path) {
  // Open and read the file
  FILE *fp = fopen(path, "r");
  if (!fp) {
    error("cannot open %s: %s", path, strerror(errno));
  }

  int max_file_size = 10 * 1024 * 1024;
  char *buf = malloc(max_file_size);
  int size = fread(buf, 1, max_file_size - 2, fp);
  if (!feof(fp)) {
    error("%s: file too large", path);
  }

  // Ensure that the string ends with "\n\0"
  if (size == 0 || buf[size - 1] != '\n') {
    buf[size] = '\n';
    size++;
  }
  buf[size] = '\0';
  return buf;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  // Tokenize and parse input
  filename = argv[1];
  user_input = read_file(filename);
  token = tokenize();
  Program *prog = program();

  // Assign offsets to local variables
  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset = align_to(offset, var->type->align) + var->type->size;
      var->offset = offset;
    }
    fn->stack_size = align_to(offset, 8);
  }

  // Generate assembly with traversing the AST
  codegen(prog);

  return 0;
}
