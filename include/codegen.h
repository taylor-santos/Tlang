#ifndef TLANG_CODEGEN_H
#define TLANG_CODEGEN_H

#include <stdio.h>
#include "ast.h"

typedef struct {
    int indent;
    const Map *func_defs;
    const Vector *class_defs;
    int tmp_count;
} CodegenState;

int GenerateCode(const ASTNode *program, FILE *out);
void print_indent(int n, FILE *out);

#endif//TLANG_CODEGEN_H
