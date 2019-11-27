#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/trait.h"
#include "ast/program.h"
#include "ast/statement.h"
#include "codegen.h"

static void free_trait(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTTraitData  *data = node->data;
    free_VarType(data->type);
    data->supers->free(data->supers, free_NamedType);
    data->stmts->free(data->stmts, free_NamedType);
    data->fields->free(data->fields, NULL);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_trait(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Trait\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTTraitData  *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"extends\": ");
    json_vector(data->supers, indent, out, json_string);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Trait(const ASTNode *node,
                         const Map *symbols,
                         TypeCheckState *state,
                         VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTTraitData *data = node->data;
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int traitID = program_data->class_types->size(program_data->class_types);

    // Initialize trait's type object
    safe_function_call(new_TraitType, &data->type);
    ClassType *trait = data->type->class;
    trait->classID = traitID;
    *type_ptr = data->type;

    // Update program data with new trait information
    safe_method_call(program_data->class_types, append, trait);
    safe_method_call(program_data->class_envs,  append, NULL);
    safe_method_call(program_data->class_stmts, append, NULL);

    // Iterate through parent traits/classes, collecting their fields
    size_t super_count = data->supers->size(data->supers);
    for (size_t i = 0; i < super_count; i++) {
        char *super_name = NULL;
        safe_method_call(data->supers, get, i, &super_name);
        VarType *super = NULL;
        if(symbols->get(symbols, super_name, strlen(super_name), &super)) {
            //TODO: Handle unrecognized super name
            fprintf(stderr, "error: class inherits from unrecognized "
                            "type\n");
            return 1;
        }
        const Map *field_map = super->class->field_name_to_type;
        size_t field_count = 0;
        size_t *field_lengths = NULL;
        char **field_names = NULL;
        safe_method_call(field_map,
                         keys,
                         &field_count,
                         &field_lengths,
                         &field_names);
        for (size_t j = 0; j < field_count; j++) {
            VarType *field_type = NULL;
            safe_method_call(field_map,
                             get,
                             field_names[j],
                             field_lengths[j],
                             &field_type);
            VarType *prev_type = NULL;
            VarType *type_copy = NULL;
            safe_function_call(copy_VarType, field_type, &type_copy);
            prev_type = NULL;
            safe_method_call(trait->field_name_to_type,
                             put,
                             field_names[j],
                             field_lengths[j],
                             type_copy,
                             &prev_type);
            if (prev_type != NULL &&
                typecmp(type_copy, prev_type, state, symbols, NULL)) {
                //TODO: Handle incompatible fields
                fprintf(stderr, "error: field \"%.*s\" inherited from "
                                "\"%s\" shadows existing field\n",
                        (int)field_lengths[j],
                        field_names[j],
                        super_name);
                return 1;
            }
            free_VarType(prev_type);
            free(field_names[j]);
        }
        free(field_names);
        free(field_lengths);
    }

    size_t field_count = data->stmts->size(data->stmts);
    for (size_t i = 0; i < field_count; i++) {
        NamedType *field = NULL;
        safe_method_call(data->stmts, get, i, &field);
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, field->type, &type_copy);
        VarType *prev_type = NULL;
        safe_method_call(trait->field_name_to_type,
                         put,
                         field->name,
                         strlen(field->name),
                         type_copy,
                         &prev_type);
        if (prev_type != NULL &&
            typecmp(type_copy, prev_type, state, symbols, NULL)) {
            //TODO: Handle incompatible fields
            fprintf(stderr, "error: field \"%s\" shadows existing field\n",
                    field->name);
            return 1;
        }
        free_VarType(prev_type);
    }
    return 0;
}

static char *CodeGen_Trait(UNUSED const ASTNode *node,
                           UNUSED CodegenState *state,
                           UNUSED FILE *out) {
    return NULL;
}

const ASTNode *new_TraitNode(struct YYLTYPE *loc,
                             const Vector *supers,
                             const Vector *stmts) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_trait_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_trait_vtable *vtable = malloc(sizeof(*vtable));
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
    data->supers = supers;
    data->stmts       = stmts;
    data->fields      = new_Map(0, 0);
    data->type        = NULL;
    data->name        = NULL;
    data->is_hold     = 0;
    vtable->free      = free_trait;
    vtable->json      = json_trait;
    vtable->get_type  = GetType_Trait;
    vtable->codegen   = CodeGen_Trait;
    return node;
}
