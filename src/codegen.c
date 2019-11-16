#include <string.h>
#include <stdio.h>
#include "safe.h"
#include "builtins.h"
#include "map.h"
#include "vector.h"
#include "codegen.h"
#include "typechecker.h"
#include "ast/class.h"
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
