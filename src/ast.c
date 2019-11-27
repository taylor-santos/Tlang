#include <string.h>
#include "ast.h"
#include "Tlang_parser.h"
#include "safe.h"

void free_ASTNode(void *this) {
    if (this == NULL) {
        return;
    }
    // Call the vtable->free() function on any subtype of an AST node
    const ASTNode *node   = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->free(node);
}

void json_ASTNode(const void *this, int indent, FILE *out) {
    const ASTNode *node   = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->json(node, indent, out);
}

void json_vector(const Vector *vec,
                 int indent,
                 FILE *out,
                 void(*element_json)(const void *, int, FILE *)) {
    char *sep = "\n";
    fprintf(out, "[");
    int size = vec->size(vec);
    if (size > 0) {
        indent++;
        for (int i = 0; i < size; i++) {
            fprintf(out, "%s", sep);
            fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
            const void *element = NULL;
            if (vec->get(vec, i, &element)) {
                return;
            }
            element_json(element, indent, out);
            sep = ",\n";
        }
        fprintf(out, "\n");
        indent--;
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "]");
}

void json_VarType(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": ");
    const VarType *type = this;
    if (type == NULL) {
        fprintf(out, "\"none\"\n");
    } else {
        switch (type->type) {
            case FUNCTION:
                fprintf(out, "\"func\",\n");
                fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
                fprintf(out, "\"signature\": ");
                json_FuncType(type->function, indent, out);
                fprintf(out, "\n");
                break;
            case REFERENCE:
                fprintf(out, "\"ref\",\n");
                fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
                fprintf(out, "\"ref_type\": ");
                json_VarType(type->sub_type, indent, out);
                fprintf(out, "\n");
                break;
            case TRAIT:
                fprintf(out, "\"trait\"\n");
                break;
            case CLASS:
                fprintf(out, "\"class\"\n");
                break;
            case OBJECT:
                fprintf(out, "\"object\"\n");
                break;
            case HOLD:
                fprintf(out, "\"hold\"\n");
                fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
                fprintf(out, "\"hold_type\": ");
                json_VarType(type->sub_type, indent, out);
                fprintf(out, "\n");
                break;
            case TUPLE:
                fprintf(out, "\"tuple\"\n");
                fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
                fprintf(out, "\"elements\": ");
                json_vector(type->tuple, indent, out, json_VarType);
                fprintf(out, "\n");
                break;
            case PAREN:
                fprintf(out, "\"paren\"\n");
                fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
                fprintf(out, "\"paren_type\": ");
                json_VarType(type->sub_type, indent, out);
                fprintf(out, "\n");
                break;
        }
    }
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

void json_NamedArg(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const NamedType *arg = this;
    if (arg->name != NULL) {
        fprintf(out, "\"name\": ");
        fprintf(out, "\"%s\",\n", arg->name);
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "\"type\": ");
    json_VarType(arg->type, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

void json_FuncType(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    const FuncType *type = this;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"args\": ");
    json_vector(type->named_args, indent, out, json_NamedArg);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"returns\": ");
    json_VarType(type->ret_type, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

void json_string(const void *this, UNUSED int indent, FILE *out) {
    const char *str = this;
    fprintf(out, "\"%s\"", str);
}
