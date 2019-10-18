#ifndef CODEGEN_H
#define CODEGEN_H

#define INDENT_WIDTH 4

#include "ast.h"
#include "typechecker.h"

char *CodeGen_Program   (const void *this, void *state, FILE *out);
char *CodeGen_Assignment(const void *this, void *state, FILE *out);
char *CodeGen_Def       (const void *this, void *state, FILE *out);
char *CodeGen_Return    (const void *this, void *state, FILE *out);
char *CodeGen_Expression(const void *this, void *state, FILE *out);
char *CodeGen_Ref       (const void *this, void *state, FILE *out);
char *CodeGen_Paren     (const void *this, void *state, FILE *out);
char *CodeGen_Hold      (const void *this, void *state, FILE *out);
char *CodeGen_Function  (const void *this, void *state, FILE *out);
char *CodeGen_Class     (const void *this, void *state, FILE *out);
char *CodeGen_Int       (const void *this, void *state, FILE *out);
char *CodeGen_Double    (const void *this, void *state, FILE *out);
char *CodeGen_Variable  (const void *this, void *state, FILE *out);
char *CodeGen_TypedVar  (const void *this, void *state, FILE *out);

#endif//CODEGEN_H
