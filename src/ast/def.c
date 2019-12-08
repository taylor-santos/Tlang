#include <stdlib.h>
#include <string.h>
#include <ast/program.h>
#include <ast/class.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/def.h"
#include "ast/l_expr.h"
#include "codegen.h"
#include "ast/statement.h"

static void free_def(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTDefData    *data = node->data;
    data->lhs->free(data->lhs, free_ASTNode);
    free_ASTNode((void *) data->rhs);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_def(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Def\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTDefData    *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"lhs\": ");
    json_vector(data->lhs, indent, out, json_ASTNode);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"rhs\": ");
    node = data->rhs;
    ASTNodeVTable *vtable = node->vtable;
    vtable->json(node, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Def(const ASTNode *node,
                       const Map *symbols,
                       TypeCheckState *state,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTDefData *data = node->data;
    const Vector *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTDefVTable *rhs_vtable = rhs->vtable;
    *type_ptr = NULL;
    if (rhs_vtable->get_type(rhs, symbols, state, type_ptr)) {
        return 1;
    }
    if (*type_ptr == NULL) {
        //TODO: Handle var := none
        fprintf(stderr, "error: variable assigned to none\n");
        return 1;
    }
    size_t lhs_size = lhs->size(lhs);
    if (lhs_size != 1 && (*type_ptr)->type == TUPLE) {
        size_t rhs_size = (*type_ptr)->tuple->size((*type_ptr)->tuple);
        if (lhs_size != rhs_size) {
            //TODO: Handle wrong number of variables assigned to tuple
            fprintf(stderr, "error: wrong number of variables defined to "
                            "tuple\n");
            return 1;
        }
        for (size_t i = 0; i < lhs_size; i++) {
            const ASTNode *lhs_node = NULL;
            safe_method_call(lhs, get, i, &lhs_node);
            ASTLExprData *lhs_data = lhs_node->data;
            VarType *prev_type = NULL;
            VarType *expected_type = NULL;
            safe_method_call((*type_ptr)->tuple, get, i, &expected_type);
            if (symbols->get(symbols,
                             lhs_data->name,
                             strlen(lhs_data->name),
                             &prev_type)) {
                VarType *type_copy = NULL;
                safe_function_call(copy_VarType, expected_type, &type_copy);
                safe_method_call(symbols,
                                 put,
                                 lhs_data->name,
                                 strlen(lhs_data->name),
                                 type_copy,
                                 NULL);
                if (state->curr_class != NULL) {
                    ASTClassData *class = state->curr_class->data;
                    safe_function_call(copy_VarType, expected_type, &type_copy);
                    prev_type = NULL;
                    safe_method_call(class->type->class->field_name_to_type,
                                     put,
                                     lhs_data->name,
                                     strlen(lhs_data->name),
                                     type_copy,
                                     &prev_type);
                }
            } else if (typecmp(expected_type,
                               prev_type,
                               state,
                               symbols,
                               NULL)) {
                //TODO: Handle incompatible type def
                fprintf(stderr, "error: incompatible type reassignment\n");
                return 1;
            }
        }
    } else {
        if (lhs_size != 1) {
            //TODO: Handle multiple variables assigned to single object
            fprintf(stderr, "error: multiple variables defined to be "
                            "non-tuple type\n");
            return 1;
        }
        const ASTNode *lhs_node = NULL;
        safe_method_call(lhs, get, 0, &lhs_node);
        ASTLExprData *lhs_data = lhs_node->data;
        VarType *prev_type = NULL;
        VarType *expected_type = *type_ptr;
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, expected_type, &type_copy);
        safe_method_call(symbols,
                         put,
                         lhs_data->name,
                         strlen(lhs_data->name),
                         type_copy,
                         &prev_type);

        if (prev_type != NULL &&
            typecmp(expected_type, prev_type, state, symbols, NULL)) {
            //TODO: Handle def to incompatible type
            fprintf(stderr,
                    "error: assignment to incompatible type\n");
            return 1;
        }
        lhs_data->type = type_copy;
        if (state->curr_class != NULL) {
            ASTClassData *class = state->curr_class->data;
            safe_function_call(copy_VarType, expected_type, &type_copy);
            safe_method_call(class->type->class->field_name_to_type,
                             put,
                             lhs_data->name,
                             strlen(lhs_data->name),
                             type_copy,
                             &prev_type);
        }
    }

    data->type = *type_ptr;
    return 0;
}

static char *CodeGen_Def(const ASTNode *node, CodegenState *state, FILE *out) {
    ASTDefData *data = node->data;
    size_t lhs_size = data->lhs->size(data->lhs);
    const ASTNode *rhs = data->rhs;
    ASTStatementVTable *rhs_vtable = rhs->vtable;
    ASTStatementData *rhs_data = rhs->data;
    char *rhs_code = rhs_vtable->codegen(rhs, state, out);
    if (rhs_code == NULL) {
        return NULL;
    }
    if (lhs_size != 1 && rhs_data->type->type == TUPLE) {
        for (size_t i = 0; i < lhs_size; i++) {
            ASTNode *lhs = NULL;
            safe_method_call(data->lhs, get, i, &lhs);
            ASTLExprVTable *lhs_vtable = lhs->vtable;
            char *lhs_code = lhs_vtable->codegen(lhs, state, out);
            print_indent(state->indent, out);
            ASTLExprData *lhs_data = lhs->data;
            if (lhs_data->type->is_ref) {
                fprintf(out, "*");
            }
            fprintf(out, "%s = %s[%ld];\n", lhs_code, rhs_code, i);
        }
    } else {
        ASTNode *lhs = NULL;
        safe_method_call(data->lhs, get, 0, &lhs);
        ASTLExprVTable *lhs_vtable = lhs->vtable;
        char *lhs_code = lhs_vtable->codegen(lhs, state, out);
        print_indent(state->indent, out);
        ASTLExprData *lhs_data = lhs->data;
        if (lhs_data->type->is_ref) {
            fprintf(out, "*");
        }
        fprintf(out, "%s = %s;\n", lhs_code, rhs_code);
    }
    return rhs_code;
}

const ASTNode *new_DefNode(struct YYLTYPE *loc,
                           const Vector *lhs,
                           const void *rhs) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_def_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_def_vtable *vtable = malloc(sizeof(*vtable));
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
    data->lhs        = lhs;
    data->rhs        = rhs;
    data->type       = NULL;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_def;
    vtable->json     = json_def;
    vtable->get_type = GetType_Def;
    vtable->codegen  = CodeGen_Def;
    return node;
}
