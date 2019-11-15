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
    fprintf(out, "}\n");
    fprintf(out, "#define build_tuple(t, ...) { \\\n");
    print_indent(1, out);
    fprintf(out, "void *tuple[] = {__VA_ARGS__}; \\\n");
    print_indent(1, out);
    fprintf(out, "t = malloc(sizeof(tuple)); \\\n");
    print_indent(1, out);
    fprintf(out, "memcpy(t, tuple, sizeof(tuple)); \\\n");
    fprintf(out, "}\n\n");

}

static void define_builtins(FILE *out) {
    fprintf(out, "void builtin_println(closure *c, void *val) {\n");
    print_indent(1, out);
    fprintf(out, "printf(\"%%d\\n\", ((class0*)val)->val);\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
}

static int define_classes(CodegenState *state, FILE *out) {
    size_t count = state->class_defs->size(state->class_defs);
    fprintf(out, "/* Define Classes */\n");
    for (size_t i = 0; i < count; i++) {
        VarType *type = NULL;
        ASTClassData *classData = NULL; //Only defined for non-builtins
        if (i < BUILTIN_COUNT) {
            safe_method_call(state->class_defs, get, i, &type);
        } else {
            const ASTNode *classNode = NULL;
            safe_method_call(state->class_defs, get, i, &classNode);
            classData = classNode->data;
            type = classData->type;
        }
        ClassType *class = type->class;
        fprintf(out, "typedef struct {\n");
        state->indent++;
        if (i < BUILTIN_COUNT) {
            print_indent(state->indent, out);
            fprintf(out, "%s val;\n", BUILTIN_NAMES[i]);
        }
        size_t field_count = class->fields->size(class->fields);
        for (size_t j = 0; j < field_count; j++) {
            NamedType *field = NULL;
            safe_method_call(class->fields, get, j, &field);
            print_indent(state->indent, out);
            fprintf(out, "void *");
            if (field->type->is_ref) {
                fprintf(out, "*");
            }
            fprintf(out, "%s;\n", field->name);
        }
        state->indent--;
        fprintf(out, "} class%ld;\n", class->classID);
        fprintf(out, "\n");
        fprintf(out, "void *new_class%ld(closure *c) {\n", class->classID);
        state->indent++;
        print_indent(state->indent, out);
        fprintf(out,
                "class%ld *var_self = malloc(sizeof(class%ld));\n",
                class->classID,
                class->classID);
        for (size_t j = 0; j < field_count; j++) {
            NamedType *field = NULL;
            safe_method_call(type->class->fields, get, j, &field);
            print_indent(state->indent, out);
            fprintf(out, "#define %s var_self->%s\n", field->name, field->name);
        }
        if (i >= BUILTIN_COUNT) {
            const char **env = NULL;
            size_t env_count;
            size_t *env_lengths;
            safe_method_call(classData->env,
                             keys,
                             &env_count,
                             &env_lengths,
                             &env);
            for (size_t j = 0; j < env_count; j++) {
                print_indent(1, out);
                fprintf(out,
                        "#define %.*s c->env[%ld]\n",
                        (int) env_lengths[j],
                        env[j],
                        j);
            }
            size_t stmt_count = class->stmts->size(class->stmts);
            for (size_t j = 0; j < stmt_count; j++) {
                const ASTNode *stmt = NULL;
                safe_method_call(class->stmts, get, j, &stmt);
                ASTStatementVTable *vtable = stmt->vtable;
                char *code = vtable->codegen(stmt, state, out);
                print_indent(state->indent, out);
                fprintf(out, "%s;\n", code);
            }
            print_indent(state->indent, out);
            fprintf(out, "return var_self;\n");
            for (size_t j = 0; j < env_count; j++) {
                print_indent(1, out);
                fprintf(out,
                        "#undef %.*s\n",
                        (int) env_lengths[j],
                        env[j]);
            }
        } else {

        }
        for (size_t j = 0; j < field_count; j++) {
            NamedType *field = NULL;
            safe_method_call(class->fields, get, j, &field);
            print_indent(state->indent, out);
            fprintf(out, "#undef %s\n", field->name);
        }
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }
    return 0;
}

static int forward_declare_functions(CodegenState *state, FILE *out) {
    ASTNode **nodes = NULL;
    size_t *ptr_lengths;
    size_t fn_count;
    safe_method_call(state->func_defs, keys, &fn_count, &ptr_lengths, &nodes);
    fprintf(out, "/* Forward declare functions */\n");
    for (size_t i = 0; i < fn_count; i++) {
        ASTNode *node = nodes[i];
        ASTFunctionData *data = node->data;
        fprintf(out, "void ");
        if (data->type->function->ret_type != NULL) {
            fprintf(out, "*");
        }
        int *index = NULL;
        safe_method_call(state->func_defs, get, node, sizeof(node), &index);
        fprintf(out, "func%d(closure*", *index);
        size_t arg_count = data->type->function->named_args
                ->size(data->type->function->named_args);
        if (arg_count > 0) {
            fprintf(out, ", void*");
        }
        fprintf(out, ");\n");
        free(nodes[i]);
    }
    free(nodes);
    free(ptr_lengths);
    fprintf(out, "\n");
    return 0;
}

static int define_functions(CodegenState *state, FILE *out) {
    ASTNode **nodes = NULL;
    size_t *ptr_lengths;
    size_t fn_count;
    safe_method_call(state->func_defs, keys, &fn_count, &ptr_lengths, &nodes);
    if (fn_count > 0) {
        fprintf(out, "/* Define functions */\n");
        for (size_t i = 0; i < fn_count; i++) {
            ASTNode *node = nodes[i];
            ASTFunctionData *data = node->data;
            fprintf(out, "void ");
            if (data->type->function->ret_type != NULL) {
                fprintf(out, "*");
            }
            int *index = NULL;
            safe_method_call(state->func_defs, get, node, sizeof(node), &index);
            fprintf(out, "func%d(closure *c", *index);
            size_t arg_count = data->type->function->named_args->size
                    (data->type->function->named_args);
            if (arg_count > 0) {
                fprintf(out, ", void *arg");
            }
            fprintf(out, ") {\n");
            state->indent++;

            if (arg_count == 1) {
                NamedType *arg = NULL;
                safe_method_call(data->type->function->named_args,
                                 get,
                                 0,
                                 &arg);
                print_indent(state->indent, out);
                fprintf(out, "#define %s arg\n", arg->name);
            } else {
                for (size_t j = 0; j < arg_count; j++) {
                    NamedType *arg = NULL;
                    safe_method_call(data->type->function->named_args,
                                     get,
                                     j,
                                     &arg);
                    print_indent(state->indent, out);
                    fprintf(out,
                            "#define %s ((void**)arg)[%ld]\n",
                            arg->name,
                            j);
                }
            }

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
                    if (data->args
                            ->contains(data->args, symbols[j], lengths[j])) {
                        free((char *) symbols[j]);
                        continue;
                    }
                    if (data->env
                            ->contains(data->env, symbols[j], lengths[j])) {
                        free((char *) symbols[j]);
                        continue;
                    }
                    VarType *type = NULL;
                    safe_method_call(data->symbols,
                                     get,
                                     symbols[j],
                                     lengths[j],
                                     &type);
                    if (no_vars) {
                        print_indent(state->indent, out);
                    }
                    no_vars = 0;
                    fprintf(out, "%s%.*s", sep, (int) lengths[j], symbols[j]);
                    sep = ", *";
                    free((char *) symbols[j]);
                }
                if (!no_vars) {
                    fprintf(out, ";\n");
                }
                free(lengths);
                free(symbols);
            }
            const char **env = NULL;
            size_t env_count;
            size_t *env_lengths;
            safe_method_call(data->env, keys, &env_count, &env_lengths, &env);
            for (size_t j = 0; j < env_count; j++) {
                print_indent(1, out);
                fprintf(out,
                        "#define %.*s c->env[%ld]\n",
                        (int) env_lengths[j],
                        env[j],
                        j);
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
                print_indent(1, out);
                fprintf(out, "#undef %.*s\n", (int) env_lengths[j], env[j]);
                free((char *) env[j]);
            }
            free(env_lengths);
            free(env);
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
    size_t *lengths, symbol_count;
    safe_method_call(data->symbols, keys, &symbol_count, &lengths, &symbols);
    char *sep = "void *";
    if (symbol_count > 0) {
        print_indent(state->indent, out);
    }
    for (size_t i = 0; i < symbol_count; i++) {
        VarType *type = NULL;
        safe_method_call(data->symbols, get, symbols[i], lengths[i], &type);
        fprintf(out, "%s%.*s", sep, (int) lengths[i], symbols[i]);
        sep = ", *";
        free((char *) symbols[i]);
    }
    if (symbol_count > 0) {
        fprintf(out, ";\n");
    }
    free(lengths);
    free(symbols);

    const char *builtins[] = { "println" };

    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        print_indent(state->indent, out);
        fprintf(out,
                "build_closure(var_%s, builtin_%s)\n",
                builtins[i],
                builtins[i]);
    }
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        print_indent(state->indent, out);
        fprintf(out,
                "build_closure(var_%s, new_class%ld)\n",
                BUILTIN_NAMES[i],
                i);
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
    forward_declare_functions(codegen_state, out);
    define_classes(codegen_state, out);
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
    char *lhs_ref = lhs_type->is_ref
                    ? "*"
                    : "";
    char *rhs_ref = rhs_type->is_ref
                    ? "*"
                    : "";
    char *ret = NULL;
    safe_asprintf(&ret, "%s%s = %s%s", lhs_ref, lhs_code, rhs_ref, rhs_code);
    free(lhs_code);
    free(rhs_code);
    return ret;
}

char *CodeGen_Def(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTDefData *data = node->data;
    size_t lhs_size = data->lhs->size(data->lhs);
    const ASTNode *rhs = data->rhs;
    ASTStatementVTable *rhs_vtable = rhs->vtable;
    ASTStatementData *rhs_data = rhs->data;
    char *rhs_code = rhs_vtable->codegen(rhs, state, out);
    if (lhs_size != 1 && rhs_data->type->type == TUPLE) {
        for (size_t i = 0; i < lhs_size; i++) {
            ASTNode *lhs = NULL;
            safe_method_call(data->lhs, get, i, &lhs);
            ASTLExprVTable *lhs_vtable = lhs->vtable;
            char *lhs_code = lhs_vtable->codegen(lhs, state, out);
            print_indent(cg_state->indent, out);
            fprintf(out, "%s = %s[%ld];\n", lhs_code, rhs_code, i);
        }
    } else {
        ASTNode *lhs = NULL;
        safe_method_call(data->lhs, get, 0, &lhs);
        ASTLExprVTable *lhs_vtable = lhs->vtable;
        char *lhs_code = lhs_vtable->codegen(lhs, state, out);
        print_indent(cg_state->indent, out);
        fprintf(out, "%s = %s;\n", lhs_code, rhs_code);
    }
    return rhs_code;
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

char *gen_expression(Expression *expr, CodegenState *state, FILE *out) {
    char *tmp = NULL, *sub_expr = NULL;
    const ASTNode *node = NULL;
    ASTStatementVTable *vtable = NULL;
    ASTStatementData *data = NULL;
    switch (expr->expr_type) {
        case EXPR_VAR:
            node = expr->node;
            vtable = node->vtable;
            return vtable->codegen(node, state, out);
        case EXPR_FIELD:
            asprintf(&tmp, "tmp%d", state->tmp_count++);
            sub_expr = gen_expression(expr->sub_expr, state, out);
            print_indent(state->indent, out);
            node = expr->sub_expr->node;
            data = node->data;
            int classID = data->type->object->classID;
            fprintf(out,
                    "void *%s = ((class%d*)%s)->%s;\n",
                    tmp,
                    classID,
                    sub_expr,
                    expr->name);
            return tmp;
        case EXPR_CONS:;
            char *cons = gen_expression(expr->sub_expr, state, out);
            asprintf(&tmp, "tmp%d", state->tmp_count++);
            print_indent(state->indent, out);
            fprintf(out, "void *%s = call(%s);\n", tmp, cons);
            return tmp;
        case EXPR_FUNC:;
            char *arg = NULL;
            if (expr->arg != NULL) {
                Expression *arg_expr = expr->arg;
                arg = gen_expression(arg_expr, state, out);
            }
            char *func = gen_expression(expr->sub_expr, state, out);
            asprintf(&tmp, "tmp%d", state->tmp_count++);
            print_indent(state->indent, out);
            fprintf(out, "void *%s = call(%s", tmp, func);
            if (expr->arg != NULL) {
                fprintf(out, ", %s", arg);
            }
            fprintf(out, ");\n");
            return tmp;
    }
    fprintf(stderr, "error: unmatched expression\n");
    return NULL;
}

char *CodeGen_Expression(
        const void *this, void *state, FILE *out
) {
    const ASTNode *node = this;
    ASTExpressionData *data = node->data;
    return gen_expression(data->expr_node, state, out);
}

char *CodeGen_Tuple(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTTupleData *data = node->data;
    size_t size = data->exprs->size(data->exprs);
    char **exprs = malloc(sizeof(*exprs) * size);
    for (size_t i = 0; i < size; i++) {
        const ASTNode *expr_node = NULL;
        safe_method_call(data->exprs, get, i, &expr_node);
        ASTStatementVTable *vtable = expr_node->vtable;
        exprs[i] = vtable->codegen(expr_node, state, out);
    }
    char *ret = NULL;
    CodegenState *cg_state = state;
    asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void **%s;\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "build_tuple(%s", ret);
    for (size_t i = 0; i < size; i++) {
        fprintf(out, ", %s", exprs[i]);
    }
    fprintf(out, ")");
    return ret;
}

char *CodeGen_Ref(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTRefData *data = node->data;
    ASTStatementVTable *ref_vtable = data->expr->vtable;
    char *code = ref_vtable->codegen(data->expr, state, out);
    ASTStatementData *ref_data = data->expr->data;
    if (ref_data->type->is_ref) {
        return code;
    }
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

char *CodeGen_Hold(
        UNUSED const void *this, UNUSED void *state, UNUSED FILE *out
) {
    return strdup("HOLD");
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
    fprintf(out, "build_closure(%s, func%d", ret, *index);
    const char **symbols = NULL;
    size_t *lengths, symbol_count;
    safe_method_call(data->env, keys, &symbol_count, &lengths, &symbols);
    for (size_t j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        fprintf(out, ", %.*s", (int) lengths[j], symbols[j]);
        free((void *) symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, ")\n");
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
    fprintf(out, "build_closure(%s, new_class%ld", ret, class->classID);
    const char **symbols = NULL;
    size_t *lengths, symbol_count;
    safe_method_call(data->env, keys, &symbol_count, &lengths, &symbols);
    for (size_t j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        fprintf(out, ", %.*s", (int) lengths[j],
                symbols[j]);
        free((void *) symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, ")\n");
    return ret;
}

char *CodeGen_Int(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s = call(var_int);\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "((class%d*)%s)->val = %d;\n", INT, ret, data->val);
    return ret;
}

char *CodeGen_Double(
        UNUSED const void *this, UNUSED void *state, UNUSED FILE *out
) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s = new_class%d(%f);\n", ret, DOUBLE, data->val);
    return ret;
}

char *CodeGen_Variable(
        const void *this, UNUSED void *state, UNUSED FILE *out
) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    return strdup(data->name);
}

char *CodeGen_TypedVar(
        const void *this, UNUSED void *state, UNUSED FILE *out
) {
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    return strdup(data->name);
}
