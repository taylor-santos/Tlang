#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/expression.h"
#include "ast/statement.h"
#include "ast/program.h"
#include "ast/paren.h"
#include "builtins.h"
#include "codegen.h"

static int parse_function(const Vector *exprs,
                          size_t *index,
                          const Map *symbols,
                          TypeCheckState *state,
                          Expression **expr_ptr,
                          VarType **vartype_ptr);

static int parse_class(const Vector *exprs,
                       size_t *index,
                       const Map *symbols,
                       TypeCheckState *state,
                       Expression **expr_ptr,
                       VarType **vartype_ptr);

static int parse_expression(const Vector *exprs,
                            size_t *index,
                            const Map *symbols,
                            TypeCheckState *state,
                            Expression **expr_ptr,
                            VarType **vartype_ptr);

static int parse_paren(const Vector *exprs,
                       size_t *index,
                       const Map *symbols,
                       TypeCheckState *state,
                       Expression **expr_ptr,
                       VarType **vartype_ptr);

static void free_expression(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode     *node = this;
    ASTExpressionData *data = node->data;
    data->exprs->free(data->exprs, free_ASTNode);
    //free_Expression(data->expr_node);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_expression(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Expression\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode     *node = this;
    ASTExpressionData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"exprs\": ");
    json_vector(data->exprs, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int parse_object(const Vector *exprs,
                        size_t *index,
                        UNUSED const Map *symbols,
                        TypeCheckState *state,
                        Expression **expr_ptr,
                        VarType **vartype_ptr) {
    size_t size = exprs->size(exprs);
    if (*index == size) {
        return 0;
    }
    const ASTNode *node = NULL;
    safe_method_call(exprs, get, *index, &node);
    (*index)++;
    ASTStatementData *data = node->data;
    char *name = data->name;
    if (name == NULL) {
        //TODO: Node is not a field
        fprintf(stderr, "error: invalid dangling expression\n");
        return 1;
    }
    //Node might be a field
    ASTProgramData *program_data = state->program_node->data;
    VarType *object_type = NULL;
    safe_function_call(getObjectClass,
                       (*vartype_ptr)->object,
                       symbols,
                       program_data->class_types,
                       &object_type);
    const Map *field_names = object_type->class->field_name_to_type;
    if (field_names->get(field_names, name, strlen(name), vartype_ptr)) {
        fprintf(stderr,
                "error: object does not have a field named \"%s\"\n",
                name);
        return 1;
    }
    safe_function_call(copy_VarType, *vartype_ptr, &data->type);
    Expression *expr = malloc(sizeof(*expr));
    expr->expr_type = EXPR_FIELD;
    expr->sub_expr = *expr_ptr;
    expr->name = name;
    expr->type = object_type;
    safe_method_call(exprs, get, *index-1, &expr->node);

    *expr_ptr = expr;

    if (parse_expression(exprs,
                         index,
                         symbols,
                         state,
                         expr_ptr,
                         vartype_ptr)) {
        return 1;
    }
    return 0;
}

static int parse_paren(const Vector *exprs,
                       size_t *index,
                       const Map *symbols,
                       TypeCheckState *state,
                       Expression **expr_ptr,
                       VarType **vartype_ptr) {
    const ASTNode *node = NULL;
    safe_method_call(exprs, get, *index-1, &node);
    ASTParenData *data = node->data;
    const ASTNode *sub_node = data->val;
    ASTExpressionData *sub_data = sub_node->data;
    const Vector *sub_exprs = sub_data->exprs;

    const ASTNode *first_node = NULL;
    safe_method_call(sub_exprs, get, 0, &first_node);
    ASTStatementVTable *vtable = first_node->vtable;
    if (vtable->get_type(first_node,
                         symbols,
                         state,
                         vartype_ptr)) {
        return 1;
    }
    size_t sub_index = 1;
    (*expr_ptr)->expr_type = EXPR_PAREN;
    (*expr_ptr)->sub_expr = malloc(sizeof(Expression));
    if ((*expr_ptr)->sub_expr == NULL) {
        print_ICE("unable to allocate memory");
    }
    (*expr_ptr)->sub_expr->expr_type = EXPR_VAR;
    (*expr_ptr)->sub_expr->node = first_node;
    (*expr_ptr)->sub_expr->type = *vartype_ptr;
    (*expr_ptr)->type = *vartype_ptr;
    if (parse_expression(sub_exprs,
                         &sub_index,
                         symbols,
                         state,
                         &(*expr_ptr)->sub_expr,
                         vartype_ptr)) {
        return 1;
    }
    data->type = *vartype_ptr;
    if (parse_expression(exprs,
                         index,
                         symbols,
                         state,
                         expr_ptr,
                         vartype_ptr)) {
        return 1;
    }
    return 0;
}

static int parse_function(const Vector *exprs,
                          size_t *index,
                          const Map *symbols,
                          TypeCheckState *state,
                          Expression **expr_ptr,
                          VarType **vartype_ptr) {
    if (*expr_ptr == NULL) {
        print_ICE("unable to allocate memory");
    }
    Expression *expr = malloc(sizeof(*expr));
    if (expr == NULL) {
        print_ICE("unable to allocate memory");
    }
    expr->expr_type = EXPR_FUNC;
    expr->sub_expr = *expr_ptr;
    *expr_ptr = expr;

    size_t size = exprs->size(exprs);
    FuncType *function = (*vartype_ptr)->function;
    size_t arg_count = function->named_args->size(function->named_args);
    if (arg_count > 0) {
        if (*index == size) {
            //TODO: Handle missing arguments
            fprintf(stderr, "error: function call given no arguments\n");
            return 1;
        }
        const ASTNode *next_expr = NULL;
        safe_method_call(exprs, get, *index, &next_expr);
        ASTStatementVTable *vtable = next_expr->vtable;
        VarType *given_type = NULL;
        if (vtable->get_type(next_expr, symbols, state, &given_type)) {
            return 1;
        }
        (*index)++;
        if (given_type->type == HOLD) {
            (*expr_ptr)->expr_type = EXPR_HOLD;
            return 0;
        }
        (*expr_ptr)->arg = malloc(sizeof(Expression));
        if ((*expr_ptr)->arg == NULL) {
            print_ICE("unable to allocate memory");
        }
        (*expr_ptr)->arg->expr_type = EXPR_VAR;
        (*expr_ptr)->arg->node = next_expr;
        (*expr_ptr)->arg->type = given_type;
        if (parse_expression(exprs,
                             index,
                             symbols,
                             state,
                             &(*expr_ptr)->arg,
                             &given_type)) {
            return 1;
        }
        if (given_type == NULL) {
            //TODO: Handle none as argument
            fprintf(stderr, "error: object with type 'none' used as "
                            "argument\n");
            return 1;
        }
        if (arg_count == 1) {
            if (given_type->type == TUPLE) {
                //TODO: Handle too many arguments
                fprintf(stderr, "error: %ld arguments passed to function, "
                                "expected 1\n",
                        given_type->tuple->size(given_type->tuple));
                return 1;
            }
            NamedType *expected_type = NULL;
            safe_method_call(function->named_args, get, 0, &expected_type);
            if (typecmp(given_type,
                        expected_type->type,
                        state,
                        symbols,
                        NULL)) {
                //TODO: Handle incompatible argument type
                fprintf(stderr, "error: incompatible argument type\n");
                return 1;
            }
        } else {
            if (given_type->type != TUPLE) {
                //TODO: Handle not enough arguments
                fprintf(stderr, "error: 1 argument passed to function, "
                                "expected %ld\n",
                        arg_count);
                return 1;
            }
            size_t given_count = given_type->tuple->size(given_type->tuple);
            if (given_count != arg_count) {
                //TODO: Handle wrong number of arguments
                fprintf(stderr, "error: %ld arguments passed to function, "
                                "expected %ld\n",
                        given_count,
                        arg_count);
                return 1;
            }
            for (size_t i = 0; i < arg_count; i++) {
                VarType *arg_type = NULL;
                safe_method_call(given_type->tuple, get, i, &arg_type);
                NamedType *expected_type = NULL;
                safe_method_call(function->named_args, get, i, &expected_type);
                if (typecmp(arg_type,
                            expected_type->type,
                            state,
                            symbols,
                            NULL)) {
                    //TODO: Handle incompatible argument type
                    fprintf(stderr, "error: incompatible argument type\n");
                    return 1;
                }
            }
        }
    } else {
        (*expr_ptr)->arg = NULL;
        if (*index < size) {
            //Check if the function is followed by a Hold
            const ASTNode *next_expr = NULL;
            safe_method_call(exprs, get, *index, &next_expr);
            ASTStatementData *data = next_expr->data;
            if (data->is_hold) {
                (*expr_ptr)->expr_type = EXPR_HOLD;
                (*expr_ptr)->type = *vartype_ptr;
                (*index)++;
                return 0; // Return without evaluating return type of function
            }
        }
    }
    *vartype_ptr = function->ret_type;
    (*expr_ptr)->type = *vartype_ptr;
    return 0;
}

static int parse_class(const Vector *exprs,
                       size_t *index,
                       UNUSED const Map *symbols,
                       UNUSED TypeCheckState *state,
                       Expression **expr_ptr,
                       VarType **vartype_ptr) {
    size_t size = exprs->size(exprs);
    if (*index < size) {
        const ASTNode *next_expr = NULL;
        safe_method_call(exprs, get, *index, &next_expr);
        ASTStatementData *data = next_expr->data;
        if (data->is_hold) {
            (*index)++;
            return 0;
        }
    }
    *vartype_ptr = (*vartype_ptr)->class->instance;
    Expression *expr = malloc(sizeof(*expr));
    expr->expr_type = EXPR_CONS;
    expr->sub_expr = *expr_ptr;
    expr->type = *vartype_ptr;
    safe_method_call(exprs, get, *index-1, &expr->node);
    *expr_ptr = expr;
    return 0;
}

static int parse_expression(const Vector *exprs,
                            size_t *index,
                            const Map *symbols,
                            TypeCheckState *state,
                            Expression **expr_ptr,
                            VarType **vartype_ptr) {
    switch((*vartype_ptr)->type) {
        case HOLD:
        case TUPLE:
        case REFERENCE:
            if (exprs->size(exprs) > *index) {
                //TODO: Handle dangling expression
                fprintf(stderr, "error: dangling expression\n");
                return 1;
            }
            return 0;
        case FUNCTION:
            if (parse_function(exprs,
                               index,
                               symbols,
                               state,
                               expr_ptr,
                               vartype_ptr)) {
                return 1;
            }
            break;
        case CLASS:
            if (parse_class(exprs,
                            index,
                            symbols,
                            state,
                            expr_ptr,
                            vartype_ptr)) {
                return 1;
            }
            break;
        case OBJECT:
            if (parse_object(exprs,
                             index,
                             symbols,
                             state,
                             expr_ptr,
                             vartype_ptr)) {
                return 1;
            }
            break;
        case PAREN:
            if (parse_paren(exprs,
                            index,
                            symbols,
                            state,
                            expr_ptr,
                            vartype_ptr)) {
                return 1;
            }
            break;
        case TRAIT:
            //TODO: Handle trait used in expression
            fprintf(stderr, "error: trait cannot be used in an expression\n");
            return 1;
    }
    size_t size = exprs->size(exprs);
    if (*index < size) {
        if (parse_expression(exprs,
                             index,
                             symbols,
                             state,
                             expr_ptr,
                             vartype_ptr)) {
            return 1;
        }
        if (*index < size) {
            //TODO: Handle dangling expression
            fprintf(stderr, "error: dangling expression\n");
            return 1;
        }
    }
    return 0;
}

static int GetType_Expression(const ASTNode *node,
                              const Map *symbols,
                              TypeCheckState *state,
                              VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTExpressionData *data = node->data;
    const Vector *exprs = data->exprs;

    const ASTNode *first_node = NULL;
    safe_method_call(exprs, get, 0, &first_node);
    ASTStatementVTable *vtable = first_node->vtable;
    if (vtable->get_type(first_node,
                         symbols,
                         state,
                         type_ptr)) {
        return 1;
    }
    size_t index = 1;
    data->expr_node = malloc(sizeof(*data->expr_node));
    if (data->expr_node == NULL) {
        print_ICE("unable to allocate memory");
    }
    data->expr_node->node = first_node;
    data->expr_node->type = *type_ptr;
    data->expr_node->expr_type = EXPR_VAR;
    if (parse_expression(exprs,
                         &index,
                         symbols,
                         state,
                         &data->expr_node,
                         type_ptr)) {
        return 1;
    }
    data->type = *type_ptr;
    return 0;
}


char *gen_expression(Expression *expr, CodegenState *state, FILE *out) {
    char *tmp = NULL, *sub_expr = NULL;
    const ASTNode *node = NULL;
    ASTStatementVTable *vtable = NULL;
    switch (expr->expr_type) {
        case EXPR_VAR:
            node = expr->node;
            vtable = node->vtable;
            return vtable->codegen(node, state, out);
        case EXPR_FIELD:
            asprintf(&tmp, "tmp%d", state->tmp_count++);
            sub_expr = gen_expression(expr->sub_expr, state, out);
            print_indent(state->indent, out);
            VarType *expr_type = expr->type;
            fprintf(out,
                    "void *%s = ((class%d*)%s)->field_%s;\n",
                    tmp,
                    expr_type->class->classID,
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
                fprintf(out, ", ");
                if (expr->arg->type->is_ref) {
                    fprintf(out, "*(void**)");
                }
                fprintf(out, "%s", arg);
            }
            fprintf(out, ");\n");
            return tmp;
        case EXPR_PAREN:
        case EXPR_HOLD:
            return gen_expression(expr->sub_expr, state, out);
    }
    fprintf(stderr, "error: unmatched expression\n");
    return NULL;
}

static char *CodeGen_Expression(const ASTNode *node,
                                CodegenState *state,
                                FILE *out) {
    ASTExpressionData *data = node->data;
    return gen_expression(data->expr_node, state, out);
}

const ASTNode *new_ExpressionNode(struct YYLTYPE *loc, const Vector *exprs) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_expression_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_expression_vtable *vtable = malloc(sizeof(*vtable));
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
    data->exprs      = exprs;
    data->type       = NULL;
    data->expr_node  = NULL;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_expression;
    vtable->json     = json_expression;
    vtable->get_type = GetType_Expression;
    vtable->codegen  = CodeGen_Expression;
    return node;
}
