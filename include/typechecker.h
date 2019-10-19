#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"
#include "vector.h"
#include "stack.h"

typedef struct var_type      VarType;
typedef struct named_arg     NamedArg;
typedef struct func_type     FuncType;
typedef struct gettype_state GetTypeState;

struct var_type {
    enum { BUILTIN, FUNCTION, NONE, REFERENCE, HOLD, CLASS, RETURN } type;
    union {
        enum { INT, DOUBLE } builtin;
        FuncType *function;
        VarType *sub_type;
    };
    int is_ref;
};

struct named_arg {
    char    *name;
    VarType *type;
};

struct func_type {
    const Vector *named_args; // Vector<NamedArg*>
    VarType      *ret_type;
    NamedArg     *extension;
};

int add_builtins(const Map*);

void free_VarType(void*);
void free_FuncType(void*);
void free_NamedArg(void*);

int new_VarType(const char *type, VarType **vartype_ptr);
int new_ReturnType(VarType **vartype_ptr, VarType *sub_type);
int new_NoneType (VarType **vartype_ptr);
int new_RefType  (VarType **vartype_ptr, VarType *sub_type);
int new_HoldType (VarType **vartype_ptr, VarType *ref_type);
int new_ClassType(VarType **vartype_ptr);
int new_VarType_from_FuncType(FuncType *type, VarType **vartype_ptr);
int new_NamedArg(char *name, VarType *type, NamedArg **namedarg_ptr);
int new_FuncType(const Vector *args,
                 VarType *ret_type,
                 FuncType **functype_ptr);

void print_VarType(const void*);

int TypeCheck_Program(const void*);

int GetType_Assignment(const void*, const Map*, const void*, VarType**);
int GetType_Def       (const void*, const Map*, const void*, VarType**);
int GetType_Return    (const void*, const Map*, const void*, VarType**);
int GetType_Expression(const void*, const Map*, const void*, VarType**);
int GetType_Ref       (const void*, const Map*, const void*, VarType**);
int GetType_Paren     (const void*, const Map*, const void*, VarType**);
int GetType_Hold      (const void*, const Map*, const void*, VarType**);
int GetType_Variable  (const void*, const Map*, const void*, VarType**);
int GetType_TypedVar  (const void*, const Map*, const void*, VarType**);
int GetType_Int       (const void*, const Map*, const void*, VarType**);
int GetType_Double    (const void*, const Map*, const void*, VarType**);
int GetType_Function  (const void*, const Map*, const void*, VarType**);
int GetType_Class     (const void*, const Map*, const void*, VarType**);

int AssignType_Variable(const void*, VarType*, const Map*);
int AssignType_TypedVar(const void*, VarType*, const Map*);

int GetVars_Assignment(const void*, const Vector*);
int GetVars_Def       (const void*, const Vector*);
int GetVars_Return    (const void*, const Vector*);
int GetVars_Expression(const void*, const Vector*);
int GetVars_Ref       (const void*, const Vector*);
int GetVars_Paren     (const void*, const Vector*);
int GetVars_Hold      (const void*, const Vector*);
int GetVars_Variable  (const void*, const Vector*);
int GetVars_TypedVar  (const void*, const Vector*);
int GetVars_Int       (const void*, const Vector*);
int GetVars_Double    (const void*, const Vector*);
int GetVars_Function  (const void*, const Vector*);
int GetVars_Class     (const void*, const Vector*);

#endif//TYPECHECKER_H
