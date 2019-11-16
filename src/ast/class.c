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
                         TypeCheckState *typecheck_state,
                         VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTClassData *data = node->data;

    TypeCheckState *state = typecheck_state;
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int classID = program_data->class_defs->size(program_data->class_defs);
    safe_function_call(new_ClassType, &data->type);
    data->type->class->def = data->type->class;
    data->type->class->stmts = data->stmts;
    data->type->class->classID = classID;
    data->type->class->instance->object->classID = classID;
    data->type->class->fields = new_Vector(0);
    data->type->class->field_names = new_Map(0, 0);
    *type_ptr = data->type;

    const Vector *stmts = data->stmts;
    size_t size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(stmts, array, &size, &stmt_arr);
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
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt_arr[i],
                             data->symbols,
                             typecheck_state,
                             &type)) {
            return 1;
        }
    }
    free(stmt_arr);
    size_t num_fields = state->new_symbols->size(state->new_symbols);
    for (size_t i = 0; i < num_fields; i++) {
        NamedType *field = NULL;
        safe_method_call(state->new_symbols, get, i, &field);
        safe_method_call(data->type->class->fields, append, field);
        safe_method_call(data->type->class->field_names,
                         put,
                         field->name,
                         strlen(field->name),
                         field->type,
                         NULL);
    }
    size_t parent_count = data->inheritance->size(data->inheritance);
    for (size_t i = 0; i < parent_count; i++) {
        NamedType *parent_type = NULL;
        safe_method_call(data->inheritance, get, i, &parent_type);
        if (getObjectID(parent_type->type, symbols)) {
            //TODO: Handle unrecognized parent type
            fprintf(stderr, "error: class inherits from unrecognized type\n");
            return 1;
        }
        if (parent_type->type->type != OBJECT) {
            //TODO: Handle inheritance from non-class
            fprintf(stderr, "error: class inherits from non-class\n");
            return 1;
        }
        int parentID = parent_type->type->object->classID;
        const ASTNode *parent_node = NULL;
        program_data->class_defs->get(program_data->class_defs,
                                      parentID,
                                      &parent_node);
        ASTStatementData *parent_data = parent_node->data;
        VarType *parent_class = parent_data->type;
        int field_count =
                parent_class->class->fields->size(parent_class->class->fields);
        for (int j = 0; j < field_count; j++) {
            NamedType *field = NULL;
            safe_method_call(parent_class->class->fields, get, j, &field);
            if (!data->type->class->field_names->contains(
                    data->type->class->field_names,
                    field->name,
                    strlen(field->name))) {
                //TODO: Handle inherited field not initialized
                fprintf(stderr, "error: field %s inherited from class %s "
                                "not initialized\n",
                        field->name+strlen("var_"),
                        parent_type->type->object->className+strlen
                                ("var_"));
                return 1;
            }
        }
    }
    //Clear the new_symbols vector without freeing its elements. They have been
    //moved to the class's fields list.
    state->new_symbols->clear(state->new_symbols, NULL);
    safe_method_call(program_data->class_defs, append, node);
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
