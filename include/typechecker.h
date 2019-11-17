#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "map.h"
#include "vector.h"
#include "stack.h"
#include "ast.h"

typedef struct VarType    VarType;
typedef struct NamedType  NamedType;
typedef struct FuncType   FuncType;
typedef struct ClassType  ClassType;
typedef struct ObjectType ObjectType;

struct VarType {
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

struct NamedType {
    VarType *type;
    char    *name;
};

struct FuncType {
    const Vector *named_args; // Vector<NamedType*>
    VarType      *ret_type;
};

struct ClassType {
    const Map *field_name_to_type; // Map<char*, VarType*>
    int classID;
    VarType *instance;
};

struct ObjectType {
    enum { NAME, ID } id_type;
    union {
        char *name;
        int classID;
    };
};

typedef struct {
    const struct ASTNode *program_node;
    const Vector *new_symbols; //Vector<NamedType*>
    const Vector *classTypes;  //Vector<ClassType*>
    VarType *curr_ret_type;
} TypeCheckState;

int add_builtins(const ASTNode *node);
int typecmp(const VarType *type1, const VarType *type2, TypeCheckState *state);
int copy_VarType(const void *type, const void *copy_ptr);
void print_VarType(const void*);
int getObjectID(VarType *type, const Map *symbols);

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

#endif//TYPECHECKER_H
