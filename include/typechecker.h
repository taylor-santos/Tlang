#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"
#include "vector.h"

typedef struct var_type  VarType;
typedef struct named_arg NamedArg;
typedef struct func_type FuncType;

struct var_type {
    enum { BUILTIN, FUNCTION, NONE } type;
    union {
        enum { INT, DOUBLE } builtin;
        FuncType *function;
    };
    int init;
};

struct named_arg {
    char    *name;
    VarType *type;
};

struct func_type {
    const Vector *arg_names;
    const Vector *arg_types; // Vector<NamedArg*>
    VarType      *ret_type;
};

void free_VarType(void*);
void free_FuncType(void*);

int new_VarType(const char *type, VarType **vartype_ptr);
int new_NoneType(VarType **vartype_ptr);
int new_VarType_from_FuncType(FuncType *type, VarType **vartype_ptr);
int new_NamedArg(char *name, VarType *type, NamedArg **namedarg_ptr);
int new_FuncType(const Vector *args,
                 VarType *ret_type,
                 FuncType **functype_ptr);

int TypeCheck_Program(const void*);

int GetType_Assignment(const void*, const Map*, VarType**);
int GetType_Return    (const void*, const Map*, VarType**);
int GetType_Variable  (const void*, const Map*, VarType**);
int GetType_TypedVar  (const void*, const Map*, VarType**);
int GetType_Int       (const void*, const Map*, VarType**);
int GetType_Double    (const void*, const Map*, VarType**);
int GetType_Function  (const void*, const Map*, VarType**);

int AssignType_Variable(const void*, VarType*, const Map*);
int AssignType_TypedVar(const void*, VarType*, const Map*);

int GetVars_Variable(const void*);
int GetVars_TypedVar(const void*);

#endif//TYPECHECKER_H
