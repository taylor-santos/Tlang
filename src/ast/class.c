#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/class.h"
#include "ast/program.h"
#include "ast/statement.h"
#include "codegen.h"
#include "builtins.h"

static void free_class(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTClassData  *data = node->data;
    free_VarType(data->type);
    data->inheritance->free(data->inheritance, free_NamedType);
    data->stmts->free(data->stmts, free_ASTNode);
    data->symbols->free(data->symbols, free_VarType);
    data->fields->free(data->fields, NULL);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_class(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Class\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTClassData  *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"extends\": ");
    json_vector(data->inheritance, indent, out, json_string);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Class(const ASTNode *node,
                         const Map *symbols,
                         TypeCheckState *state,
                         VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTClassData *data = node->data;
    safe_function_call(new_ClassType, &data->type);
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int classID = program_data->class_types->size(program_data->class_types);
    data->type->class->classID = classID;
    data->type->class->instance->object->id_type = ID;
    data->type->class->instance->object->classID = classID;
    *type_ptr = data->type;
    safe_method_call(program_data->class_types, append, data->type->class);
    safe_method_call(program_data->class_stmts, append, data->stmts);
    const Vector *stmts = data->stmts;
    size_t size = stmts->size(stmts);
    state->new_symbols->clear(state->new_symbols, free_NamedType);
    safe_method_call(symbols, copy, &data->symbols, copy_VarType);
    safe_method_call(data->symbols, copy, &data->env, copy_VarType);
    VarType *self_type = NULL;
    safe_function_call(copy_VarType, data->type->class->instance, &self_type);
    VarType *prev_self = NULL;
    char *self_name = "var_self";
    safe_method_call(data->symbols,
                     put,
                     self_name,
                     strlen(self_name),
                     self_type,
                     &prev_self);
    if (prev_self != NULL) {
        free_VarType(prev_self);
    }
    for (size_t i = 0; i < size; i++) {
        const ASTNode *stmt = NULL;
        safe_method_call(stmts, get, i, &stmt);
        ASTStatementVTable *vtable = stmt->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt,
                             data->symbols,
                             state,
                             &type)) {
            return 1;
        }
    }
    size_t num_fields = state->new_symbols->size(state->new_symbols);
    for (size_t i = 0; i < num_fields; i++) {
        NamedType *field = NULL;
        safe_method_call(state->new_symbols, get, i, &field);
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, field->type, &type_copy);
        safe_method_call(data->type->class->field_name_to_type,
                         put,
                         field->name,
                         strlen(field->name),
                         type_copy,
                         NULL);
    }
    //Clear the new_symbols vector without freeing its elements. They have been
    //moved to the class's fields list.
    state->new_symbols->clear(state->new_symbols, NULL);
    return 0;
}

static char *CodeGen_Class(const ASTNode *node,
                           CodegenState *state,
                           FILE *out) {
    ASTClassData *data = node->data;
    VarType *type = data->type;
    ClassType *class = type->class;
    char *ret;
    safe_asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "void *%s;\n", ret);
    print_indent(state->indent, out);
    fprintf(out, "build_closure(%s, new_class%d", ret, class->classID);
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

const ASTNode *new_ClassNode(
        struct YYLTYPE *loc, const Vector *inheritance, const Vector *stmts
) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_class_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_class_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->inheritance = inheritance;
    data->stmts       = stmts;
    data->fields      = new_Map(0, 0);
    data->type        = NULL;
    data->symbols     = NULL;
    data->name        = NULL;
    data->is_hold     = 0;
    vtable->free      = free_class;
    vtable->json      = json_class;
    vtable->get_type  = GetType_Class;
    vtable->codegen   = CodeGen_Class;
    return node;
}
