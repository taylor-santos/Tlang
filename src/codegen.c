#include <string.h>
#include "codegen.h"
#include "safe.h"

typedef struct {
    int indent;
    const Map *func_defs;
    int tmp_count;
} CodegenState;

static void include_headers(FILE *out) {
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "\n");
}

static void print_indent(int n, FILE *out) {
    fprintf(out, "%*s", n * INDENT_WIDTH, "");
}

static void define_closures(FILE *out) {
    fprintf(out, "typedef struct {\n");
    print_indent(1, out);
    fprintf(out, "void *fn;\n");
    print_indent(1, out);
    fprintf(out, "void ***env;\n");
    print_indent(1, out);
    fprintf(out, "size_t env_size;\n");
    fprintf(out, "} closure;\n");
    fprintf(out, "\n");
    fprintf(out, "void print_closure(closure *c) {\n");
    print_indent(1, out);
    fprintf(out, "printf(\"{ \");\n");
    print_indent(1, out);
    fprintf(out, "char *sep = \"\";\n");
    print_indent(1, out);
    fprintf(out, "for (int i = 0; i < c->env_size; i++) {\n");
    print_indent(2, out);
    fprintf(out, "printf(\"%%s%%p\", sep, c->env[i]);\n");
    print_indent(2, out);
    fprintf(out, "sep = \", \";\n");
    print_indent(1, out);
    fprintf(out, "}\n");
    print_indent(1, out);
    fprintf(out, "printf(\" }\\n\");\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out, "#define call(c, ...) ((void*(*)(closure*, ...))"
                 "((closure*)c)->fn)(c, ##__VA_ARGS__)\n");
    fprintf(out, "#define build_closure(c_ptr, func, ...) { \\\n");
    print_indent(1, out);
    fprintf(out, "*c_ptr = malloc(sizeof(closure)); \\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)*c_ptr)->fn = func; \\\n");
    print_indent(1, out);
    fprintf(out, "void **env[] = {__VA_ARGS__}; \\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)*c_ptr)->env = malloc(sizeof(env)); \\\n");
    print_indent(1, out);
    fprintf(out, "memcpy(((closure*)*c_ptr)->env, env, sizeof(env)); "
                 "\\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)*c_ptr)->env_size = sizeof(env) / "
                 "sizeof(void**); \\\n");
    fprintf(out, "}\n\n");
}

static void define_builtins(FILE *out) {
    fprintf(out, "typedef struct {\n");
    print_indent(1, out);
    fprintf(out, "int value;\n");
    fprintf(out, "} class_Int;\n");
    fprintf(out, "\n");
    fprintf(out, "void *new_Int(int val) {\n");
    print_indent(1, out);
    fprintf(out, "class_Int *ret = malloc(sizeof(class_Int));\n");
    print_indent(1, out);
    fprintf(out, "ret->value = val;\n");
    print_indent(1, out);
    fprintf(out, "return ret;\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out, "void builtin_println(closure *c, void *val) {\n");
    print_indent(1, out);
    fprintf(out, "printf(\"%%d\\n\", ((class_Int*)val)->value);\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out,
            "void *builtin_0x%X(closure *c, void *a, void *b) {\n", '+');
    print_indent(1, out);
    fprintf(out,
            "return new_Int(((class_Int*)a)->value + "
            "((class_Int*)b)->value);\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
}

static int define_functions(CodegenState *state, FILE *out) {
    ASTNode **nodes = NULL;
    size_t *ptr_lengths;
    int fn_count;

    safe_method_call(state->func_defs, keys, &fn_count, &ptr_lengths, &nodes);
    fprintf(out, "/* Forward declare functions */\n");
    for (int i = 0; i < fn_count; i++) {
        ASTNode *node = nodes[i];
        ASTFunctionData *data = node->data;
        fprintf(out, "void ");
        if (data->signature->ret_type->type != NONE) {
            fprintf(out, "*");
        }
        int *index = NULL;
        safe_method_call(state->func_defs, get, node, sizeof(node), &index);
        fprintf(out, "func%d(closure*", *index);
        if (data->signature->extension != NULL) {
            char *ref = data->signature->extension->type->type == REFERENCE
                        ? "*"
                        : "";
            fprintf(out, ", void*%s", ref);
        }
        int arg_count;
        NamedArg **args = NULL;
        safe_method_call(data->signature->named_args, array, &arg_count, &args);
        for (int j = 0; j < arg_count; j++) {
            char *ref = args[j]->type->type == REFERENCE
                        ? "*"
                        : "";
            fprintf(out, ", void*%s", ref);
        }
        free(args);
        fprintf(out, ");\n");
    }
    fprintf(out, "\n");
    fprintf(out, "/* Define functions */\n");
    for (int i = 0; i < fn_count; i++) {
        ASTNode *node = nodes[i];
        ASTFunctionData *data = node->data;
        fprintf(out, "void ");
        if (data->signature->ret_type->type != NONE) {
            fprintf(out, "*");
        }
        int *index = NULL;
        safe_method_call(state->func_defs, get, node, sizeof(node), &index);
        fprintf(out, "func%d(closure *c", *index);
        if (data->signature->extension != NULL) {
            char *ref = data->signature->extension->type->type == REFERENCE
                        ? "*"
                        : "";
            fprintf(out,
                    ", void *%s%s",
                    ref,
                    data->signature->extension->name);
        }
        int arg_count;
        NamedArg **args = NULL;
        safe_method_call(data->signature->named_args, array, &arg_count, &args);
        for (int j = 0; j < arg_count; j++) {
            char *ref = args[j]->type->type == REFERENCE
                        ? "*"
                        : "";
            fprintf(out, ", void *%s%s", ref, args[j]->name);
        }
        free(args);
        fprintf(out, ") {\n");
        state->indent++;
        int no_vars = 1;
        {
            const char **symbols = NULL;
            int symbol_count;
            size_t *lengths;
            safe_method_call(data->symbols,
                             keys,
                             &symbol_count,
                             &lengths,
                             &symbols);
            char *sep = "void *";
            for (int j = 0; j < symbol_count; j++) {
                if (data->args->contains(
                        data->args,
                        symbols[j],
                        lengths[j])) {
                    free((char *) symbols[j]);
                    continue;
                }
                if (data->env->contains(
                        data->env,
                        symbols[j],
                        lengths[j])) {
                    free((char *) symbols[j]);
                    continue;
                }
                if (data->self->contains(
                        data->self,
                        symbols[j],
                        lengths[j])) {
                    free((char *) symbols[j]);
                    continue;
                }
                VarType *type = NULL;
                safe_method_call(data->symbols,
                                 get,
                                 symbols[j],
                                 lengths[j],
                                 &type);
                char *ref = type->type == REFERENCE
                            ? "*"
                            : "";
                if (no_vars) print_indent(state->indent, out);
                no_vars = 0;
                fprintf(out,
                        "%s%s%.*s",
                        sep,
                        ref,
                        (int)lengths[j],
                        symbols[j]);
                sep = ", *";
                free((char*)symbols[j]);
            }
            if (!no_vars) fprintf(out, ";\n");
            free(lengths);
            free(symbols);
        }
        const char **env = NULL;
        int env_count;
        size_t *env_lengths;
        safe_method_call(data->env,
                         keys,
                         &env_count,
                         &env_lengths,
                         &env);
        int env_end_index = env_count;
        for (int j = 0; j < env_count; j++) {
            print_indent(1, out);
            fprintf(out,
                    "#define %.*s *c->env[%d]\n",
                    (int) env_lengths[j],
                    env[j],
                    j);
        }
        const char **assigned = NULL;
        int assigned_count;
        size_t *assigned_lengths;
        safe_method_call(data->self,
                         keys,
                         &assigned_count,
                         &assigned_lengths,
                         &assigned);
        for (int j = 0; j < assigned_count; j++) {
            print_indent(1, out);
            fprintf(out,
                    "#define %.*s c\n",
                    (int) assigned_lengths[j],
                    assigned[j]);
        }

        ASTNode **stmts = NULL;
        int stmt_count;
        safe_method_call(data->stmts, array, &stmt_count, &stmts);
        for (int j = 0; j < stmt_count; j++) {
            ASTNode *stmt = stmts[j];
            ASTNodeVTable *vtable = stmt->vtable;
            char *code = vtable->codegen(stmt, state, out);
            print_indent(state->indent, out);
            fprintf(out, "%s;\n", code);
            free(code);
        }
        free(stmts);
        for (int j = 0; j < env_count; j++) {
            print_indent(1, out);
            fprintf(out,
                    "#undef %.*s\n",
                    (int) env_lengths[j],
                    env[j]);
            free((char*)env[j]);
        }
        free(env_lengths);
        free(env);
        for (int j = 0; j < assigned_count; j++) {
            print_indent(1, out);
            fprintf(out,
                    "#undef %.*s\n",
                    (int) assigned_lengths[j],
                    assigned[j]);
            free((char *) assigned[j]);
        }
        free(assigned_lengths);
        free(assigned);
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
        free(nodes[i]);
    }
    free(nodes);
    free(ptr_lengths);
    return 0;
}

int define_main(ASTProgramData *data, CodegenState *state, FILE *out) {
    fprintf(out, "int main(int argc, char *argv[]) {\n");
    state->indent++;
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->symbols, keys, &symbol_count, &lengths, &symbols);
    char *sep = "void *";
    if (symbol_count > 0) print_indent(state->indent, out);
    for (int i = 0; i < symbol_count; i++) {
        VarType *type = NULL;
        safe_method_call(data->symbols, get, symbols[i], lengths[i], &type);
        char *ref = type->type == REFERENCE
                    ? "*"
                    : "";
        fprintf(out, "%s%s%.*s", sep, ref, (int)lengths[i], symbols[i]);
        sep = ", *";
        free((char*)symbols[i]);
    }
    if (symbol_count > 0) fprintf(out, ";\n");
    free(lengths);
    free(symbols);

    const char *builtins[] = {"println", "0x2B"};

    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        print_indent(state->indent, out);
        fprintf(out, "build_closure(&var_%s, builtin_%s);\n", builtins[i], builtins[i]);
    }

    ASTNode **stmts = NULL;
    int stmt_count;
    safe_method_call(data->statements, array, &stmt_count, &stmts);
    for (int i = 0; i < stmt_count; i++) {
        ASTNode *stmt = stmts[i];
        ASTStatementVTable *vtable = stmt->vtable;
        char *code = vtable->codegen(stmt, state, out);
        print_indent(1, out);
        fprintf(out, "%s;\n", code);
        free(code);
    }
    free(stmts);
    state->indent--;
    fprintf(out, "}\n");
    fprintf(out, "\n");
    return 0;
}

char *CodeGen_Program(const void *this, UNUSED void *state, FILE *out) {
    const ASTNode *node = this;
    ASTProgramData *data = node->data;

    CodegenState *codegen_state = malloc(sizeof(*codegen_state));
    if (codegen_state == NULL) {
        print_ICE("unable to allocate memory");
    }
    codegen_state->indent = 0;
    codegen_state->tmp_count = 0;
    codegen_state->func_defs = data->func_defs;

    include_headers(out);
    define_closures(out);
    define_builtins(out);
    define_functions(codegen_state, out);
    define_main(data, codegen_state, out);
    free(codegen_state);
    return 0;
}

char *CodeGen_Assignment(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTNodeVTable *vtable = lhs->vtable;
    ASTStatementData *lhs_data = lhs->data;
    VarType *lhs_type = lhs_data->type;
    char *lhs_code = vtable->codegen(lhs, state, out);
    vtable = rhs->vtable;
    ASTStatementData *rhs_data = rhs->data;
    VarType *rhs_type = rhs_data->type;
    char *rhs_code = vtable->codegen(rhs, state, out);
    char *ret = NULL;
    if (lhs_type->type == REFERENCE && rhs_type->type != REFERENCE) {
        append_string(&ret, "*");
    }
    append_string(&ret, "%s = ", lhs_code);
    if (lhs_type->type != REFERENCE && rhs_type->type == REFERENCE) {
        append_string(&ret, "*");
    }
    append_string(&ret, "%s", rhs_code);
    free(lhs_code);
    free(rhs_code);
    return ret;
}

char *CodeGen_Return(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    const ASTReturnData *data = node->data;
    char *ret = strdup("return");
    if (data->returns != NULL) {
        const ASTNode *ret_node = data->returns;
        ASTStatementVTable *ret_vtable = ret_node->vtable;
        char *code = ret_vtable->codegen(ret_node, state, out);
        ASTStatementData *ret_data = ret_node->data;
        char *ref = ret_data->type->type == REFERENCE
                    ? data->type->sub_type->type == REFERENCE
                    ? ""
                    : "*"
                    : data->type->sub_type->type == REFERENCE
                    ? "&"
                    : "";
        append_string(&ret, " %s%s", ref, code);
        free(code);
    }
    return ret;
}

char *CodeGen_Expression(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this, *ext_node;
    ASTNode **args = NULL;
    ASTExpressionData *data = node->data;
    Expression *expr = data->expr_node;
    const ASTNode *expr_node = expr->node, *arg;
    ASTStatementVTable *expr_vtable = expr_node->vtable,
                       *ext_vtable,
                       *arg_vtable;
    ASTStatementData *expr_data = expr_node->data, *ext_data, *arg_data;
    int arg_count;
    char *func_code, *ret, **arg_codes, *ext_code = NULL;
    FuncType *function;
    NamedArg **arg_types = NULL;
    VarType **passed_types, *ext_type = NULL;

    switch(expr->type) {
        case EXPR_FUNC:
            function = expr_data->type->function;
            func_code = expr_vtable->codegen(expr_node, state, out);

            if (expr->extension != NULL) {
                ext_node = expr->extension;
                ext_vtable = ext_node->vtable;
                ext_data = ext_node->data;
                ext_type = ext_data->type;
                ext_code = ext_vtable->codegen(ext_node, state, out);
            }
            safe_method_call(expr->args, array, &arg_count, &args);
            arg_codes = malloc(sizeof(*arg_codes) * arg_count);
            passed_types = malloc(sizeof(*passed_types) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                arg = args[i];
                arg_vtable = arg->vtable;
                arg_data = arg->data;
                arg_codes[i] = arg_vtable->codegen(arg, state, out);
                passed_types[i] = arg_data->type;
            }
            ret = NULL;
            safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
            print_indent(cg_state->indent, out);
            char *ref = function->ret_type->type == REFERENCE
                        ? "*"
                        : "";
            fprintf(out, "void *%s%s = ", ref, ret);
            fprintf(out, "call(%s", func_code);
            free(func_code);
            if (expr->extension != NULL) {
                fprintf(out, ", ");
                if (ext_type->type == REFERENCE
                    && function->extension->type->type != REFERENCE) {
                    fprintf(out, "*");
                }
                fprintf(out, "%s", ext_code);
                free(ext_code);
            }
            safe_method_call(function->named_args,
                             array,
                             &arg_count,
                             &arg_types);
            for (int i = 0; i < arg_count; i++) {
                VarType *arg_type = arg_types[i]->type;
                VarType *passed_type = passed_types[i];
                fprintf(out, ", ");
                if (passed_type->type == REFERENCE
                    && arg_type->type != REFERENCE) {
                    fprintf(out, "*");
                }
                fprintf(out, "%s", arg_codes[i]);
                free(arg_codes[i]);
            }
            free(args);
            free(arg_codes);
            free(arg_types);
            free(passed_types);
            fprintf(out, ");\n");
            return ret;
        case EXPR_VALUE:
            return expr_vtable->codegen(expr_node, state, out);
    }
    return NULL;
}

char *CodeGen_Ref(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTRefData *data = node->data;
    ASTStatementVTable *ref_vtable = data->expr->vtable;
    char *ret;
    char *code = ref_vtable->codegen(data->expr, state, out);
    asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void **%s = &%s;\n", ret, code);
    return ret;
}

char *CodeGen_Paren(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTParenData *data = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    return vtable->codegen(data->val, state, out);
}

char *CodeGen_Hold(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTHoldData *data = node->data;
    ASTHoldVTable *vtable = data->val->vtable;
    return vtable->codegen(data->val, state, out);
}

char *CodeGen_Function(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    int *index = NULL;
    safe_method_call(cg_state->func_defs, get, node, sizeof(node), &index);
    char *ret = NULL;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s;\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "build_closure(&%s, func%d, ", ret, *index);
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->env,
                     keys,
                     &symbol_count,
                     &lengths,
                     &symbols);
    char *sep = "";
    for (int j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        char *ref = var_type->type == REFERENCE
                    ? ""
                    : "&";
        fprintf(out,
                "%s(void**)%s%.*s",
                sep,
                ref,
                (int)lengths[j],
                symbols[j]);
        sep = ", ";
        free((void*)symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, ");\n");
    return ret;
}

char *CodeGen_Class(const void *this, void *state, FILE *out) {
    return strdup("CLASS");
}

char *CodeGen_Int(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s = new_Int(%d);\n", ret, data->val);
    return ret;
}

char *CodeGen_Double(const void *this, void *state, FILE *out) {
    return strdup("DOUBLE");
}

char *CodeGen_Variable(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    return strdup(data->name);
}

char *CodeGen_TypedVar(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    return strdup(data->name);
}
