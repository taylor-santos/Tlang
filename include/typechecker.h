#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"

typedef struct var_type VarType;

struct var_type {
    enum { BUILTIN } type;
    union {
        enum { INT } builtin;
    };
    int init;
};

int TypeCheck_Program(const void*);

VarType *GetType_Assignment(const void*, const Map*);
VarType *GetType_Variable  (const void*, const Map*);
VarType *GetType_TypedVar  (const void*, const Map*);
VarType *GetType_Int       (const void*, const Map*);

int AssignType_Variable(const void*, VarType*, const Map*);
int AssignType_TypedVar(const void*, VarType*, const Map*);
int AssignType_Int     (const void*, VarType*, const Map*);

#endif//TYPECHECKER_H
