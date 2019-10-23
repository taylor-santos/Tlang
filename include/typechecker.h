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
    enum { FUNCTION, REFERENCE, HOLD, CLASS, OBJECT } type;
    union {
        FuncType *function;
        ClassType *class;
        ObjectType *object;
        VarType *sub_type;
    };
    int is_ref;
    int init;
};

struct named_type {
    char    *name;
    VarType *type;
};

struct func_type {
    const Vector *named_args; // Vector<NamedType*>
    VarType      *ret_type;
    NamedType    *extension;
};

struct class_type {
    int isDef;
    const Vector *fields;   // Vector<NamedType*>
    const Map *field_names; // Map<char*, NULL>
    size_t classID;
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
int new_HoldType  (VarType **vartype_ptr, VarType *ref_type);
int new_ClassType (VarType **vartype_ptr);
int new_ObjectType(VarType **vartype_ptr);
int new_VarType_from_FuncType(FuncType *type, VarType **vartype_ptr);
int new_NamedArg(char *name, VarType *type, NamedType **namedarg_ptr);
int new_FuncType(const Vector *args,
                 VarType *ret_type,
                 FuncType **functype_ptr);

void print_VarType(const void*);

int TypeCheck_Program(const void*);

int GetType_Assignment(const void*, const Map*, void*, VarType**);
int GetType_Def       (const void*, const Map*, void*, VarType**);
int GetType_Return    (const void*, const Map*, void*, VarType**);
int GetType_Expression(const void*, const Map*, void*, VarType**);
int GetType_Ref       (const void*, const Map*, void*, VarType**);
int GetType_Paren     (const void*, const Map*, void*, VarType**);
int GetType_Hold      (const void*, const Map*, void*, VarType**);
int GetType_Variable  (const void*, const Map*, void*, VarType**);
int GetType_TypedVar  (const void*, const Map*, void*, VarType**);
int GetType_Int       (const void*, const Map*, void*, VarType**);
int GetType_Double    (const void*, const Map*, void*, VarType**);
int GetType_Function  (const void*, const Map*, void*, VarType**);
int GetType_Class     (const void*, const Map*, void*, VarType**);

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

int GetName_Assignment(const void*, char**);
int GetName_Def       (const void*, char**);
int GetName_Return    (const void*, char**);
int GetName_Expression(const void*, char**);
int GetName_Ref       (const void*, char**);
int GetName_Paren     (const void*, char**);
int GetName_Hold      (const void*, char**);
int GetName_Variable  (const void*, char**);
int GetName_TypedVar  (const void*, char**);
int GetName_Int       (const void*, char**);
int GetName_Double    (const void*, char**);
int GetName_Function  (const void*, char**);
int GetName_Class     (const void*, char**);

#endif//TYPECHECKER_H
