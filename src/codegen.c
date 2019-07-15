#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"
#include "safe.h"
#include "typechecker.h"
#include "queue.h"

static CodeGenState state;
extern const Map *func_defs;

static void fprintf_code(FILE *out, const char *fmt, ...) {
    va_list args;
    safe_fprintf(out, "%*s", state.indent * INDENT_WIDTH, "");
    va_start(args, fmt);
    safe_vfprintf(out, fmt, args);
    va_end(args);
}

static int print_signature(FILE *out, VarType *type, const char *name);

static int print_function_args(FILE *out, VarType *func_type) {
    safe_fprintf(out, "(void*");
    if (func_type->function->extension != NULL) {
        safe_fprintf(out, ", ");
        safe_function_call(print_signature,
                           out,
                           func_type->function->extension->type,
                           NULL);
    }
    int count;
    const NamedArg **args = NULL;
    safe_method_call(func_type->function->named_args, array, &count, &args);
    for (int i = 0; i < count; i++) {
        safe_fprintf(out, ", ");
        safe_function_call(print_signature, out, args[i]->type, NULL);
    }
    safe_fprintf(out, ")");
    return 0;
}

static int print_function_signature(FILE *out,
                                    VarType *type,
                                    const char *name,
                                    const Queue *func_queue) {
    VarType *ret_type = type->function->ret_type;
    if (ret_type != NULL && ret_type->type == FUNCTION) {
        safe_method_call(func_queue, push, type);
        return print_function_signature(out, ret_type, name, func_queue);
    } else {
        safe_function_call(print_signature, out, ret_type, NULL);
        safe_fprintf(out, " ");
        int nest_count = func_queue->size(func_queue);
        for (int i = 0; i < nest_count; i++) {
            safe_fprintf(out, "(*");
        }
        safe_fprintf(out, "(*");
        if (name != NULL) {
            safe_fprintf(out, "%s", name);
        }
        safe_fprintf(out, ")");
        safe_method_call(func_queue, push, type);
        while (func_queue->size(func_queue) > 0){
            VarType *curr_type = NULL;
            safe_method_call(func_queue, pop, &curr_type);
            safe_fprintf(out, "(void*");
            if (curr_type->function->extension != NULL) {
                safe_fprintf(out, ", ");
                safe_function_call(print_signature,
                                   out,
                                   curr_type->function->extension->type,
                                   NULL);
            }
            int count;
            const NamedArg **args = NULL;
            safe_method_call(curr_type->function->named_args,
                array,
                &count,
                &args);
            for (int i = 0; i < count; i++) {
                safe_fprintf(out, ", ");
                safe_function_call(print_signature, out, args[i]->type,
                    NULL);
            }
            free(args);
            safe_fprintf(out, ")");
            if (func_queue->size(func_queue) != 0) {
                safe_fprintf(out, ")");
            }
        }
        return 0;
    }
}

static int print_signature(FILE *out, VarType *type, const char *name) {
    switch(type->type) {
        case BUILTIN:
            switch(type->builtin) {
                case INT:
                    safe_fprintf(out, "int");
                    if (name != NULL) {
                        safe_fprintf(out, " %s", name);
                    }
                    return 0;
                case DOUBLE:
                    safe_fprintf(out, "double");
                    if (name != NULL) {
                        safe_fprintf(out, " %s", name);
                    }
                    return 0;
            }
            return 1;
        case NONE:
            safe_fprintf(out, "void");
            return 0;
        case FUNCTION:;
            const Queue *func_queue = new_Queue(0);
            safe_function_call(print_function_signature,
                               out,
                               type,
                               name,
                               func_queue);
            func_queue->free(func_queue, NULL);
            return 0;
    }
    return 1;
}

static void declare_type(FILE *out, VarType *type, const char *name) {
    fprintf_code(out, "");
    safe_function_call(print_signature, out, type, name);
    safe_fprintf(out, ";\n");
}

static int declare_functions(FILE *out, const Map *locals) {
    int size = 0;
    size_t *lengths = NULL;
    VarType ***keys = NULL;
    safe_method_call(func_defs, keys, &size, &lengths, &keys);
    for (int i = 0; i < size; i++) {
        VarType *func_type = *keys[i];
        size_t ID =
        safe_method_call(state.function_IDs,
            put,
            &func_type,
            sizeof(func_type),
            (size_t)i,
            NULL);
        VarType *ret_type = func_type->function->ret_type;
        fprintf_code(out, "typedef ");
        if (ret_type->type == FUNCTION) {
            safe_fprintf(out, "struct {\n");
            state.indent++;
            fprintf_code(out, "");
            print_signature(out, ret_type, "func");
            safe_fprintf(out, ";\n");
            fprintf_code(out, "void *env;\n", i);
            state.indent--;
            fprintf_code(out, "} *");
        } else {
            print_signature(out, ret_type, NULL);
            safe_fprintf(out, " ");
        }
        safe_fprintf(out, "func%d_ret;\n", i);
        fprintf_code(out, "typedef struct {\n");
        state.indent++;
        fprintf_code(out, "func%d_ret (*func)", i);
        safe_function_call(print_function_args, out, func_type);
        safe_fprintf(out, ";\n");
        fprintf_code(out, "void *env;\n");
        state.indent--;
        fprintf_code(out, "} closure%d;\n", i);
        fprintf_code(out, "func%d_ret func%d", i, i);
        safe_function_call(print_function_args, out, func_type);
        safe_fprintf(out, ";\n\n");
    }
    return 0;
}

static int print_env(FILE *out, const Map *symbols) {
    int size = 0;
    size_t *lengths = NULL;
    char **keys = NULL;
    safe_method_call(symbols, keys, &size, &lengths, &keys);
    for (int i = 0; i < size; i++) {
        VarType *type = NULL;
        safe_method_call(symbols, get, keys[i], lengths[i], &type);
        if (type->type == FUNCTION) {
            int funcID
        }
        fprintf_code(out, "%*s\n", lengths[i], keys[i]);
    }
    return 0;
}

static int define_envs(FILE *out, const Map *globals) {
    int size = 0;
    size_t *lengths = NULL;
    VarType ***keys = NULL;
    safe_method_call(func_defs, keys, &size, &lengths, &keys);
    for (int i = 0; i < size; i++) {
        ASTNode *node = NULL;
        safe_method_call(func_defs, get, keys[i], lengths[i], &node);
        fprintf_code(out, "typedef struct {\n");
        state.indent++;
        ASTFunctionData *data = node->data;
        safe_function_call(print_env, out, data->symbols);
        state.indent--;
        fprintf_code(out, "} func%d_locals;\n\n", i);
    }
    fprintf_code(out, "struct globals {\n");
    state.indent++;
    safe_function_call(print_env, out, globals);
    state.indent--;
    fprintf_code(out, "} globals;\n\n");
    return 0;
}

static int define_functions(FILE *out, const Map *locals) {
    int size = 0;
    size_t *lengths = NULL;
    VarType **sigs = NULL;
    safe_method_call(func_defs, keys, &size, &lengths, &sigs);
    for (int i = 0; i < size; i++) {
        ASTNode *node = NULL;
        safe_method_call(func_defs, get, sigs[i], lengths[i], &node);
        ASTFunctionData *data = node->data;
        FuncType *signature = data->type->function;
        char *name;
        safe_asprintf(&name, "func%d_ret", (int)state.func_count);
        fprintf_code(out, "typedef ");
        print_signature(out, signature->ret_type, name);
        safe_fprintf(out, ";\n");
        fprintf_code(out, "%s ", name);
        free(name);
        safe_method_call(state.defined_funcs,
                         put,
                         sigs[i],
                         lengths[i],
                         (void*)state.func_count,
                         NULL);
        safe_fprintf(out, "func%d(", (int)state.func_count);

        int arg_count = 0;
        NamedArg **args = NULL;
        safe_method_call(signature->named_args, array, &arg_count, &args);
        const Map *vars_copy = NULL;
        safe_method_call(state.declared_vars, copy, &vars_copy, NULL);
        const Map *prev_vars = state.declared_vars;
        state.declared_vars = vars_copy;
        char *sep = ", ";
        if (signature->extension != NULL) {
            NamedArg *arg = signature->extension;
            print_signature(out, arg->type, arg->name);
            sep = ", ";
            VarType *prev_type = NULL;
            safe_method_call(state.declared_vars,
                             put,
                             arg->name,
                             strlen(arg->name),
                             (void*)state.local_count,
                             &prev_type);
        }
        for (int j = 0; j < arg_count; j++) {
            NamedArg *arg = args[j];
            safe_fprintf(out, "%s", sep);
            print_signature(out, arg->type, arg->name);
            sep = ", ";
            VarType *prev_type = NULL;
            safe_method_call(state.declared_vars,
                             put,
                             args[j]->name,
                             strlen(args[j]->name),
                             (void*)state.local_count,
                             &prev_type);
        }
        safe_fprintf(out, ") {\n");
        state.indent++;
        fprintf_code(out, "struct locals%d {\n", state.local_count);
        state.indent++;
        if (signature->extension != NULL) {
            NamedArg *arg = signature->extension;
            fprintf_code(out, "");
            print_signature(out, arg->type, arg->name);
            safe_fprintf(out, ";\n");
        }
        for (int j = 0; j < arg_count; j++) {
            NamedArg *arg = args[j];
            fprintf_code(out, "");
            print_signature(out, arg->type, arg->name);
            safe_fprintf(out, ";\n");
        }
        state.indent--;
        fprintf_code(out, "} locals%d;\n", state.local_count);
        if (signature->extension != NULL) {
            fprintf_code(out,
                         "locals%d.%s = %s;\n",
                         state.local_count,
                         signature->extension->name,
                         signature->extension->name);
        }
        for (int j = 0; j < arg_count; j++) {
            NamedArg *arg = args[j];
            fprintf_code(out,
                         "locals%d.%s = %s;\n",
                         state.local_count,
                         arg->name,
                         arg->name);
        }
        state.local_count++;
        int stmt_count = 0;
        ASTNode **stmts = NULL;
        safe_method_call(data->stmts, array, &stmt_count, &stmts);
        for (int j = 0; j < stmt_count; j++) {
            ASTStatementVTable *vtable = stmts[j]->vtable;
            fprintf_code(out, "");
            vtable->codegen(stmts[j], out);
            safe_fprintf(out, ";\n");
        }
        state.declared_vars = prev_vars;
        state.indent--;
        fprintf_code(out, "}\n\n");
        free(sigs[i]);
        state.func_count++;
    }
    free(sigs);
    free(lengths);
    return 0;
}

static int define_builtin_funcs(FILE *out) {
    char binops[] = { '+', '-', '*', '/' };
    int num = sizeof(binops) / sizeof(*binops);
    for (int i = 0; i < num; i++) {
        char *func_name = NULL;
        safe_asprintf(&func_name, "chr_0x%X", binops[i]);
        fprintf_code(out, "int %s(int x, int y) {\n", func_name);
        state.indent++;
        fprintf_code(out, "return x %c y;\n", binops[i]);
        state.indent--;
        fprintf_code(out, "}\n");
    }
    char *builtins[] = { "var_println" };
    num = sizeof(builtins) / sizeof(*builtins);
    for (int i = 0; i < num; i++) {
        fprintf_code(out, "void %s(int val) {\n", builtins[i]);
        state.indent++;
        fprintf_code(out, "printf(\"%%d\\n\", val);\n");
        state.indent--;
        fprintf_code(out, "}\n");
    }

    return 0;
}

static int assign_builtin_funcs(FILE *out) {
    char binops[] = { '+', '-', '*', '/' };
    int num = sizeof(binops) / sizeof(*binops);
    for (int i = 0; i < num; i++) {
        char *func_name = NULL;
        safe_asprintf(&func_name, "chr_0x%X", binops[i]);
        fprintf_code(out, "locals0.%s = %s;\n", func_name, func_name);
    }
    char *builtins[] = { "var_println" };
    num = sizeof(builtins) / sizeof(*builtins);
    for (int i = 0; i < num; i++) {
        fprintf_code(out, "locals0.%s = %s;\n", builtins[i], builtins[i]);
    }
    return 0;
}

static int declare_locals(FILE *out, const Map *locals) {
    int size = 0;
    size_t *lengths = NULL;
    const char **keys = NULL;
    safe_method_call(locals, keys, &size, &lengths, &keys);
    if (size > 0) {
        fprintf_code(out, "struct locals%d {\n", state.local_count);
        state.indent++;
        for (int i = 0; i < size; i++) {
            VarType *type = NULL;
            safe_method_call(locals, get, keys[i], lengths[i], &type);
            VarType *prev_type = NULL;
            safe_method_call(state.declared_vars,
                             put,
                             keys[i],
                             lengths[i],
                             (void*)state.local_count,
                             &prev_type);
            if (prev_type != NULL) {
                //TODO: Handle multiple locals with same name
                fprintf(stderr, "error: multiple variables with same name");
                return 1;
            }
            char *name = malloc(lengths[i] + 1);
            if (name == NULL) {
                return 0;
            }
            memcpy(name, keys[i], lengths[i]);
            name[lengths[i]] = 0;
            if (type->type != FUNCTION) {
                declare_type(out, type, name);
            } else {
                size_t funcID = 0;
                safe_method_call(state.declared_func_sigs,
                                 get,
                                 &type,
                                 sizeof(type),
                                 &funcID);
                fprintf_code(out, "func%d_sig %s;\n", funcID, name);
            }
            free(name);
            free((void*)keys[i]);
        }
        state.indent--;
        safe_fprintf(out, "} locals%d;\n", (int)state.local_count);
        state.local_count++;
        free(keys);
        free(lengths);
    }
    return 0;
}


int CodeGen_Program(const void *this, FILE *out) {
    state.declared_vars      = new_Map(0, 0);
    state.declared_func_sigs = new_Map(0, 0);
    state.defined_funcs      = new_Map(0, 0);
    state.function_IDs       = new_Map(0, 0);
    state.indent = 0;
    state.local_count = 0;
    fprintf_code(out, "#include <stdlib.h>\n");
    fprintf_code(out, "#include <stdio.h>\n");
    fprintf_code(out, "\n");
    fprintf_code(out,
        "#define call(closure, args...) closure->func(closure->env, ##args)\n");
    safe_fprintf(out, "\n");
    const ASTNode  *node = this;
    ASTProgramData *data = node->data;
    safe_function_call(declare_functions, out, data->symbols);
    safe_function_call(define_envs, out, data->symbols);

    /*
    safe_function_call(declare_locals, out, data->symbols);
    safe_fprintf(out, "\n");
    safe_function_call(define_functions, out, data->symbols);
    safe_fprintf(out, "\n");
    safe_function_call(define_builtin_funcs, out);
    safe_fprintf(out, "\n");
    fprintf_code(out, "int main() {\n");
    state.indent++;
    assign_builtin_funcs(out);
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(data->statements, array, &size, &stmt_arr);
    int err = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        fprintf_code(out, "");
        err = vtable->codegen(stmt_arr[i], out) || err;
        safe_fprintf(out, ";\n");
    }
    free(stmt_arr);
    state.indent--;
    fprintf_code(out, "}\n");
     */
    return 0;
}

int CodeGen_Assignment(const void *this, FILE *out) {
    const ASTNode     *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode     *lhs_node = data->lhs;
    ASTLExprVTable    *lhs_vtable = lhs_node->vtable;
    if (lhs_vtable->codegen(lhs_node, out)) {
        return 1;
    }
    safe_fprintf(out, " = ");
    const ASTNode  *rhs_node   = data->rhs;
    ASTRExprVTable *rhs_vtable = rhs_node->vtable;
    if (rhs_vtable->codegen(rhs_node, out)) {
        return 1;
    }
    return 0;
}

int CodeGen_Return(const void *this, FILE *out) {
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    if (data->returns == NULL) {
        safe_fprintf(out, "return");
    } else {
        safe_fprintf(out, "return ");
        const ASTNode *return_node = data->returns;
        ASTStatementVTable *vtable = return_node->vtable;
        safe_function_call(vtable->codegen, return_node, out);
    }
    return 0;
}

int gen_expression(FILE *out, Expression *expr) {
    if (expr->type == EXPR_VALUE) {
        const ASTNode      *node = expr->node;
        ASTStatementVTable *vtable = node->vtable;
        safe_function_call(vtable->codegen, node, out);
        return 0;
    } else {
        const ASTNode      *node = expr->node;
        ASTStatementVTable *vtable = node->vtable;
        safe_function_call(vtable->codegen, node, out);
        safe_fprintf(out, "(");
        char *sep = "";
        if (expr->extension != NULL) {
            safe_function_call(
                gen_expression,
                out,
                (Expression*)expr->extension);
            sep = ", ";
        }
        int count = 0;
        Expression **args = NULL;
        safe_method_call(expr->args, array, &count, &args);
        for (int i = 0; i < count; i++) {
            safe_fprintf(out, "%s", sep);
            safe_function_call(gen_expression, out, args[i]);
            sep = ", ";
        }
        safe_fprintf(out, ")");
        return 0;
    }
}

int CodeGen_Expression(UNUSED const void *this, UNUSED FILE *out) {
    const ASTNode     *node  = this;
    ASTExpressionData *data  = node->data;
    Expression *expr = data->expr_node;
    return gen_expression(out, expr);
}

int CodeGen_Ref(UNUSED const void *this, UNUSED FILE *out) {
    fprintf(stderr, "Ref code gen under construction\n");
    return 0;
}

int CodeGen_Paren(UNUSED const void *this, UNUSED FILE *out) {
    safe_fprintf(out, "(");
    const ASTNode *node = this;
    ASTParenData  *data = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    safe_function_call(vtable->codegen, data->val, out);
    safe_fprintf(out, ")");
    return 0;
}

int CodeGen_Call(UNUSED const void *this, UNUSED FILE *out) {
    fprintf(stderr, "Call code gen under construction\n");
    return 0;
}

int CodeGen_Function(const void *this, FILE *out) {
    const ASTNode   *node = this;
    ASTFunctionData *data = node->data;
    size_t funcID = 0;
    //Function AST should have been added to defined_funcs by define_functions()
    safe_method_call(state.defined_funcs,
                     get,
                     &data->type,
                     sizeof(data->type),
                     &funcID);
    safe_fprintf(out, "func%d", (int)funcID);
    return 0;
}

int CodeGen_Class(UNUSED const void *this, UNUSED FILE *out) {
    fprintf(stderr, "Class code gen under construction\n");
    return 0;
}

int CodeGen_Int(const void *this, UNUSED FILE *out) {
    const ASTNode *node = this;
    ASTIntData    *data = node->data;
    safe_fprintf(out, "%d", data->val);
    return 0;
}

int CodeGen_Double(UNUSED const void *this, UNUSED FILE *out) {
    fprintf(stderr, "Double code gen under construction\n");
    return 0;
}

int CodeGen_Variable(const void *this, FILE *out) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (!state.declared_vars->contains(state.declared_vars, data->name, len)) {
        //TODO: handle variable used without being in symbol table (ICE?)
        print_ICE(
            "variable not found in code gen's declared variable list");
        return 1;
    }
    if (data->type == NULL) {
        print_ICE(
            "type checker failed to infer type of variable before code gen");
        return 1;
    }
    size_t local = 0;
    safe_method_call(state.declared_vars, get, data->name, len, &local);
    safe_fprintf(out, "locals%d.%s", (int)local, data->name);
    return 0;
}

int CodeGen_TypedVar(const void *this, UNUSED FILE *out) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    if (!state.declared_vars->contains(state.declared_vars, data->name, len)) {
        //TODO: handle variable used without being in symbol table (ICE?)
        print_ICE(
            "type checker failed to infer type of variable before code gen");
        return 1;
    }
    return 0;
}
