#ifndef CODEGEN_H
#define CODEGEN_H

#define INDENT_WIDTH 4

#include "ast.h"
#include "typechecker.h"

int CodeGen_Program   (const void *this, void *state, FILE *out);
int CodeGen_Assignment(const void *this, void *state, FILE *out);
int CodeGen_Return    (const void *this, void *state, FILE *out);
int CodeGen_Expression(const void *this, void *state, FILE *out);
int CodeGen_Ref       (const void *this, void *state, FILE *out);
int CodeGen_Paren     (const void *this, void *state, FILE *out);
int CodeGen_Function  (const void *this, void *state, FILE *out);
int CodeGen_Class     (const void *this, void *state, FILE *out);
int CodeGen_Int       (const void *this, void *state, FILE *out);
int CodeGen_Double    (const void *this, void *state, FILE *out);
int CodeGen_Variable  (const void *this, void *state, FILE *out);
int CodeGen_TypedVar  (const void *this, void *state, FILE *out);

#endif//CODEGEN_H
