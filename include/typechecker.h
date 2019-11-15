#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"
#include "vector.h"
#include "stack.h"

typedef struct var_type     VarType;
typedef struct named_type   NamedType;
typedef struct func_type    FuncType;
typedef struct class_type   ClassType;
typedef struct object_type  ObjectType;

struct var_type {
    enum { FUNCTION, REFERENCE, HOLD, CLASS, OBJECT, TUPLE, PAREN } type;
    union {
        FuncType *function;
        ClassType *class;
        ObjectType *object;
        const Vector *tuple;
        VarType *sub_type;
    };
    int is_ref;
};

struct named_type {
    VarType *type;
    char    *name;
};

struct func_type {
    const Vector *named_args; // Vector<NamedType*>
    VarType      *ret_type;
};

struct class_type {
    ClassType *def;
    const Vector *stmts;    // Vector<const ASTNode*>
    const Vector *fields;   // Vector<NamedType*>
    const Map *field_names; // Map<char*, VarType*>
    size_t classID;
    VarType *instance;
};

struct object_type {
    int classID;
    char *className;
};

int add_builtins(const Map*, const Vector*);

void free_VarType(void*);
void free_FuncType(void*);
void free_ClassType(void*);
void free_ObjectType(void*);
void free_NamedType(void*);

int new_RefType   (VarType **vartype_ptr, VarType *sub_type);
int new_TupleType (VarType **vartype_ptr, const Vector *types);
int new_HoldType  (VarType **vartype_ptr);
int new_ClassType (VarType **vartype_ptr);
int new_ObjectType(VarType **vartype_ptr);
int new_ParenType (VarType **vartype_ptr);
int new_NamedType (char *name, VarType *type, NamedType **namedarg_ptr);
int new_FuncType  (const Vector *args,
                   VarType *ret_type,
                   VarType **vartype_ptr);

void print_VarType(const void*);

int TypeCheck_Program(const void*);

int GetType_Assignment(const void*, const Map*, void*, VarType**);
int GetType_Def       (const void*, const Map*, void*, VarType**);
int GetType_Return    (const void*, const Map*, void*, VarType**);
int GetType_Expression(const void*, const Map*, void*, VarType**);
int GetType_Tuple     (const void*, const Map*, void*, VarType**);
int GetType_Ref       (const void*, const Map*, void*, VarType**);
int GetType_Paren     (const void*, const Map*, void*, VarType**);
int GetType_Hold      (const void*, const Map*, void*, VarType**);
int GetType_Variable  (const void*, const Map*, void*, VarType**);
int GetType_TypedVar  (const void*, const Map*, void*, VarType**);
int GetType_Int       (const void*, const Map*, void*, VarType**);
int GetType_Double    (const void*, const Map*, void*, VarType**);
int GetType_Function  (const void*, const Map*, void*, VarType**);
int GetType_Class     (const void*, const Map*, void*, VarType**);

int GetVars_Assignment(const void*, const Vector*);
int GetVars_Def       (const void*, const Vector*);
int GetVars_Return    (const void*, const Vector*);
int GetVars_Expression(const void*, const Vector*);
int GetVars_Tuple     (const void*, const Vector*);
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
