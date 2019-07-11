#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"

static GetTypeState *state;
static int in_ref;
static int in_paren;
static const Vector *assigned_vars; // Use this to store any variables that will
                                    // be assigned to an object with scope. They
                                    // will then be visible within that scope as
                                    // instances of itself.

static int funccmp(const FuncType *type1, const FuncType *type2);
static int copy_FuncType(FuncType *type, FuncType **copy_ptr);

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

int add_builtins(const Map *map) {
    if (map == NULL) {
        return 1;
    }
    const char *binops[] = { "+", "-", "*", "/" };
    size_t n = sizeof(binops) / sizeof(*binops);
    for (size_t i = 0; i < n; i++) {
        VarType *arg_type = NULL;
        safe_function_call(new_VarType, "int", &arg_type);
        NamedArg *arg = NULL;
        safe_function_call(new_NamedArg, strdup("b"), arg_type, &arg);
        const Vector *args = new_Vector(0);
        safe_method_call(args, append, arg);
        VarType *ret_type = NULL;
        safe_function_call(new_VarType, "int", &ret_type);
        FuncType *func = NULL;
        safe_function_call(new_FuncType, args, ret_type, &func);
        VarType *ext_type = NULL;
        safe_function_call(new_VarType, "int", &ext_type);
        safe_function_call(new_NamedArg,
            strdup("a"),
            ext_type,
            &func->extension);
        VarType *func_type = NULL;
        safe_function_call(new_VarType_from_FuncType, func, &func_type);
        func_type->init = 1;
        VarType *prev_type = NULL;
        safe_method_call(map,
                         put,
                         binops[i],
                         strlen(binops[i]),
                         func_type,
                         &prev_type);
        if (prev_type != NULL) {
            free_VarType(prev_type);
        }
    }
    return 0;
}

void free_VarType(void *this) {
    VarType *type = this;
    switch(type->type) {
        case BUILTIN:
            free(this);
            return;
        case FUNCTION:
            free_FuncType(type->function);
            free(this);
            return;
        case NONE:
            free(this);
            return;
        case REFERENCE:
            free(this);
            return;
        case CALLER:
            free(this);
            return;
    }
    //TODO
    fprintf(stderr, "error: VarType not handled in switch statement.\n");
}

void free_FuncType(void *this) {
    FuncType *type = this;
    type->named_args->free(type->named_args, free_NamedArg);
    free_VarType(type->ret_type);
    if (type->extension != NULL) {
        free_NamedArg(type->extension);
    }
    free(this);
}

void free_NamedArg(void *this) {
    NamedArg *arg = this;
    free(arg->name);
    free_VarType(arg->type);
    free(this);
}

int new_VarType(const char *type, VarType **vartype_ptr) {
    if (strcmp(type, "int") == 0) {
        *vartype_ptr = malloc(sizeof(**vartype_ptr));
        (*vartype_ptr)->init = 0;
        (*vartype_ptr)->type = BUILTIN;
        (*vartype_ptr)->builtin = INT;
        return 0;
    } else if (strcmp(type, "double") == 0) {
        *vartype_ptr = malloc(sizeof(**vartype_ptr));
        (*vartype_ptr)->init = 0;
        (*vartype_ptr)->type = BUILTIN;
        (*vartype_ptr)->builtin = DOUBLE;
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

int new_RefType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = REFERENCE;
    return 0;
}

int new_CallType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = CALLER;
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
    (*functype_ptr)->named_args = args;
    (*functype_ptr)->ret_type  = ret_type;
    (*functype_ptr)->ret_type->init = 1;
    (*functype_ptr)->extension = NULL;
    return 0;
}

void print_VarType(const void *this) {
    const VarType *type = this;
    switch(type->type) {
        case BUILTIN:
            switch(type->builtin) {
                case INT:
                    printf("int");
                    break;
                case DOUBLE:
                    printf("double");
                    break;
            }
            break;
        case FUNCTION:
            if (type->function->extension != NULL) {
                printf("(");
                if (type->function->extension->name != NULL) {
                    printf("%s: ", type->function->extension->name);
                }
                print_VarType(type->function->extension->type);
                printf(")");
            }
            printf("fn(");
            FuncType *func = type->function;
            int n = func->named_args->size(func->named_args);
            char *sep = "";
            for (int i = 0; i < n; i++) {
                NamedArg *arg = NULL;
                safe_method_call(func->named_args, get, i, &arg);
                printf("%s", sep);
                if (arg->name != NULL) {
                    printf("%s:", arg->name);
                }
                print_VarType(arg->type);
                sep = ", ";
            }
            printf(")->");
            print_VarType(func->ret_type);
            break;
        case NONE:
            printf("none");
            break;
        case REFERENCE:
            printf("ref");
            break;
        case CALLER:
            printf("call");
            break;
    }
}

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
        case FUNCTION:
            safe_function_call(copy_FuncType,
            VT_type->function,
            &(*VT_copy_ptr)->function);
            break;
        case NONE:
            break;
        case REFERENCE:
            break;
        case CALLER:
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
    if (arg->name != NULL) {
        safe_function_call(strdup_safe, arg->name, &(*copy)->name);
    } else {
        (*copy)->name = NULL;
    }
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
    safe_method_call(type->named_args,
                     copy,
                     &(*copy_ptr)->named_args,
                     copy_NamedArg);
    safe_function_call(copy_VarType, type->ret_type, &(*copy_ptr)->ret_type);
    if (type->extension != NULL) {
        safe_function_call(copy_NamedArg,
                           type->extension,
                           &(*copy_ptr)->extension);
    } else {
        (*copy_ptr)->extension = NULL;
    }
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
        case REFERENCE:
            return 1; //TODO: Should ref types be comparable? The caller should
                      //      probably handle checking the referenced type
        case CALLER:
            return 1;
    }
    fprintf(stderr, "%s:%d: " RED "internal compiler error: " WHITE "switch "
            "statement failed to cover all cases\n", __FILE__, __LINE__);
    return 1;
}

static int funccmp(const FuncType *type1, const FuncType *type2) {
    int size1, size2, ret = 0;
    NamedArg **arr1 = NULL, **arr2 = NULL;
    safe_method_call(type1->named_args, array, &size1, &arr1);
    safe_method_call(type2->named_args, array, &size2, &arr2);
    if (size1 != size2) {
        ret = 1;
    } else {
        for (int i = 0; i < size1; i++) {
            if (typecmp(arr1[i]->type, arr2[i]->type)) {
                ret = 1;
                break;
            }
        }
    }
    free(arr1);
    free(arr2);
    return ret || typecmp(type1->ret_type, type2->ret_type);
}

int TypeCheck_Program(const void *this) {
    const ASTNode  *node  = this;
    ASTProgramData *data  = node->data;
    const Vector   *stmts = data->statements;
    if (state == NULL) {
        state = malloc(sizeof(*state));
        state->expr_stack = new_Stack(0);
    }
    state->scope_type = NULL;
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
    free(stmt_arr);
    print_Map(data->symbols, print_VarType);
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
    assigned_vars->clear(assigned_vars, NULL);
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
    if (state == NULL || state->scope_type->type != FUNCTION) {
        //TODO: Handle return statement outside of function
        fprintf(stderr, "return statement outside of function\n");
        return 1;
    }
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    const ASTNode *ret_expr = data->returns;
    if (ret_expr == NULL) {
        // Returns nothing
        if (state->scope_type->function->ret_type->type != NONE) {
            //TODO: Handle incompatible return type
            fprintf(stderr,
                    "error: returning nothing from a function that expects a "
                    "return type\n");
            return 1;
        } else {
            *type_ptr = data->type;
            return 0;
        }
    }
    VarType *ret_type = NULL;
    ASTRExprVTable *ret_vtable = ret_expr->vtable;
    if (ret_vtable->get_type(ret_expr, symbols, &ret_type)) {
        //TODO: Handle failed to infer type of returned value
        fprintf(stderr, "error: failed to infer type of returned value\n");
        return 1;
    } else {
        FuncType *func_type = state->scope_type->function;
        // Compare stated return type of function to type of return statement
        if (typecmp(func_type->ret_type, ret_type)) {
            //TODO: Handle incompatible return statement to function return type
            fprintf(stderr, "error: incompatible return type\n");
            return 1;
        }
        *type_ptr = ret_type;
        return 0;
    }
}

int parse_expression(const ASTNode **nodes,
                     int size,
                     int *index_ptr,
                     const Map *symbols,
                     VarType **type_ptr,
                     int ret_first_type) {
    if (*index_ptr < 0 || size <= *index_ptr) {
        //TODO: Handle not enough arguments
        fprintf(stderr, "error: Not enough arguments\n");
        return 1;
    }
    ASTStatementVTable *vtable = nodes[*index_ptr]->vtable;
    VarType* type = NULL;
    int prev_paren = in_paren;
    in_paren = 0;
    if (vtable->get_type(nodes[*index_ptr], symbols, &type)) {
        return 1;
    }
    if (in_paren) {
        in_paren = prev_paren;
        (*index_ptr)++;
        *type_ptr = type;
        return 0;
    }
    in_paren = prev_paren;
    (*index_ptr)++;
    VarType *expr_type = NULL;
    switch (type->type) {
        case BUILTIN:
            expr_type = type;
            break;
        case FUNCTION:;
            if (type->function->extension != NULL) {
                *type_ptr = type;
                return 0;
            }
            int arg_count =
                type->function->named_args->size(
                    type->function->named_args);
            // Don't implicitly call functions without args
            if (in_ref == 0 && arg_count > 0) {
                if (*index_ptr + arg_count > size) {
                    *type_ptr = type;
                    return 0;
                }
                for (int i = 0; i < arg_count; i++) {
                    VarType *arg_type = NULL;
                    if (parse_expression(nodes,
                                         size,
                                         index_ptr,
                                         symbols,
                                         &arg_type,
                                         1)) {
                        return 1;
                    }
                    NamedArg *expected_arg = NULL;
                    safe_method_call(type->function->named_args,
                        get,
                        i,
                        &expected_arg);
                    if (typecmp(arg_type, expected_arg->type)) {
                        //TODO: Handle incompatible argument type
                        fprintf(stderr,
                            "error: incompatible argument type\n");
                        return 1;
                    }
                }
                type = type->function->ret_type;
            }
            expr_type = type;
            break;
        case NONE:
            expr_type = type;
            break;
        case REFERENCE:;
            int prev_ref = in_ref;
            in_ref = 1;
            if (parse_expression(nodes,
                                 size,
                                 index_ptr,
                                 symbols,
                                 &expr_type,
                                 1)) {
                return 1;
            }

            in_ref = prev_ref;
            break;
        case CALLER:
            prev_ref = in_ref;
            in_ref = 0;
            if (parse_expression(nodes,
                                 size,
                                 index_ptr,
                                 symbols,
                                 &expr_type,
                                 1)) {
                return 1;
            }
            in_ref = prev_ref;
            if (expr_type->type == FUNCTION) {
                const Vector *args = expr_type->function->named_args;
                int num_args = args->size(args);
                if (num_args == 0) {
                    expr_type = expr_type->function->ret_type;
                    break;
                }
            }
            //TODO: Handle warning incorrect 'call' usage
            fprintf(stderr,
                "warning: 'call' should only be used on nullary functions\n");
            break;
    }
    while (ret_first_type == 0 && *index_ptr < size) {
        VarType *new_type;
        if (parse_expression(nodes,
                             size,
                             index_ptr,
                             symbols,
                             &new_type,
                             0)) {
            return 1;
        }
        if (new_type->type == FUNCTION &&
            new_type->function->extension != NULL) {
            if (typecmp(new_type->function->extension->type, expr_type)) {
                //TODO: Handle incompatible left-hand type
                fprintf(stderr, "error: incompatible left-hand type\n");
                return 1;
            } else {
                int arg_count =
                    new_type->function->named_args->size(
                        new_type->function->named_args);
                for (int i = 0; i < arg_count; i++) {
                    VarType *arg_type = NULL;
                    if (parse_expression(nodes,
                        size,
                        index_ptr,
                        symbols,
                        &arg_type,
                        1)) {
                        return 1;
                    }
                    NamedArg *expected_arg = NULL;
                    safe_method_call(new_type->function->named_args,
                        get,
                        i,
                        &expected_arg);
                    if (typecmp(arg_type, expected_arg->type)) {
                        //TODO: Handle incompatible argument type
                        fprintf(stderr,
                            "error: incompatible argument type\n");
                        return 1;
                    }
                }
                expr_type = new_type->function->ret_type;
                if (expr_type->type == FUNCTION &&
                    expr_type->function->extension != NULL) {
                    break;
                }
            }
        } else {
            //TODO: Handle dangling expression
            fprintf(stderr, "error: dangling expression\n");
            return 1;
        }
    }
    *type_ptr = expr_type;
    return 0;
}

int GetType_Expression(const void *this,
                       const Map *symbols,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode     *node  = this;
    ASTExpressionData *data  = node->data;
    const Vector      *exprs = data->exprs;
    const ASTNode **nodes = NULL;
    int size = exprs->size(exprs);
    safe_method_call(exprs, array, &size, &nodes);
    int index = 0;
    int result = parse_expression(nodes, size, &index, symbols, type_ptr, 0);
    if (index < size) {
        //TODO: Handle dangling expression
        fprintf(stderr, "error: dangling expression\n");
        return 1;
    }
    free(nodes);
    return result;
}

int GetType_Ref(const void *this,
                UNUSED const Map *symbols,
                VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTRefData    *data = node->data;
    *type_ptr = data->type;
    return 0;
}

int GetType_Paren(const void *this,
                  UNUSED const Map *symbols,
                  VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode      *node   = this;
    ASTParenData       *data   = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    int prev_ref = in_ref;
    in_ref = 0;
    in_paren = 1;
    int result =  vtable->get_type(data->val, symbols, type_ptr);
    in_ref = prev_ref;
    return result;
}

int GetType_Call(UNUSED const void *this,
                 UNUSED const Map *symbols,
                 VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    safe_function_call(new_CallType, type_ptr);
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
    const ASTNode *node = this;
    ASTIntData    *data = node->data;
    *type_ptr = data->type;
    return 0;
}

int GetType_Double(UNUSED const void *this,
                   UNUSED const Map *symbols,
                   VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    *type_ptr = data->type;
    return 0;
}

int GetType_Function(const void *this,
                     const Map *symbols,
                     VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    data->symbols->free(data->symbols, free_VarType);
    safe_method_call(symbols, copy, &data->symbols, copy_VarType);
    for (int i = 0; i < assigned_vars->size(assigned_vars); i++) {
        char *var = NULL;
        safe_method_call(assigned_vars, get, i, &var);
        size_t len = strlen(var);
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, data->type, &type_copy);
        VarType *prev_type = NULL;
        safe_method_call(data->symbols, put, var, len, type_copy, &prev_type);
        if (prev_type != NULL) {
            //TODO: Verify type compatibility
            fprintf(stderr, "Possible type conflict???\n");
            free_VarType(prev_type);
        }
    }
    if (data->signature->extension != NULL) {
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType,
            data->signature->extension->type,
            &type_copy);
        VarType *prev_type = NULL;
        safe_method_call(data->symbols,
            put,
            data->signature->extension->name,
            strlen(data->signature->extension->name),
            type_copy,
            &prev_type);
        if (prev_type != NULL) {
            //TODO: Handle argument type conflict with outer scope variable
            free_VarType(prev_type);
        }
    }
    //TODO: ensure argument name does not conflict with extension name
    FuncType *signature = data->signature;
    NamedArg **args = NULL;
    int num_args;
    safe_method_call(signature->named_args, array, &num_args, &args);
    for (int i = 0; i < num_args; i++) {
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, args[i]->type, &type_copy);
        size_t arg_len = strlen(args[i]->name);
        VarType *prev_type = NULL;
        safe_method_call(data->symbols,
                         put,
                         args[i]->name,
                         arg_len,
                         type_copy,
                         &prev_type);
        if (prev_type != NULL) {
            //TODO: Handle argument type conflict with outer scope variable
            free_VarType(prev_type);
        }
    }
    free(args);
    assigned_vars->clear(assigned_vars, NULL);
    VarType *prev_scope = state->scope_type;

    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(data->stmts, array, &size, &stmt_arr);
    state->scope_type = data->type;
    int err = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        err = vtable->get_type(stmt_arr[i], data->symbols, &type) || err;
    }
    free(stmt_arr);
    if (err) return 1;
    state->scope_type = prev_scope;
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
