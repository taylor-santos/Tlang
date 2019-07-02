#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"

typedef struct var_type VarType;

struct var_type {
    enum { BUILTIN } type;
    union {
        enum { INT, DOUBLE } builtin;
    };
    int init;
};

int TypeCheck_Program(const void*);

VarType *GetType_Assignment(const void*, const Map*, int*);
VarType *GetType_Variable  (const void*, const Map*, int*);
VarType *GetType_TypedVar  (const void*, const Map*, int*);
VarType *GetType_Int       (const void*, const Map*, int*);
VarType *GetType_Double    (const void*, const Map*, int*);

int AssignType_Variable(const void*, VarType*, const Map*);
int AssignType_TypedVar(const void*, VarType*, const Map*);
int AssignType_Int     (const void*, VarType*, const Map*);
int AssignType_Double  (const void*, VarType*, const Map*);

#endif//TYPECHECKER_H
