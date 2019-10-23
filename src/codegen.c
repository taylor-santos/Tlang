#include <string.h>
#include "codegen.h"
#include "safe.h"
#include "builtins.h"

typedef struct {
    int indent;
    const Map *func_defs;
    const Vector *class_defs;
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
    fprintf(out, "void **env;\n");
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
    fprintf(out, "for (size_t i = 0; i < c->env_size; i++) {\n");
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
    fprintf(out, "#define build_closure(c, func, ...) { \\\n");
    print_indent(1, out);
    fprintf(out, "c = malloc(sizeof(closure)); \\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)c)->fn = func; \\\n");
    print_indent(1, out);
    fprintf(out, "void *env[] = {__VA_ARGS__}; \\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)c)->env = malloc(sizeof(env)); \\\n");
    print_indent(1, out);
    fprintf(out, "memcpy(((closure*)c)->env, env, sizeof(env)); "
                 "\\\n");
    print_indent(1, out);
    fprintf(out, "((closure*)c)->env_size = sizeof(env) / "
                 "sizeof(void*); \\\n");
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
    fprintf(out, "typedef struct {\n");
    print_indent(1, out);
    fprintf(out, "double value;\n");
    fprintf(out, "} class_Double;\n");
    fprintf(out, "\n");
    fprintf(out, "void *new_Double(double val) {\n");
    print_indent(1, out);
    fprintf(out, "class_Double *ret = malloc(sizeof(class_Double));\n");
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

static int define_classes(CodegenState *state, FILE *out) {
    VarType **nodes = NULL;
    size_t count;
    safe_method_call(state->class_defs, array, &count, &nodes);
    fprintf(out, "/* Builtin Classes */\n");
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        VarType *type = nodes[i];
        fprintf(out, "// %s\n", BUILTIN_NAMES[i]);
        fprintf(out, "typedef struct {\n");
        state->indent++;
        print_indent(state->indent, out);
        fprintf(out, "%s val;\n", BUILTIN_NAMES[i]);
        NamedType **fields = NULL;
        size_t field_count;
        safe_method_call(type->class->fields,
                         array,
                         &field_count,
                         &fields);
        for (size_t j = 0; j < field_count; j++) {
            print_indent(state->indent, out);
            fprintf(out,"void *");
            if (fields[j]->type->is_ref) {
                fprintf(out, "*");
            }
            fprintf(out, "%s;\n", fields[j]->name);
        }
        free(fields);
        state->indent--;
        fprintf(out, "} class%ld;\n", type->class->classID);
        fprintf(out, "\n");
        fprintf(out,
                "void *new_class%ld(%s val) {\n",
                type->class->classID,
                BUILTIN_NAMES[i]);
        state->indent++;
        print_indent(state->indent, out);
        fprintf(out,
                "class%ld *ret = malloc(sizeof(class%ld));\n",
                type->class->classID,
                type->class->classID);
        print_indent(state->indent, out);
        fprintf(out, "ret->val = val;\n");
        print_indent(state->indent, out);
        fprintf(out, "return ret;\n");
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }
    fprintf(out, "\n");

    if (BUILTIN_COUNT < count) {
        fprintf(out, "/* Class Definitions */\n");
    }
    for (size_t i = BUILTIN_COUNT; i < count; i++) {
        VarType *type = nodes[i];
        fprintf(out, "typedef struct {\n");
        state->indent++;
        NamedType **fields = NULL;
        size_t field_count;
        safe_method_call(type->class->fields,
                         array,
                         &field_count,
                         &fields);
        for (size_t j = 0; j < field_count; j++) {
            print_indent(state->indent, out);
            fprintf(out,"void *");
            if (fields[j]->type->is_ref) {
                fprintf(out, "*");
            }
            fprintf(out, "%s;\n", fields[j]->name);
        }
        free(fields);
        state->indent--;
        fprintf(out, "} class%ld;\n", type->class->classID);
        fprintf(out, "\n");
        fprintf(out, "void *new_class%ld() {\n", type->class->classID);
        state->indent++;
        print_indent(state->indent, out);
        fprintf(out,
                "class%ld *ret = malloc(sizeof(class%ld));\n",
                type->class->classID,
                type->class->classID);
        print_indent(state->indent, out);
        fprintf(out, "return ret;\n");
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }
    free(nodes);
    return 0;
}

static int define_functions(CodegenState *state, FILE *out) {
    ASTNode **nodes = NULL;
    size_t *ptr_lengths;
    size_t fn_count;
    safe_method_call(state->func_defs, keys, &fn_count, &ptr_lengths, &nodes);
    if (fn_count > 0) {
        fprintf(out, "/* Forward declare functions */\n");
        for (size_t i = 0; i < fn_count; i++) {
            ASTNode *node = nodes[i];
            ASTFunctionData *data = node->data;
            fprintf(out, "void ");
            if (data->signature->ret_type != NULL) {
                fprintf(out, "*");
            }
            int *index = NULL;
            safe_method_call(state->func_defs, get, node, sizeof(node), &index);
            fprintf(out, "func%d(closure*", *index);
            if (data->signature->extension != NULL) {
                fprintf(out, ", void*");
                if (data->signature->extension->type->is_ref) {
                    fprintf(out, "*");
                }
            }
            size_t arg_count;
            NamedType **args = NULL;
            safe_method_call(data->signature->named_args, array, &arg_count, &args);
            for (size_t j = 0; j < arg_count; j++) {
                fprintf(out, ", void*");
                if (args[j]->type->is_ref) fprintf(out, "*");
            }
            free(args);
            fprintf(out, ");\n");
        }
        fprintf(out, "\n");
        fprintf(out, "/* Define functions */\n");
        for (size_t i = 0; i < fn_count; i++) {
            ASTNode *node = nodes[i];
            ASTFunctionData *data = node->data;
            char **self = NULL;
            size_t *self_lengths;
            size_t self_count;
            safe_method_call(data->self, keys, &self_count, &self_lengths, &self);
            if (self_count > 0) {
                char *sep = "/* ";
                for (size_t j = 0; j < self_count; j++) {
                    fprintf(out, "%s%.*s", sep, (int)self_lengths[j], self[j]);
                    sep = ", ";
                    free(self[j]);
                }
                free(self);
                free(self_lengths);
                fprintf(out, " */\n");
            } else {
                fprintf(out, "/* Anonymous Function */\n");
            }
            fprintf(out, "void ");
            if (data->signature->ret_type != NULL) {
                fprintf(out, "*");
            }
            int *index = NULL;
            safe_method_call(state->func_defs, get, node, sizeof(node), &index);
            fprintf(out, "func%d(closure *c", *index);
            if (data->signature->extension != NULL) {
                fprintf(out, ", void *");
                if (data->signature->extension->type->is_ref) {
                    fprintf(out, "*");
                }
                fprintf(out, "%s", data->signature->extension->name);
            }
            size_t arg_count;
            NamedType **args = NULL;
            safe_method_call(data->signature->named_args, array, &arg_count, &args);
            for (size_t j = 0; j < arg_count; j++) {
                fprintf(out, ", void *");
                if (args[j]->type->is_ref) fprintf(out, "*");
                fprintf(out, "%s", args[j]->name);
            }
            free(args);
            fprintf(out, ") {\n");
            state->indent++;
            int no_vars = 1;
            {
                const char **symbols = NULL;
                size_t symbol_count;
                size_t *lengths;
                safe_method_call(data->symbols,
                                 keys,
                                 &symbol_count,
                                 &lengths,
                                 &symbols);
                char *sep = "void *";
                for (size_t j = 0; j < symbol_count; j++) {
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
                    if (no_vars) print_indent(state->indent, out);
                    no_vars = 0;
                    fprintf(out,
                            "%s%.*s",
                            sep,
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
            size_t env_count;
            size_t *env_lengths;
            safe_method_call(data->env,
                             keys,
                             &env_count,
                             &env_lengths,
                             &env);
            for (size_t j = 0; j < env_count; j++) {
                if (data->self->contains(data->self, env[j], env_lengths[j]))
                    continue;
                print_indent(1, out);
                fprintf(out,
                        "#define %.*s c->env[%ld]\n",
                        (int) env_lengths[j],
                        env[j],
                        j);
            }
            const char **assigned = NULL;
            size_t assigned_count;
            size_t *assigned_lengths;
            safe_method_call(data->self,
                             keys,
                             &assigned_count,
                             &assigned_lengths,
                             &assigned);
            for (size_t j = 0; j < assigned_count; j++) {
                print_indent(1, out);
                fprintf(out,
                        "#define %.*s c\n",
                        (int) assigned_lengths[j],
                        assigned[j]);
            }

            ASTNode **stmts = NULL;
            size_t stmt_count;
            safe_method_call(data->stmts, array, &stmt_count, &stmts);
            for (size_t j = 0; j < stmt_count; j++) {
                ASTNode *stmt = stmts[j];
                ASTNodeVTable *vtable = stmt->vtable;
                char *code = vtable->codegen(stmt, state, out);
                print_indent(state->indent, out);
                fprintf(out, "%s;\n", code);
                free(code);
            }
            free(stmts);
            for (size_t j = 0; j < env_count; j++) {
                if (data->self->contains(data->self, env[j], env_lengths[j]))
                    continue;
                print_indent(1, out);
                fprintf(out,
                        "#undef %.*s\n",
                        (int) env_lengths[j],
                        env[j]);
                free((char*)env[j]);
            }
            free(env_lengths);
            free(env);
            for (size_t j = 0; j < assigned_count; j++) {
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
    }
    free(nodes);
    free(ptr_lengths);
    return 0;
}

int define_main(ASTProgramData *data, CodegenState *state, FILE *out) {
    fprintf(out, "int main(int argc, char *argv[]) {\n");
    state->indent++;
    const char **symbols = NULL;
    size_t *lengths, symbol_count;;
    safe_method_call(data->symbols, keys, &symbol_count, &lengths, &symbols);
    char *sep = "void *";
    if (symbol_count > 0) print_indent(state->indent, out);
    for (size_t i = 0; i < symbol_count; i++) {
        VarType *type = NULL;
        safe_method_call(data->symbols, get, symbols[i], lengths[i], &type);
        fprintf(out, "%s%.*s", sep, (int)lengths[i], symbols[i]);
        sep = ", *";
        free((char*)symbols[i]);
    }
    if (symbol_count > 0) fprintf(out, ";\n");
    free(lengths);
    free(symbols);

    const char *builtins[] = { "println" };

    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        print_indent(state->indent, out);
        fprintf(out, "build_closure(var_%s, builtin_%s);\n", builtins[i], builtins[i]);
    }

    ASTNode **stmts = NULL;
    size_t stmt_count;
    safe_method_call(data->statements, array, &stmt_count, &stmts);
    for (size_t i = 0; i < stmt_count; i++) {
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
    codegen_state->class_defs = data->class_defs;

    include_headers(out);
    define_closures(out);
    define_builtins(out);
    define_classes(codegen_state, out);
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
    char *lhs_ref = lhs_type->is_ref ? "*" : "";
    char *rhs_ref = rhs_type->is_ref ? "*" : "";
    char *ret = NULL;
    safe_asprintf(&ret, "%s%s = %s%s", lhs_ref, lhs_code, rhs_ref, rhs_code);
    free(lhs_code);
    free(rhs_code);
    return ret;
}

char *CodeGen_Def(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTDefData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTNodeVTable *vtable = lhs->vtable;
    char *lhs_code = vtable->codegen(lhs, state, out);
    vtable = rhs->vtable;
    char *rhs_code = vtable->codegen(rhs, state, out);
    char *ret = NULL;
    append_string(&ret, "%s = ", lhs_code);
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
        append_string(&ret, " %s", code);
        free(code);
    }
    return ret;
}

char *CodeGen_Expression(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node, *ext_node, **args;
    ASTExpressionData *data;
    Expression *expr;
    const ASTNode *expr_node, *arg;
    ASTStatementVTable *expr_vtable, *ext_vtable, *arg_vtable;
    ASTStatementData *expr_data, *ext_data, *arg_data;
    size_t arg_count;
    char *func_code, *ret, **arg_codes, *ext_code, *code;
    FuncType *function;
    NamedType **arg_types;
    VarType **passed_types, *ext_type;

    args = NULL;
    ext_code = NULL;
    arg_types = NULL;
    ext_type = NULL;
    node = this;
    data = node->data;
    expr = data->expr_node;
    expr_node = expr->node;
    expr_vtable = expr_node->vtable;
    expr_data = expr_node->data;

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
            for (size_t i = 0; i < arg_count; i++) {
                arg = args[i];
                arg_vtable = arg->vtable;
                arg_data = arg->data;
                arg_codes[i] = arg_vtable->codegen(arg, state, out);
                passed_types[i] = arg_data->type;
            }
            ret = NULL;
            if (function->ret_type != NULL) {
                safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
                print_indent(cg_state->indent, out);
                fprintf(out, "void ");
                if (function->ret_type->is_ref) fprintf(out, "*");
                fprintf(out, "*%s = ", ret);
            }
            append_string(&ret, "call(%s", func_code);
            free(func_code);
            if (expr->extension != NULL) {
                append_string(&ret, ", ");
                if ((ext_type->is_ref || ext_type->type == REFERENCE)
                    && !function->extension->type->is_ref) {
                    append_string(&ret, "*");
                }
                if (!ext_type->is_ref
                    && ext_type->type != REFERENCE
                    && function->extension->type->is_ref) {
                    append_string(&ret, "&");
                }

                append_string(&ret, "%s", ext_code);
                free(ext_code);
            }
            safe_method_call(function->named_args,
                             array,
                             &arg_count,
                             &arg_types);
            for (size_t i = 0; i < arg_count; i++) {
                VarType *arg_type = arg_types[i]->type;
                VarType *passed_type = passed_types[i];
                append_string(&ret, ", ");
                if ((passed_type->is_ref || passed_type->type == REFERENCE)
                    && !arg_type->is_ref) {
                    append_string(&ret, "*");
                }
                if (!passed_type->is_ref
                    && passed_type->type != REFERENCE
                    && arg_type->is_ref) {
                    append_string(&ret, "&");
                }
                append_string(&ret, "%s", arg_codes[i]);
                free(arg_codes[i]);
            }
            free(args);
            free(arg_codes);
            free(arg_types);
            free(passed_types);
            append_string(&ret, ")");
            return ret;
        case EXPR_CLASS:
            code = expr_vtable->codegen(expr_node, state, out);;
            safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
            print_indent(cg_state->indent, out);
            fprintf(out, "void *%s = call(%s);\n", ret, code);
            free(code);
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
    char *code = ref_vtable->codegen(data->expr, state, out);
    ASTStatementData *ref_data = data->expr->data;
    if (ref_data->type->is_ref) return code;
    char *ret;
    asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void **%s = &%s;\n", ret, code);
    free(code);
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
    fprintf(out, "build_closure(%s, func%d, ", ret, *index);
    const char **symbols = NULL;
    size_t *lengths, symbol_count;
    safe_method_call(data->env,
                     keys,
                     &symbol_count,
                     &lengths,
                     &symbols);
    char *sep = "";
    for (size_t j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        fprintf(out,
                "%s(void*)%.*s",
                sep,
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
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTClassData *data = node->data;
    VarType *type = data->type;
    ClassType *class = type->class;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s;\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "build_closure(%s, new_class%ld);\n", ret, class->classID);
    return ret;
}

char *CodeGen_Int(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s = new_class%d(%d);\n", ret, INT, data->val);
    return ret;
}

char *CodeGen_Double(UNUSED const void *this,
                     UNUSED void *state,
                     UNUSED FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s = new_class%d(%f);\n", ret, DOUBLE, data->val);
    return ret;
}

char *CodeGen_Variable(const void *this,
                       UNUSED void *state,
                       UNUSED FILE *out) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    return strdup(data->name);
}

char *CodeGen_TypedVar(const void *this,
                       UNUSED void *state,
                       UNUSED FILE *out) {
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    return strdup(data->name);
}
