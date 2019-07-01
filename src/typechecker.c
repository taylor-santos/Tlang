#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"

#define UNUSED __attribute__ ((unused))
#define RED     "\033[0;91m"
#define WHITE   "\033[0m"
#define safe_call(obj, fn, ...) { \
    if (obj->fn(obj, __VA_ARGS__)) { \
        fprintf(stderr, \
                "%s:%d: " RED "internal compiler error: " WHITE #obj "." #fn \
                "(" #__VA_ARGS__ ") failed\n", \
                __FILE__, \
                __LINE__); \
        exit(EXIT_FAILURE); \
    } \
} \
void safe_call()

static int new_VarType(const char *type, VarType **vartype_ptr) {
    if (strcmp(type, "int") == 0) {
        *vartype_ptr = malloc(sizeof(**vartype_ptr));
        (*vartype_ptr)->init = 0;
        (*vartype_ptr)->type = BUILTIN;
        (*vartype_ptr)->builtin = INT;
        return 0;
    } else {
        //TODO: Create VarTypes from classes
        return 1;
    }
}

static int copy_VarType(VarType *type, VarType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    memcpy(*copy_ptr, type, sizeof(**copy_ptr));
    return 0;
}

static int typecmp(const VarType *type1, const VarType *type2) {
    //Returns 1 if the types differ, 0 if they match
    if (type1->type != type2->type) {
        return 1;
    }
    switch (type1->type) {
        case BUILTIN:
            if (type1->builtin != type2->builtin) {
                return 1;
            }
            return 0;
    }
    fprintf(stderr, "%s:%d: " RED "internal compiler error: " WHITE "switch "
            "statement failed to cover all cases\n", __FILE__, __LINE__);
    return 1;
}

int TypeCheck_Program(const void *this) {
    const ASTNode  *node  = this;
    ASTProgramData *data  = node->data;
    const Vector   *stmts = data->statements;
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_call(stmts, array, &size, &stmt_arr);
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        vtable->get_type(stmt_arr[i], data->symbols);
    }
    return 0;
}

VarType *GetType_Assignment(const void *this, const Map *symbols) {
    const ASTNode     *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTLExprVTable      *lhs_vtable = lhs->vtable;
    ASTAssignmentVTable *rhs_vtable = rhs->vtable;
    VarType *rhs_type = rhs_vtable->get_type(rhs, symbols);
    if (rhs_type->init == 0) {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
    }
    if (lhs_vtable->assign_type(lhs, rhs_type, symbols)) {
        //TODO: Handle type reassignment error
        fprintf(stderr, "error: type reassignment");
    }
    return rhs_type;
}

VarType *GetType_Variable(const void *this, const Map *symbols) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        VarType *type = NULL;
        safe_call(symbols, get, data->name, len, &type);
        return type;
    } else {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
        return NULL;
    }
}

VarType *GetType_TypedVar(const void *this, const Map *symbols) {
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        VarType *type = NULL;
        safe_call(symbols, get, data->name, len, &type);
        return type;
    } else {
        VarType *type = NULL;
        if (new_VarType(data->given_type, &type)) {
            //TODO: Handle invalid given type error
            return NULL;
        }
        safe_call(symbols, put, data->name, len, type, NULL);
        return type;
    }
}

VarType *GetType_Int(UNUSED const void *this, UNUSED const Map *symbols) {
    VarType *type = malloc(sizeof(*type));
    type->type = BUILTIN;
    type->builtin = INT;
    type->init = 1;
    return type;
}

int AssignType_Variable(const void *this, VarType *type, const Map *symbols) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        VarType *prev_type = NULL;
        safe_call(symbols, get, data->name, len, &prev_type);
        if (typecmp(type, prev_type)) {
            //TODO: Handle type mismatch error
            return 1;
        }
        return 0;
    } else {
        VarType *new_type = NULL;
        if (copy_VarType(type, &new_type)) {
            //TODO: Handle malloc error
            return 1;
        }
        safe_call(symbols, put, data->name, len, new_type, NULL);
        return 0;
    }
}

int AssignType_TypedVar(const void *this, VarType *type, const Map *symbols) {
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    VarType *prev_type = NULL;
    // Before comparing, check if it's in the symbol table. Add it if not.
    if (symbols->contains(symbols, data->name, len)) {
        safe_call(symbols, get, data->name, len, &prev_type);
    } else {
        if (new_VarType(data->given_type, &prev_type)) {
            //TODO: Handle invalid given type error
            return 1;
        }
        safe_call(symbols, put, data->name, len, prev_type, NULL);
    }
    prev_type->init = 1;
    if (typecmp(type, prev_type)) {
        //TODO: Handle type mismatch error
        return 1;
    }
    return 0;
}

int AssignType_Int(UNUSED const void *this,
                   VarType *type,
                   UNUSED const Map *symbols) {
    return type->type != BUILTIN || type->builtin != INT;
}


