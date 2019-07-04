#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"

static const Vector *assigned_vars; // Use this to store any variables that will
// be assigned to an object with scope. They
// will then be visible within that scope as
// instances of itself.

static int funccmp(const FuncType *type1, const FuncType *type2);

static int strdup_safe(const char *str, char **dup_ptr) {
    if (dup_ptr == NULL) {
        return 1;
    }
    *dup_ptr = strdup(str);
    if (*dup_ptr == NULL) {
        return 1;
    }
    return 0;
}

void free_VarType(void *this) {
    VarType *type = this;
    switch(type->type) {
        case BUILTIN:
            free(this);
            break;
        case NONE:
            free(this);
            break;
        case FUNCTION:
            type->function->arg_names->free(type->function->arg_types, free);
            type->function->arg_types->free(type->function->arg_types,
                                            free_VarType);
            free_VarType(type->function->ret_type);
            free(this);
            break;
    }
}

int new_VarType(const char *type, VarType **vartype_ptr) {
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

int new_NoneType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = NONE;
    return 0;
}

int new_VarType_from_FuncType(FuncType *type, VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = FUNCTION;
    (*vartype_ptr)->function = type;
    return 0;
}

int new_NamedArg(char *name, VarType *type, NamedArg **namedarg_ptr) {
    if (namedarg_ptr == NULL) {
        return 1;
    }
    *namedarg_ptr = malloc(sizeof(**namedarg_ptr));
    if (*namedarg_ptr == NULL) {
        return 1;
    }
    (*namedarg_ptr)->name = name;
    (*namedarg_ptr)->type = type;
    return 0;
 }

int new_FuncType(const Vector *args, VarType *ret_type, FuncType **functype_ptr) {
    if (functype_ptr == NULL) {
        return 1;
    }
    *functype_ptr = malloc(sizeof(**functype_ptr));
    if (*functype_ptr == NULL) {
        return 1;
    }
    (*functype_ptr)->arg_types = args;
    (*functype_ptr)->ret_type  = ret_type;
    return 0;
}

static int copy_FuncType(FuncType *type, FuncType **copy_ptr);

static int copy_VarType(const void *type, const void *copy_ptr) {
    VarType *VT_type      = (VarType*)type;
    VarType **VT_copy_ptr = (VarType**)copy_ptr;
    if (VT_copy_ptr == NULL) {
        return 1;
    }
    *VT_copy_ptr = malloc(sizeof(**VT_copy_ptr));
    if (*VT_copy_ptr == NULL) {
        return 1;
    }
    (*VT_copy_ptr)->type = VT_type->type;
    switch(VT_type->type) {
        case BUILTIN:
            (*VT_copy_ptr)->builtin = VT_type->builtin;
            break;
        case NONE:
            break;
        case FUNCTION:
            safe_function_call(copy_FuncType,
                               VT_type->function,
                               &(*VT_copy_ptr)->function);
            break;
    }
    (*VT_copy_ptr)->init = VT_type->init;
    return 0;
}

static int copy_NamedArg(const void *this, const void *copy_ptr) {
    NamedArg *arg = (NamedArg*)this;
    NamedArg **copy = (NamedArg**)copy_ptr;
    if (copy == NULL) {
        return 1;
    }
    *copy = malloc(sizeof(**copy));
    safe_function_call(copy_VarType, arg->type, &(*copy)->type);
    safe_function_call(strdup_safe, arg->name, &(*copy)->name);
    return 0;
}

static int copy_FuncType(FuncType *type, FuncType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    safe_method_call(type->arg_types,
                     copy,
                     &(*copy_ptr)->arg_types,
                     copy_NamedArg);
    safe_function_call(copy_VarType, type->ret_type, &(*copy_ptr)->ret_type);
    return 0;
}

static int typecmp(const VarType *type1, const VarType *type2) {
    //Returns 1 if the types differ, 0 if they match
    if (type1->type != type2->type) {
        return 1;
    }
    switch (type1->type) {
        case BUILTIN:
            return type1->builtin != type2->builtin;
        case FUNCTION:
            return funccmp(type1->function, type2->function);
        case NONE:
            return 0;
    }
    fprintf(stderr, "%s:%d: " RED "internal compiler error: " WHITE "switch "
            "statement failed to cover all cases\n", __FILE__, __LINE__);
    return 1;
}

static int funccmp(const FuncType *type1, const FuncType *type2) {
    int size1, size2;
    NamedArg **arr1 = NULL, **arr2 = NULL;
    safe_method_call(type1->arg_types, array, &size1, &arr1);
    safe_method_call(type2->arg_types, array, &size2, &arr2);
    if (size1 != size2) {
        return 1;
    }
    for (int i = 0; i < size1; i++) {
        if (typecmp(arr1[i]->type, arr2[i]->type)) {
            return 1;
        }
    }
    return typecmp(type1->ret_type, type2->ret_type);
}

int TypeCheck_Program(const void *this) {
    const ASTNode  *node  = this;
    ASTProgramData *data  = node->data;
    const Vector   *stmts = data->statements;
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(stmts, array, &size, &stmt_arr);
    int errstate = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        errstate = vtable->get_type(stmt_arr[i], data->symbols, &type)
                   || errstate;
    }
    return errstate;
}

int GetType_Assignment(const void *this,
                       const Map *symbols,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    int err = 0;
    const ASTNode     *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTLExprVTable      *lhs_vtable = lhs->vtable;
    ASTAssignmentVTable *rhs_vtable = rhs->vtable;
    if (assigned_vars == NULL) {
        assigned_vars = new_Vector(0);
    }
    lhs_vtable->get_vars(lhs);
    *type_ptr = NULL;
    err = rhs_vtable->get_type(rhs, symbols, type_ptr) || err;
    if (*type_ptr == NULL) {
        return 1;
    }
    if ((*type_ptr)->init == 0) {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
        return 1;
    }
    if (lhs_vtable->assign_type(lhs, *type_ptr, symbols)) {
        //TODO: Handle type reassignment error
        fprintf(stderr, "error: type reassignment\n");
        err = 1;
    }
    return err;
}

int GetType_Return(UNUSED const void *this,
                   UNUSED const Map *symbols,
                   VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    safe_function_call(new_NoneType, type_ptr);
    return 0;
}

int GetType_Variable(const void *this, const Map *symbols, VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (assigned_vars != NULL) {
        safe_method_call(assigned_vars, append, data->name);
    }
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, type_ptr);
        return 0;
    } else {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
        return 1;
    }
}

int GetType_TypedVar(const void *this, const Map *symbols, VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    if (assigned_vars != NULL) {
        safe_method_call(assigned_vars, append, data->name);
    }
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, type_ptr);
        if (typecmp(*type_ptr, data->given_type)) {
            return 1;
        }
        return 0;
    } else {
        safe_function_call(copy_VarType, data->given_type, type_ptr);
        safe_method_call(symbols, put, data->name, len, *type_ptr, NULL);
        return 0;
    }
}

int GetType_Int(UNUSED const void *this,
                UNUSED const Map *symbols,
                VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    *type_ptr = malloc(sizeof(**type_ptr));
    (*type_ptr)->type = BUILTIN;
    (*type_ptr)->builtin = INT;
    (*type_ptr)->init = 1;
    return 0;
}

int GetType_Double(UNUSED const void *this,
                   UNUSED const Map *symbols,
                   VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    *type_ptr = malloc(sizeof(**type_ptr));
    (*type_ptr)->type = BUILTIN;
    (*type_ptr)->builtin = DOUBLE;
    (*type_ptr)->init = 1;
    return 0;
}

int GetType_Function(const void *this,
                     UNUSED const Map *symbols,
                     VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    const Map *symbols_copy = NULL;
    safe_method_call(symbols, copy, &symbols_copy, copy_VarType);
    for (int i = 0; i < assigned_vars->size(assigned_vars); i++) {
        char *var = NULL;
        safe_method_call(assigned_vars, get, i, &var);
        size_t len = strlen(var);
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, data->type, &type_copy);
        VarType *prev_type = NULL;
        safe_method_call(symbols_copy, put, var, len, type_copy, &prev_type);
        if (prev_type != NULL) {
            //TODO: Verify type compatibility
            free_VarType(prev_type);
        }
    }
    assigned_vars->clear(assigned_vars, NULL);

    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(data->stmts, array, &size, &stmt_arr);
    int err = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        err = vtable->get_type(stmt_arr[i], symbols_copy, &type) || err;
    }
    *type_ptr = data->type;
    return 0;
}

int AssignType_Variable(const void *this, VarType *type, const Map *symbols) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        VarType *prev_type = NULL;
        safe_method_call(symbols, get, data->name, len, &prev_type);
        if (typecmp(type, prev_type)) {
            //TODO: Handle type mismatch error
            return 1;
        }
        return 0;
    } else {
        VarType *new_type = NULL;
        safe_function_call(copy_VarType, type, &new_type);
        safe_method_call(symbols, put, data->name, len, new_type, NULL);
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
        safe_method_call(symbols, get, data->name, len, &prev_type);
    } else {
        safe_function_call(copy_VarType, type, &prev_type);
        safe_method_call(symbols, put, data->name, len, prev_type, NULL);
    }
    prev_type->init = 1;
    if (typecmp(type, prev_type)) {
        //TODO: Handle type mismatch error
        return 1;
    }
    return 0;
}

int GetVars_Variable(const void *this) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    safe_method_call(assigned_vars, append, data->name);
    return 0;
}

int GetVars_TypedVar(const void *this) {
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    safe_method_call(assigned_vars, append, data->name);
    return 0;
}
