#ifndef CODEGEN_H
#define CODEGEN_H

#define INDENT_WIDTH 4

#include "ast.h"
#include "typechecker.h"

typedef struct codegen_state CodeGenState;

struct codegen_state {
    const Map *declared_vars;
    const Map *declared_func_sigs;
    const Map *defined_funcs;
    int indent;
    size_t func_sig_count;
    size_t func_count;
    size_t local_count;
};

int CodeGen_Program   (const void *this, FILE *out);
int CodeGen_Assignment(const void *this, FILE *out);
int CodeGen_Return    (const void *this, FILE *out);
int CodeGen_Expression(const void *this, FILE *out);
int CodeGen_Ref       (const void *this, FILE *out);
int CodeGen_Paren     (const void *this, FILE *out);
int CodeGen_Call      (const void *this, FILE *out);
int CodeGen_Function  (const void *this, FILE *out);
int CodeGen_Class     (const void *this, FILE *out);
int CodeGen_Int       (const void *this, FILE *out);
int CodeGen_Double    (const void *this, FILE *out);
int CodeGen_Variable  (const void *this, FILE *out);
int CodeGen_TypedVar  (const void *this, FILE *out);

#endif//CODEGEN_H
