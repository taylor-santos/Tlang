#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"

static const ASTNode *in_class;
static const Vector *assigned_vars; // Use this to store any variables that will
                                    // be assigned to an object with scope. They
                                    // will then be visible within that scope as
                                    // instances of itself.

static int funccmp(const FuncType *type1, const FuncType *type2);
static int copy_FuncType(FuncType *type, FuncType **copy_ptr);
static int copy_ClassType(ClassType *type, ClassType **copy_ptr);
static int copy_VarType(const void *type, const void *copy_ptr);

typedef struct {
    const Vector *assigned_vars;
    const ASTNode *program_node;
    const Vector *new_symbols;
} TypeCheckState;

int add_builtins(const Map *map) {
    if (map == NULL) {
        return 1;
    }
    size_t n;
    const char *binops[] = { "+" };
    n = sizeof(binops) / sizeof(*binops);
    for (size_t i = 0; i < n; i++) {
        VarType *arg_type = NULL;
        safe_function_call(new_VarType, "int", &arg_type);
        NamedType *arg = NULL;
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
        char *name;
        asprintf(&name, "var_0x%X", binops[i][0]);
        size_t len = strlen(binops[i]);
        for (size_t j = 1; j < len; j++) {
            append_string(&name, "%X", binops[i][j]);
        }
        safe_method_call(map,
                         put,
                         name,
                         strlen(name),
                         func_type,
                         NULL);
        free(name);
    }
    const char *builtins[] = { "var_println" };
    n = sizeof(builtins) / sizeof(*builtins);
    for (size_t i = 0; i < n; i++) {
        VarType *arg_type = NULL;
        safe_function_call(new_VarType, "int", &arg_type);
        NamedType *arg = NULL;
        safe_function_call(new_NamedArg, strdup("val"), arg_type, &arg);
        const Vector *args = new_Vector(0);
        safe_method_call(args, append, arg);
        VarType *ret_type = NULL;
        safe_function_call(new_NoneType, &ret_type);
        FuncType *func = NULL;
        safe_function_call(new_FuncType, args, ret_type, &func);
        VarType *func_type = NULL;
        safe_function_call(new_VarType_from_FuncType, func, &func_type);
        safe_method_call(map,
                         put,
                         builtins[i],
                         strlen(builtins[i]),
                         func_type,
                         NULL);
    }
    return 0;
}

void free_VarType(void *this) {
    if (this == NULL) return;
    VarType *type = this;
    switch(type->type) {
        case FUNCTION:
            free_FuncType(type->function);
            break;
        case CLASS:
            free_ClassType(type->class);
            break;
        case REFERENCE:
        case RETURN:
        case HOLD:
            free_VarType(type->sub_type);
            break;
        case BUILTIN:
        case NONE:
            break;
    }
    free(this);
}

void free_FuncType(void *this) {
    FuncType *type = this;
    type->named_args->free(type->named_args, free_NamedType);
    free_VarType(type->ret_type);
    if (type->extension != NULL) {
        free_NamedType(type->extension);
    }
    free(this);
}

void free_ClassType(void *this) {
    ClassType *type = this;
    type->fields->free(type->fields, free_NamedType);
    free(this);
}

void free_NamedType(void *this) {
    NamedType *arg = this;
    free(arg->name);
    free_VarType(arg->type);
    free(this);
}

int new_VarType(const char *type, VarType **vartype_ptr) {
    if (strcmp(type, "int") == 0) {
        *vartype_ptr = malloc(sizeof(**vartype_ptr));
        (*vartype_ptr)->type = BUILTIN;
        (*vartype_ptr)->builtin = INT;
        (*vartype_ptr)->is_ref = 0;
        return 0;
    } else if (strcmp(type, "double") == 0) {
        *vartype_ptr = malloc(sizeof(**vartype_ptr));
        (*vartype_ptr)->type = BUILTIN;
        (*vartype_ptr)->builtin = DOUBLE;
        (*vartype_ptr)->is_ref = 0;
        return 0;
    } else {
        //TODO: Create VarTypes from classes
        fprintf(stderr, "error: invalid type statement\n");
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
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_RefType(VarType **vartype_ptr, VarType *sub_type) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = REFERENCE;
    (*vartype_ptr)->sub_type = sub_type;
    (*vartype_ptr)->is_ref = 0;
    sub_type->is_ref = 1;
    return 0;
}

int new_HoldType(VarType **vartype_ptr, VarType *sub_type) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = HOLD;
    (*vartype_ptr)->sub_type = sub_type;
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_ClassType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = CLASS;
    (*vartype_ptr)->class = malloc(sizeof(ClassType));
    if ((*vartype_ptr)->class == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->class->fields = new_Vector(0);
    if ((*vartype_ptr)->class->fields == NULL) {
        free((*vartype_ptr)->class);
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_ReturnType(VarType **vartype_ptr, VarType *sub_type) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = RETURN;
    (*vartype_ptr)->sub_type = sub_type;
    (*vartype_ptr)->is_ref = 0;
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
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_NamedArg(char *name, VarType *type, NamedType **namedarg_ptr) {
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
                NamedType *arg = NULL;
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
            printf("ref ");
            print_VarType(type->sub_type);
            break;
        case HOLD:
            printf("hold ");
            print_VarType(type->sub_type);
            break;
        case CLASS:
            printf("class");
            break;
        case RETURN:
            printf("return ");
            print_VarType(type->sub_type);
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
    (*VT_copy_ptr)->is_ref = VT_type->is_ref;
    switch(VT_type->type) {
        case BUILTIN:
            (*VT_copy_ptr)->builtin = VT_type->builtin;
            break;
        case FUNCTION:
            safe_function_call(copy_FuncType,
                               VT_type->function,
                               &(*VT_copy_ptr)->function);
            break;
        case CLASS:
            safe_function_call(copy_ClassType,
                               VT_type->class,
                               &(*VT_copy_ptr)->class);
            break;
        case RETURN:
        case REFERENCE:
        case HOLD:
            safe_function_call(copy_VarType,
                               VT_type->sub_type,
                               &(*VT_copy_ptr)->sub_type);
            break;
        case NONE:
            break;
    }
    return 0;
}

static int copy_NamedType(const void *this, const void *copy_ptr) {
    NamedType *arg = (NamedType*)this;
    NamedType **copy = (NamedType**)copy_ptr;
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
                     copy_NamedType);
    safe_function_call(copy_VarType, type->ret_type, &(*copy_ptr)->ret_type);
    if (type->extension != NULL) {
        safe_function_call(copy_NamedType,
                           type->extension,
                           &(*copy_ptr)->extension);
    } else {
        (*copy_ptr)->extension = NULL;
    }
    return 0;
}

static int copy_ClassType(ClassType *type, ClassType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    safe_method_call(type->fields,
                     copy,
                     &(*copy_ptr)->fields,
                     copy_NamedType);
    return 0;
}

static int typecmp(const VarType *type1, const VarType *type2) {
    //Returns 1 if the types differ, 0 if they match
    if (type1->type == REFERENCE && type2->is_ref == 1) {
        type1 = type1->sub_type;
    }
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
        case CLASS:
            return 0;
        case REFERENCE:
            return typecmp(type1->sub_type, type1->sub_type);
        case RETURN:
            return 1;
        case HOLD:
            return 0;
    }
    fprintf(stderr, "%s:%d: " RED "internal compiler error: " WHITE "switch "
            "statement failed to cover all cases\n", __FILE__, __LINE__);
    return 1;
}

static int funccmp(const FuncType *type1, const FuncType *type2) {
    int size1, size2, ret = 0;
    NamedType **arr1 = NULL, **arr2 = NULL;
    safe_method_call(type1->named_args, array, &size1, &arr1);
    safe_method_call(type2->named_args, array, &size2, &arr2);
    if (size1 != size2) {
        ret = 1;
    } else {
        for (int i = 0; i < size1; i++) {
            if (typecmp(arr2[i]->type, arr1[i]->type)) {
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
    TypeCheckState state = {
            .assigned_vars = new_Vector(0),
            .new_symbols = new_Vector(0),
            .program_node = this
    };
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(stmts, array, &size, &stmt_arr);
    int errstate = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        errstate = vtable->get_type(stmt_arr[i], data->symbols, &state, &type)
                   || errstate;
    }
    free(stmt_arr);
    state.assigned_vars->free(state.assigned_vars, free);
    state.new_symbols->free(state.new_symbols, free_NamedType);
    return errstate;
}

int GetType_Assignment(const void *this,
                       const Map *symbols,
                       const void *typecheck_state,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode     *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTExpressionVTable *lhs_vtable = lhs->vtable;
    ASTAssignmentVTable *rhs_vtable = rhs->vtable;

    *type_ptr = NULL;
    if (rhs_vtable->get_type(rhs, symbols, typecheck_state, type_ptr)) {
        return 1;
    }
    data->type = *type_ptr;
    if (*type_ptr == NULL) {
        return 1;
    }
    VarType *lhs_type;
    if (lhs_vtable->get_type(lhs, symbols, typecheck_state, &lhs_type)) {
        //TODO: Handle type reassignment error
        fprintf(stderr, "error: type reassignment\n");
        return 1;
    }
    if (typecmp(lhs_type, *type_ptr)) {
        //TODO: Handle assignment type incompatibility
        fprintf(stderr, "error: incompatible type in assignment\n");
        return 1;
    }
    return 0;
}

int GetType_Def(const void *this,
                const Map *symbols,
                const void *typecheck_state,
                VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTDefData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTLExprVTable *lhs_vtable = lhs->vtable;
    ASTDefVTable *rhs_vtable = rhs->vtable;
    if (assigned_vars == NULL) {
        assigned_vars = new_Vector(0);
    }
    lhs_vtable->get_vars(lhs, assigned_vars);
    *type_ptr = NULL;
    if (rhs_vtable->get_type(rhs, symbols, typecheck_state, type_ptr)) {
        return 1;
    }
    char **vars = NULL;
    int var_count;
    safe_method_call(assigned_vars, array, &var_count, &vars);
    const TypeCheckState *state = typecheck_state;
    for (int i = 0; i < var_count; i++) {
        VarType *prev_type;
        if (!symbols->get(symbols, vars[i], strlen(vars[i]), &prev_type)) {
            if (typecmp(prev_type, *type_ptr)) {
                //TODO: Handle type redef
                fprintf(stderr, "error: type reassignment\n");
                return 1;
            }
        } else {
            VarType *type_copy;
            safe_function_call(copy_VarType, *type_ptr, &type_copy);
            safe_method_call(symbols,
                             put,
                             vars[i],
                             strlen(vars[i]),
                             type_copy,
                             NULL);
        }
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, *type_ptr, &type_copy);
        char *name = NULL;
        strdup_safe(vars[i], &name);
        NamedType *new_symbol;
        safe_function_call(new_NamedArg, name, type_copy, &new_symbol);
        safe_method_call(state->new_symbols, append, new_symbol);
    }
    free(vars);
    data->type = *type_ptr;
    assigned_vars->clear(assigned_vars, free);
    if (*type_ptr == NULL) {
        return 1;
    }
    if (lhs_vtable->assign_type(lhs, *type_ptr, symbols)) {
        //TODO: Handle type redef error
        fprintf(stderr, "error: type reassignment\n");
        return 1;
    }
    return 0;
}

int GetType_Return(UNUSED const void *this,
                   UNUSED const Map *symbols,
                   const void *typecheck_state,
                   VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    const ASTNode *ret_expr = data->returns;
    VarType *ret_type = NULL;
    if (ret_expr != NULL) {
        ASTRExprVTable *ret_vtable = ret_expr->vtable;

        if (ret_vtable->get_type(ret_expr, symbols, typecheck_state, &ret_type)) {
            //TODO: Handle failed to infer type of returned value
            fprintf(stderr,
                    "error: failed to infer type of returned value\n");
            return 1;
        } else {
            VarType *type_copy = NULL;
            safe_function_call(copy_VarType, ret_type, &type_copy);
            safe_function_call(new_ReturnType, &(data->type), type_copy);
            *type_ptr = data->type;
            return 0;
        }
    } else {
        safe_function_call(new_NoneType, &(data->type));
        *type_ptr = data->type;
        return 0;
    }
}

VarType *parse_expression(const ASTNode **nodes,
                          int size,
                          const Map *symbols,
                          const void *typecheck_state,
                          Expression **expr_ptr) {
    const ASTNode *node = nodes[0], *arg_node;
    ASTStatementVTable *vtable = node->vtable, *arg_vtable;
    VarType *node_type = NULL, *arg_type, *ret_type = NULL;
    NamedType *expected_type = NULL;
    FuncType *function;
    int arg_count, i;

    if (vtable->get_type(node, symbols, typecheck_state, &node_type)) return NULL;
    *expr_ptr = malloc(sizeof(**expr_ptr));
    if (*expr_ptr == NULL) {
        print_ICE("unable to allocate memory");
        return NULL;
    }
    (*expr_ptr)->node = node;
    switch(node_type->type) {
        case FUNCTION:
            function = node_type->function;
            (*expr_ptr)->type = EXPR_FUNC;
            (*expr_ptr)->args = new_Vector(0);
            (*expr_ptr)->extension = NULL;
            arg_count = function->named_args->size(function->named_args);
            if (arg_count != size - 1) {
                //TODO: Handle incorrect argument count
                fprintf(stderr,
                        "error: wrong number of arguments given\n");
                return NULL;
            }
            for (i = 0; i < arg_count; i++) {
                arg_node = nodes[i + 1];
                arg_vtable = arg_node->vtable;
                if (arg_vtable->get_type(arg_node,
                                         symbols,
                                         typecheck_state,
                                         &arg_type)) {
                    return NULL;
                }
                safe_method_call(function->named_args, get, i, &expected_type);
                if (typecmp(arg_type, expected_type->type) ||
                    (expected_type->type->is_ref
                    && arg_type->type != REFERENCE)) {
                    //TODO: Handle incompatible argument type
                    fprintf(stderr,
                            "error: incompatible argument type\n");
                    return NULL;
                }
                safe_method_call((*expr_ptr)->args, append, arg_node);
            }
            if (function->extension == NULL) {
                ret_type = function->ret_type;
            } else {
                ret_type = node_type;
            }
            return ret_type;
        case CLASS:
            (*expr_ptr)->type = EXPR_CLASS;
            ret_type = node_type;
            break;
        case BUILTIN:
            (*expr_ptr)->type = EXPR_VALUE;
            ret_type = node_type;
            break;
        case REFERENCE:
            (*expr_ptr)->type = EXPR_VALUE;
            ret_type = node_type;
            return ret_type;
        case HOLD:
            (*expr_ptr)->type = EXPR_VALUE;
            ret_type = node_type->sub_type;
            break;
        case NONE:
            //TODO: Handle none in expression
            fprintf(stderr, "error: invalid none in expression\n");
            return NULL;
        case RETURN:
            //TODO: Handle return in expression
            fprintf(stderr, "error: invalid return in expression\n");
            return NULL;
    }
    if (size > 1) {
        Expression *func_expr;
        VarType *new_type = parse_expression(nodes + 1,
                                             size - 1,
                                             symbols,
                                             typecheck_state,
                                             &func_expr);
        if (new_type == NULL) return NULL;
        if (new_type->type == REFERENCE) {
            new_type = new_type->sub_type;
        }
        if (new_type->type != FUNCTION
            || new_type->function->extension == NULL) {
            //TODO: Handle 2nd argument isn't an extension function
            fprintf(stderr,
                    "error: expression is not an extension function\n");
            return NULL;
        }
        NamedType *ext = new_type->function->extension;
        if (typecmp(ret_type, ext->type)) {
            //TODO: Handle incompatible argument type
            fprintf(stderr,
                    "error: incompatible argument type\n");
            return NULL;
        }
        func_expr->extension = node;
        free(*expr_ptr);
        *expr_ptr = func_expr;
        ret_type = new_type->function->ret_type;
    }
    return ret_type;
}

int GetType_Expression(const void *this,
                       const Map *symbols,
                       const void *typecheck_state,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode     *node   = this;
    ASTExpressionData *data   = node->data;
    const Vector      *exprs  = data->exprs;
    const ASTNode     **nodes = NULL;
    Expression        *expr;

    int size = exprs->size(exprs);
    safe_method_call(exprs, array, &size, &nodes);
    *type_ptr = parse_expression(nodes, size, symbols, typecheck_state, &expr);
    if (*type_ptr == NULL) {
        free(nodes);
        return 1;
    }
    data->expr_node = expr;
    data->type = *type_ptr;
    free(nodes);
    return 0;
}

int GetType_Ref(const void *this,
                UNUSED const Map *symbols,
                const void *typecheck_state,
                VarType **type_ptr) {
    const ASTNode *node;
    ASTRefData *data;
    ASTStatementVTable *vtable;

    if (type_ptr == NULL) {
        return 1;
    }
    node = this;
    data = node->data;
    vtable = data->expr->vtable;
    VarType *ref_type;
    if (vtable->get_type(data->expr, symbols, typecheck_state, &ref_type)) {
        return 1;
    }
    if (ref_type->type == REFERENCE) {
        //TODO: Handle reference to reference
        fprintf(stderr, "error: reference to reference not allowed\n");
        return 1;
    }
    VarType *type_copy = NULL;
    safe_function_call(copy_VarType, ref_type, &type_copy);
    safe_function_call(new_RefType, &data->type, type_copy);
    *type_ptr = data->type;
    return 0;
}

int GetType_Paren(const void *this,
                  UNUSED const Map *symbols,
                  const void *typecheck_state,
                  VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode      *node   = this;
    ASTParenData       *data   = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    int result =  vtable->get_type(data->val, symbols, typecheck_state, type_ptr);
    data->type = *type_ptr;
    return result;
}

int GetType_Hold(const void *this,
                UNUSED const Map *symbols,
                const void *typecheck_state,
                VarType **type_ptr) {
    const ASTNode *node;
    ASTHoldData *data;
    ASTStatementVTable *vtable;

    if (type_ptr == NULL) {
        return 1;
    }
    node = this;
    data = node->data;
    vtable = data->val->vtable;
    VarType *type;
    if (vtable->get_type(data->val, symbols, typecheck_state, &type)) {
        return 1;
    }
    VarType *type_copy = NULL;
    safe_function_call(copy_VarType, type, &type_copy);
    safe_function_call(new_HoldType, type_ptr, type_copy);
    data->type = *type_ptr;
    return 0;
}

int GetType_Variable(const void *this,
                     const Map *symbols,
                     UNUSED const void *typecheck_state,
                     VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, type_ptr);
        data->type = *type_ptr;
        return 0;
    } else {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
        return 1;
    }
}

int GetType_TypedVar(const void *this,
                     const Map *symbols,
                     const void *typecheck_state,
                     VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    if (assigned_vars != NULL) {
        char *name = NULL;
        strdup_safe(data->name, &name);
        safe_method_call(assigned_vars, append, name);
    }
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, type_ptr);
        if (typecmp(*type_ptr, data->given_type)) {
            return 1;
        }
        data->type = *type_ptr;
    } else {
        *type_ptr = data->given_type;
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, *type_ptr, &type_copy);
        safe_method_call(symbols, put, data->name, len, type_copy, NULL);
        data->type = *type_ptr;
    }
    VarType *type_copy = NULL;
    safe_function_call(copy_VarType, data->type, &type_copy);
    char *name = NULL;
    strdup_safe(data->name, &name);
    NamedType *new_symbol;
    safe_function_call(new_NamedArg, name, type_copy, &new_symbol);
    const TypeCheckState *state = typecheck_state;
    safe_method_call(state->new_symbols, append, new_symbol);
    return 0;
}

int GetType_Int(UNUSED const void *this,
                UNUSED const Map *symbols,
                UNUSED const void *typecheck_state,
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
                   UNUSED const void *typecheck_state,
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
                     const void *typecheck_state,
                     VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    safe_method_call(symbols, copy, &data->symbols, copy_VarType);
    safe_method_call(data->symbols, copy, &data->env, copy_VarType);
    data->self = new_Map(0, 0);
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
            free_VarType(prev_type);
        }
        void *prev = NULL;
        safe_method_call(data->self, put, var, len, NULL, &prev);
    }
    if (data->signature->extension != NULL) {
        VarType *type_copy = NULL;
        if (data->signature->extension->type->type == REFERENCE) {
            safe_function_call(copy_VarType,
                               data->signature->extension->type->sub_type,
                               &type_copy);
        } else {
            safe_function_call(copy_VarType,
                               data->signature->extension->type,
                               &type_copy);
        }
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
    NamedType **args = NULL;
    int num_args;
    safe_method_call(signature->named_args, array, &num_args, &args);
    data->args = new_Map(0, 0);
    for (int i = 0; i < num_args; i++) {
        VarType *type_copy = NULL;
        if (args[i]->type->type == REFERENCE) {
            safe_function_call(copy_VarType,
                               args[i]->type->sub_type,
                               &type_copy);
        } else {
            safe_function_call(copy_VarType, args[i]->type, &type_copy);
        }

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
        safe_method_call(data->args, put, args[i]->name, arg_len, NULL, NULL);
    }
    if (signature->extension != NULL) {
        char *name = signature->extension->name;
        size_t len = strlen(name);
        safe_method_call(data->args, put, name, len, NULL, NULL);
    }
    free(args);
    assigned_vars->clear(assigned_vars, free);
    VarType *ret_type = signature->ret_type;
    int stmt_count = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(data->stmts, array, &stmt_count, &stmt_arr);
    int err = 0;
    for (int i = 0; i < stmt_count; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        err = vtable->get_type(stmt_arr[i], data->symbols, typecheck_state, &type)
              || err;
        if (type && type->type == RETURN) {
            // Compare stated function return type to type of return statement
            if (typecmp(type->sub_type, ret_type)) {
                //TODO: Handle incompatible return statement
                fprintf(stderr, "error: incompatible return type\n");
                err = 1;
            }
        }
    }
    free(stmt_arr);
    if (err) return 1;
    *type_ptr = data->type;
    const TypeCheckState *state = typecheck_state;
    const ASTNode *program_ast = state->program_node;
	ASTProgramData *program_data = program_ast->data;
	int *n = malloc(sizeof(int));
	if (n == NULL) {
	    print_ICE("unable to allocate memory");
	}
	*n = program_data->func_defs->size(program_data->func_defs);
    safe_method_call(program_data->func_defs,
                     put,
                     this,
                     sizeof(this),
                     n,
                     NULL);
    return 0;
}

int GetType_Class(const void *this,
                  const Map *symbols,
                  const void *typecheck_state,
                  VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    const ASTNode *node = this;
    ASTClassData  *data = node->data;

    for (int i = 0; i < assigned_vars->size(assigned_vars); i++) {
        char *var = NULL;
        safe_method_call(assigned_vars, get, i, &var);
        size_t len = strlen(var);
        safe_method_call(data->self, put, var, len, NULL, NULL);
    }
    assigned_vars->clear(assigned_vars, free);
    safe_function_call(new_ClassType, &data->type);
    *type_ptr = data->type;
    char **parents = NULL;
    int parent_count;
    safe_method_call(data->inheritance, array, &parent_count, &parents);
    for (int i = 0; i < parent_count; i++) {
        VarType *parent_type = NULL;
        if (symbols->get(symbols,
                         parents[i],
                         strlen(parents[i]),
                         &parent_type)) {
            //TODO: handle inheritance from unrecognized type
            fprintf(stderr,
                    "error: class inherits from unrecognized type\n");
            return 1;
        }
        if (parent_type->type != CLASS) {
            //TODO: Handle inheritance from non-class
            fprintf(stderr, "error: class inherits from non-class");
            return 1;
        }
        ClassType *parent_class = parent_type->class;
        int field_count = parent_class->fields->size(parent_class->fields);
        for (int j = 0; j < field_count; j++) {
            NamedType *field = NULL;
            safe_method_call(parent_class->fields, get, j, &field);
            NamedType *field_copy = NULL;
            safe_function_call(copy_NamedType, field, &field_copy);
            safe_method_call(data->type->class->fields, append, field_copy);
        }
    }
    free(parents);
    const Vector  *stmts = data->stmts;
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(stmts, array, &size, &stmt_arr);
    const ASTNode *prev_class = in_class;
    in_class = node;
    int errstate = 0;
    const TypeCheckState *state = typecheck_state;
    state->new_symbols->clear(state->new_symbols, free_NamedType);
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        errstate = vtable->get_type(stmt_arr[i],
                                    data->symbols,
                                    typecheck_state,
                                    &type)
                   || errstate;
    }
    free(stmt_arr);
    int num_fields = state->new_symbols->size(state->new_symbols);
    for (int i = 0; i < num_fields; i++) {
        NamedType *field = NULL;
        safe_method_call(state->new_symbols, get, i, &field);
        safe_method_call(data->type->class->fields, append, field);
    }
    //Clear the new_symbols vector without freeing its elements. They have been
    //moved to the class's fields list.
    state->new_symbols->clear(state->new_symbols, NULL);
    in_class = prev_class;
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int *n = malloc(sizeof(int));
    if (n == NULL) {
        print_ICE("unable to allocate memory");
    }
    *n = program_data->class_defs->size(program_data->class_defs);
    safe_method_call(program_data->class_defs,
                     put,
                     this,
                     sizeof(this),
                     n,
                     NULL);
    return 0;
}

int AssignType_Variable(const void *this, VarType *type, const Map *symbols) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    VarType *put_type = NULL;
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, &put_type);
        if (typecmp(put_type, type)) {
            //TODO: Handle type mismatch error
            return 1;
        }
    } else {
        safe_function_call(copy_VarType, type, &put_type);
        safe_method_call(symbols, put, data->name, len, put_type, NULL);
    }
    data->type = put_type; // Store the inferred type for later
    if (in_class != NULL) {
        ASTClassData *class_data = in_class->data;
        const Map *fields = class_data->fields;
        VarType *prev_type = NULL;
        safe_method_call(fields, put, data->name, len, put_type, &prev_type);
        if (prev_type != NULL) {
            //TODO: Handle field reassignment
            fprintf(stderr, "error: multiple assignments to field");
            return 1;
        }
    }
    return 0;
}

int AssignType_TypedVar(const void *this, VarType *type, const Map *symbols) {
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    VarType *put_type = NULL;
    // Before comparing, check if it's in the symbol table. Add it if not.
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, &put_type);
    } else {
        //safe_function_call(copy_VarType, type, &put_type);
        put_type = type;
        safe_method_call(symbols, put, data->name, len, put_type, NULL);
    }
    if (typecmp(put_type, type)) {
        //TODO: Handle type mismatch error
        return 1;
    }
    data->type = put_type; // Store the inferred type for later
    if (in_class != NULL) {
        ASTClassData *class_data = in_class->data;
        const Map *fields = class_data->fields;
        VarType *prev_type = NULL;
        safe_method_call(fields, put, data->name, len, put_type, &prev_type);
        if (prev_type != NULL) {
            //TODO: Handle field reassignment
            fprintf(stderr, "error: multiple assignments to field");
            return 1;
        }
    }
    return 0;
}

int GetVars_Assignment(const void *this, const Vector *vars) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    ASTStatementVTable *vtable = lhs->vtable;
    return vtable->get_vars(lhs, vars);
}

int GetVars_Def(const void *this, const Vector *vars) {
    const ASTNode *node = this;
    ASTDefData *data = node->data;
    const ASTNode *lhs = data->lhs;
    ASTStatementVTable *vtable = lhs->vtable;
    return vtable->get_vars(lhs, vars);
}

int GetVars_Return(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Expression(const void *this, const Vector *vars) {
    int expr_count;
    const ASTNode *node = this, **nodes = NULL;
    ASTExpressionData *data = node->data;
    safe_method_call(data->exprs, array, &expr_count, &nodes);
    for (int i = 0; i < expr_count; i++) {
        const ASTNode *expr_node = nodes[i];
        ASTStatementVTable *vtable = expr_node->vtable;
        safe_function_call(vtable->get_vars, expr_node, vars);
    }
    return 0;
}

int GetVars_Ref(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Paren(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Hold(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Variable(const void *this, const Vector *vars) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    safe_method_call(vars, append, strdup(data->name));
    return 0;
}

int GetVars_TypedVar(const void *this, const Vector *vars) {
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    safe_method_call(vars, append, strdup(data->name));
    return 0;
}

int GetVars_Int(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Double(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Function(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}

int GetVars_Class(UNUSED const void *this, UNUSED const Vector *vars) {
    return 0;
}
