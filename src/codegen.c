#include <stdio.h>
#include <string.h>
#include "safe.h"
#include "builtins.h"
#include "map.h"
#include "vector.h"
#include "codegen.h"
#include "typechecker.h"
#include "ast/statement.h"
#include "ast/function.h"
#include "ast/program.h"

#define INDENT_WIDTH 4

static void include_headers(FILE *out) {
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "\n");
}

void print_indent(int n, FILE *out) {
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
    fprintf(out, "#define call(c, ...) ((void*(*)(closure*, ...))"
                 "((closure*)c)->fn)(c, ##__VA_ARGS__)\n");
    fprintf(out, "#define build_closure(c, func, ...) \\\n");
    print_indent(1, out);
    fprintf(out, "void *c = NULL; \\\n");
    print_indent(1, out);
    fprintf(out, "{ \\\n");
    print_indent(2, out);
    fprintf(out, "c = malloc(sizeof(closure)); \\\n");
    print_indent(2, out);
    fprintf(out, "((closure*)c)->fn = func; \\\n");
    print_indent(2, out);
    fprintf(out, "void *env[] = {__VA_ARGS__}; \\\n");
    print_indent(2, out);
    fprintf(out, "((closure*)c)->env = malloc(sizeof(env)); \\\n");
    print_indent(2, out);
    fprintf(out, "memcpy(((closure*)c)->env, env, sizeof(env)); "
                 "\\\n");
    print_indent(2, out);
    fprintf(out, "((closure*)c)->env_size = sizeof(env) / "
                 "sizeof(void*); \\\n");
    print_indent(1, out);
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
    //TODO: Don't hard-code stringable trait index
    int STRINGABLE = 3;
    fprintf(out, "class%d *stringable = val;\n", STRINGABLE);
    print_indent(1, out);
    fprintf(out, "class%d *str = call(stringable->field_toString);\n", STRING);
    print_indent(1, out);
    fprintf(out, "printf(\"%%s\\n\", str->val);\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        if (i != STRING) {
            fprintf(out, "void *class%ld_0x%X(closure *c, class%ld *other) "
                         "{\n", i, '=', i);
            print_indent(1, out);
            fprintf(out,
                    "#define var_self ((class%ld*)c->env[0])\n",
                    i);
            print_indent(1, out);
            fprintf(out, "var_self->val = other->val;\n");
            print_indent(1, out);
            fprintf(out, "return var_self;\n");
            print_indent(1, out);
            fprintf(out, "#undef var_self\n");
            fprintf(out, "}\n");
            if (i != BOOL) {
                for (size_t j = 0; j < BINARY_COUNT; j++) {
                    fprintf(out, "void *class%ld_0x", i);
                    const char *c = BINARIES[j];
                    do {
                        fprintf(out, "%X", *c);
                    } while (*(++c));
                    fprintf(out, "(closure *c, class%ld *other) {\n", i);
                    print_indent(1, out);
                    fprintf(out,
                            "#define var_self ((class%ld*)c->env[0])\n",
                            i);
                    print_indent(1, out);
                    fprintf(out,
                            "class%ld *ret = new_class%ld(NULL);\n",
                            i,
                            i);
                    print_indent(1, out);
                    fprintf(out,
                            "ret->val = var_self->val %s other->val;\n",
                            BINARIES[j]);
                    print_indent(1, out);
                    fprintf(out, "return ret;\n");
                    print_indent(1, out);
                    fprintf(out, "#undef var_self\n");
                    fprintf(out, "}\n");
                }
            } else { // i == BOOL
                for (size_t j = 0; j < BOOL_BINARY_COUNT; j++) {
                    fprintf(out, "void *class%ld_0x", i);
                    const char *c = BOOL_BINARIES[j];
                    do {
                        fprintf(out, "%X", *c);
                    } while (*(++c));
                    fprintf(out, "(closure *c, class%ld *other) {\n", i);
                    print_indent(1, out);
                    fprintf(out,
                            "#define var_self ((class%ld*)c->env[0])\n",
                            i);
                    print_indent(1, out);
                    fprintf(out, "class%ld *ret = new_class%ld(NULL);\n", i, i);
                    print_indent(1, out);
                    fprintf(out,
                            "ret->val = var_self->val %s other->val;\n",
                            BOOL_BINARIES[j]);
                    print_indent(1, out);
                    fprintf(out, "return ret;\n");
                    print_indent(1, out);
                    fprintf(out, "#undef var_self\n");
                    fprintf(out, "}\n");
                }
            }
        } else { // i == STRING
            {
                fprintf(out, "void *class%ld_0x", i);
                fprintf(out, "%X", '=');
                fprintf(out, "(closure *c, class%ld *other) {\n", i);
                print_indent(1, out);
                fprintf(out,
                        "#define var_self ((class%ld*)c->env[0])\n",
                        i);
                print_indent(1, out);
                fprintf(out, "var_self->val = strdup(other->val);\n");
                print_indent(1, out);
                fprintf(out, "return var_self;\n");
                print_indent(1, out);
                fprintf(out, "#undef var_self\n");
                fprintf(out, "}\n");
            }
            {
                fprintf(out, "void *class%ld_0x", i);
                fprintf(out, "%X", '+');
                fprintf(out, "(closure *c, class%ld *other) {\n", i);
                print_indent(1, out);
                fprintf(out,
                        "#define var_self ((class%ld*)c->env[0])\n",
                        i);
                print_indent(1, out);
                fprintf(out,
                        "class%ld *ret = new_class%ld(NULL);\n",
                        i,
                        i);
                print_indent(1, out);
                fprintf(out,
                        "int size = snprintf(NULL, 0, \"%%s%%s\", "
                        "var_self->val, other->val);\n");
                print_indent(1, out);
                fprintf(out, "ret->val = malloc(size + 1);\n");
                print_indent(1, out);
                fprintf(out, "sprintf(ret->val, \"%%s%%s\", "
                             " var_self->val, other->val);\n");
                print_indent(1, out);
                fprintf(out, "return ret;\n");
                print_indent(1, out);
                fprintf(out, "#undef var_self\n");
                fprintf(out, "}\n");
            }
        }
        fprintf(out, "void *class%ld_toString(closure *c) {\n", i);
        print_indent(1, out);
        fprintf(out,
                "#define var_self ((class%ld*)c->env[0])\n",
                i);
        switch((BUILTINS)i) {
            case STRING:
                print_indent(1, out);
                fprintf(out, "return var_self;\n");
                break;
            case INT:
                print_indent(1, out);
                fprintf(out,
                        "class%d *str = new_class%d(NULL);\n",
                        STRING,
                        STRING);
                print_indent(1, out);
                fprintf(out, "int size = snprintf(NULL, 0, \"%%d\", "
                             "var_self->val);\n");
                print_indent(1, out);
                fprintf(out, "str->val = malloc(size + 1);\n");
                print_indent(1, out);
                fprintf(out, "sprintf(str->val, \"%%d\", var_self->val);"
                             "\n");
                print_indent(1, out);
                fprintf(out, "return str;\n");
                break;
            case DOUBLE:
                print_indent(1, out);
                fprintf(out,
                        "class%d *str = new_class%d(NULL);\n",
                        STRING,
                        STRING);
                print_indent(1, out);
                fprintf(out, "int size = snprintf(NULL, 0, \"%%f\", "
                             "var_self->val);\n");
                print_indent(1, out);
                fprintf(out, "str->val = malloc(size + 1);\n");
                print_indent(1, out);
                fprintf(out, "sprintf(str->val, \"%%f\", var_self->val);"
                             "\n");
                print_indent(1, out);
                fprintf(out, "return str;\n");
                break;
            case BOOL:
                print_indent(1, out);
                fprintf(out,
                        "class%d *str = new_class%d(NULL);\n",
                        STRING,
                        STRING);
                print_indent(1, out);
                fprintf(out, "str->val = var_self->val ? strdup(\"true\") : "
                             "strdup(\"false\");\n");
                print_indent(1, out);
                fprintf(out, "return str;\n");
                break;
        }
        print_indent(1, out);
        fprintf(out, "#undef var_self\n");
        fprintf(out, "}\n");
    }
}

static int define_classes(CodegenState *state, FILE *out) {
    size_t num_classes = state->class_types->size(state->class_types);
    fprintf(out, "/* Forward Declare Classes */\n");
    for (size_t i = 0; i < num_classes; i++) {
        fprintf(out, "typedef struct class%ld class%ld;\n", i, i);
        fprintf(out, "void *new_class%ld(closure *c);\n", i);
        fprintf(out, "void init%ld(closure *c, class%ld *var_this);\n", i, i);
    }
    fprintf(out, "\n");
    fprintf(out, "/* Define Classes */\n");
    for (size_t i = 0; i < num_classes; i++) {
        ClassType *class = NULL;
        safe_method_call(state->class_types, get, i, &class);
        fprintf(out, "struct class%ld {\n", i);
        state->indent++;
        size_t field_count;
        char **fields = NULL;
        size_t *field_lengths = NULL;
        safe_method_call(class->field_name_to_type,
                         keys,
                         &field_count,
                         &field_lengths, &fields);
        size_t space_count = 0;
        for (size_t j = 0; j < state->field_counts[i]; j++) {
            print_indent(state->indent, out);
            if (state->fields[i][j] == NULL) {
                fprintf(out, "void *space%ld;\n", space_count++);
            } else {
                VarType *field = NULL;
                safe_method_call(class->field_name_to_type,
                                 get,
                                 state->fields[i][j],
                                 strlen(state->fields[i][j]),
                                 &field);
                fprintf(out, "void *");
                if (field->is_ref) {
                    fprintf(out, "*");
                }
                fprintf(out, "field_%s;\n", state->fields[i][j]);
            }
        }
        if (i < BUILTIN_COUNT) {
            print_indent(state->indent, out);
            fprintf(out, "%sval;\n", BUILTIN_TYPES[i]);
        }
        state->indent--;
        fprintf(out, "};\n");
        if (!class->is_trait) { // Traits don't have constructors
            fprintf(out, "void *new_class%d(closure *c) {\n", class->classID);
            state->indent++;
            print_indent(state->indent, out);
            fprintf(out,
                    "class%d *var_this = malloc(sizeof(class%d));\n",
                    class->classID,
                    class->classID);
            print_indent(state->indent, out);
            fprintf(out, "init%d(c, var_this);\n", class->classID);
            print_indent(state->indent, out);
            fprintf(out, "return var_this;\n");
            state->indent--;
            print_indent(state->indent, out);
            fprintf(out, "}\n");
            if (i < BUILTIN_COUNT) {
                fprintf(out, "void *class%ld_toString(closure *c);\n", i);
                fprintf(out, "void *class%ld_0x%X(closure *c, class%ld "
                             "*other);\n", i, '=', i);
                if (i != BOOL) {
                    for (size_t j = 0; j < BINARY_COUNT; j++) {
                        if (i != STRING || j < STRING_BINARY_COUNT) {
                            fprintf(out, "void *class%ld_0x", i);
                            const char *c = BINARIES[j];
                            do {
                                fprintf(out, "%X", *c);
                            } while (*(++c));
                            fprintf(out,
                                    "(closure *c, class%ld *other);\n",
                                    i);
                        }
                    }
                } else {
                    for (size_t j = 0; j < BOOL_BINARY_COUNT; j++) {
                        fprintf(out, "void *class%ld_0x", i);
                        const char *c = BOOL_BINARIES[j];
                        do {
                            fprintf(out, "%X", *c);
                        } while (*(++c));
                        fprintf(out, "(closure *c, class%ld *other);\n", i);
                    }
                }
            }
            fprintf(out,
                    "void init%ld(closure *c, class%ld *var_this) {\n",
                    i,
                    i);
            state->indent++;
            for (size_t j = 0; j < field_count; j++) {
                print_indent(state->indent, out);
                fprintf(out,
                        "#define var_%.*s var_this->field_%.*s\n",
                        (int) field_lengths[j],
                        fields[j],
                        (int) field_lengths[j],
                        fields[j]);
            }
            const Map *envs = NULL;
            safe_method_call(state->class_envs, get, i, &envs);
            const char **env = NULL;
            size_t env_count;
            size_t *env_lengths;
            safe_method_call(envs, keys, &env_count, &env_lengths, &env);
            size_t env_index = 0;
            for (size_t j = 0; j < env_count; j++) {
                VarType *type = NULL;
                safe_method_call(envs, get, env[j], env_lengths[j], &type);
                if (type->type != TRAIT) {
                    print_indent(1, out);
                    fprintf(out,
                            "#define var_%.*s c->env[%ld]\n",
                            (int) env_lengths[j],
                            env[j],
                            env_index++);
                }
            }
            print_indent(1, out);
            fprintf(out, "build_closure(var_self, new_class%ld", i);
            const char **symbols = NULL;
            size_t *lengths, symbol_count;
            const Map *class_env = NULL;
            safe_method_call(state->class_envs, get, i, &class_env);
            safe_method_call(class_env,
                             keys,
                             &symbol_count,
                             &lengths,
                             &symbols);
            for (size_t j = 0; j < symbol_count; j++) {
                VarType *var_type = NULL;
                safe_method_call(class_env, get, symbols[j], lengths[j], &var_type);
                if (var_type->type != TRAIT) {
                    fprintf(out, ", var_%.*s", (int) lengths[j], symbols[j]);
                }
                free((void *) symbols[j]);
            }
            free(lengths);
            free(symbols);
            fprintf(out, ")\n");
            if (i < BUILTIN_COUNT) {
                print_indent(state->indent, out);
                fprintf(out, "var_val = var_this;\n");
            }
            size_t super_count = class->supers->size(class->supers);
            for (size_t j = 0; j < super_count; j++) {
                size_t superID = 0;
                safe_method_call(class->supers, get, j, &superID);
                print_indent(state->indent, out);
                fprintf(out,
                        "init%ld(c, (class%ld*)var_this);\n",
                        superID,
                        superID);
            }
            const Vector *stmts = NULL;
            safe_method_call(state->class_stmts, get, i, &stmts);
            size_t stmt_count = stmts->size(stmts);
            for (size_t j = 0; j < stmt_count; j++) {
                ASTNode *stmt = NULL;
                safe_method_call(stmts, get, j, &stmt);
                ASTStatementVTable *stmt_vtable = stmt->vtable;
                char *stmt_code = stmt_vtable->codegen(stmt, state, out);
                free(stmt_code);
            }
            if (i < BUILTIN_COUNT) {
                print_indent(state->indent, out);
                fprintf(out,
                        "build_closure(tmp%d, class%ld_toString, var_this)\n",
                        state->tmp_count,
                        i);
                print_indent(state->indent, out);
                fprintf(out, "var_toString = tmp%d;\n", state->tmp_count++);
                print_indent(state->indent, out);
                fprintf(out,
                        "build_closure(tmp%d, class%ld_0x%X, var_this)\n",
                        state->tmp_count,
                        i,
                        '=');
                print_indent(state->indent, out);
                fprintf(out, "var_0x%X = tmp%d;\n", '=', state->tmp_count++);
                if (i != BOOL) {
                    for (size_t j = 0; j < BINARY_COUNT; j++) {
                        if (i != STRING || j < STRING_BINARY_COUNT) {
                            print_indent(state->indent, out);
                            fprintf(out,
                                    "build_closure(tmp%d, class%ld_0x",
                                    state->tmp_count,
                                    i);
                            const char *c = BINARIES[j];
                            do {
                                fprintf(out, "%X", *c);
                            } while (*(++c));
                            fprintf(out, ", var_this)\n");
                            print_indent(state->indent, out);
                            fprintf(out, "var_0x");
                            c = BINARIES[j];
                            do {
                                fprintf(out, "%X", *c);
                            } while (*(++c));
                            fprintf(out, " = tmp%d;\n", state->tmp_count++);
                        }
                    }
                } else {
                    for (size_t j = 0; j < BOOL_BINARY_COUNT; j++) {
                        print_indent(state->indent, out);
                        fprintf(out,
                                "build_closure(tmp%d, class%ld_0x",
                                state->tmp_count,
                                i);
                        const char *c = BOOL_BINARIES[j];
                        do {
                            fprintf(out, "%X", *c);
                        } while (*(++c));
                        fprintf(out, ", var_this)\n");
                        print_indent(state->indent, out);
                        fprintf(out, "var_0x");
                        c = BOOL_BINARIES[j];
                        do {
                            fprintf(out, "%X", *c);
                        } while (*(++c));
                        fprintf(out, " = tmp%d;\n", state->tmp_count++);
                    }
                }

                print_indent(state->indent, out);
                fprintf(out, "var_this->val = 0;\n");
            }
            for (size_t j = 0; j < env_count; j++) {
                print_indent(1, out);
                fprintf(out, "#undef var_%.*s\n", (int) env_lengths[j], env[j]);
            }
            for (size_t j = 0; j < field_count; j++) {
                print_indent(state->indent, out);
                fprintf(out,
                        "#undef var_%.*s\n",
                        (int) field_lengths[j],
                        fields[j]);
            }
            state->indent--;
            fprintf(out, "}\n");
            fprintf(out, "\n");
        }
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
                fprintf(out, "void *");
                if (arg->type->type == REFERENCE) {
                    fprintf(out, "*");
                }
                fprintf(out, "var_%s = arg;\n", arg->name);
            } else {
                for (size_t j = 0; j < arg_count; j++) {
                    NamedType *arg = NULL;
                    safe_method_call(data->type->function->named_args,
                                     get,
                                     j,
                                     &arg);
                    print_indent(state->indent, out);
                    fprintf(out, "void *");
                    if (arg->type->type == REFERENCE) {
                        fprintf(out, "*");
                    }
                    fprintf(out, "var_%s = ((void**)arg)[%ld];\n",
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
                    fprintf(out, "%svar_%.*s", sep, (int) lengths[j],
                            symbols[j]);
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
            size_t env_index = 0;
            for (size_t j = 0; j < env_count; j++) {
                VarType *type = NULL;
                safe_method_call(data->env, get, env[j], env_lengths[j], &type);
                if (type->type != TRAIT) {
                    print_indent(1, out);
                    fprintf(out, "#define var_%.*s c->env[%ld]\n",
                            (int) env_lengths[j],
                            env[j],
                            env_index++);
                }
            }
            print_indent(1, out);
            fprintf(out, "#define var_super var_self\n");
            size_t stmt_count = data->stmts->size(data->stmts);
            for (size_t j = 0; j < stmt_count; j++) {
                ASTNode *stmt = NULL;
                safe_method_call(data->stmts, get, j, &stmt);
                ASTNodeVTable *vtable = stmt->vtable;
                char *code = vtable->codegen(stmt, state, out);
                free(code);
            }
            print_indent(1, out);
            fprintf(out, "#undef var_super\n");
            for (size_t j = 0; j < env_count; j++) {
                print_indent(1, out);
                fprintf(out, "#undef var_%.*s\n",
                        (int) env_lengths[j],
                        env[j]);
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
        if (type->type != TRAIT) {
            fprintf(out, "%svar_%.*s", sep, (int) lengths[i], symbols[i]);
            sep = ", *";
        }
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
                "build_closure(tmp%d, builtin_%s)\n",
                state->tmp_count,
                builtins[i]);
        print_indent(state->indent, out);
        fprintf(out, "var_%s = tmp%d;\n", builtins[i], state->tmp_count++);
    }
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        print_indent(state->indent, out);
        fprintf(out,
                "build_closure(tmp%d, new_class%ld)\n",
                state->tmp_count,
                i);
        print_indent(state->indent, out);
        fprintf(out,
                "var_%s = tmp%d;\n",
                BUILTIN_NAMES[i],
                state->tmp_count++);
    }

    size_t stmt_count = data->statements->size(data->statements);
    for (size_t i = 0; i < stmt_count; i++) {
        ASTNode *stmt = NULL;
        safe_method_call(data->statements, get, i, &stmt);
        ASTStatementVTable *vtable = stmt->vtable;
        char *code = vtable->codegen(stmt, state, out);
        free(code);
    }
    state->indent--;
    fprintf(out, "}\n");
    fprintf(out, "\n");
    return 0;
}

int GenerateCode(const ASTNode *program, FILE *out) {
    const ASTNode *node = program;
    ASTProgramData *data = node->data;

    CodegenState *codegen_state = malloc(sizeof(*codegen_state));
    if (codegen_state == NULL) {
        print_ICE("unable to allocate memory");
    }
    codegen_state->indent = 0;
    codegen_state->tmp_count = 0;
    codegen_state->func_defs = data->func_defs;
    codegen_state->class_types = data->class_types;
    codegen_state->class_stmts = data->class_stmts;
    codegen_state->class_envs  = data->class_envs;
    codegen_state->fields = data->fields;
    codegen_state->field_counts = data->field_counts;

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
