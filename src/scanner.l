%{
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "queue.h"
#include "Tlang_parser.h"
#include "safe.h"

const Stack *indent_stack;
const Queue *tok_queue;
YYLTYPE save_loc = { 1, 1, 1, 1 };
int indent = 0;
enum { NOT_SET, SPACES, TABS } indent_type = NOT_SET;

typedef struct token {
    enum yytokentype type;
    union YYSTYPE value;
    YYLTYPE loc;
} Token;

#define YY_USER_ACTION \
    *yylloc = save_loc; \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for (int i = 0; yytext[i] != '\0'; i++) { \
        if (yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 1; \
        } else if (yytext[i] != '\r') { \
            yylloc->last_column++; \
        } \
    } \
    save_loc = *yylloc;
#define YY_USER_INIT { \
    tok_queue    = new_Queue(0); \
    indent_stack = new_Stack(0); \
    int *first_indent = malloc(sizeof(*first_indent)); \
    *first_indent = 0; \
    indent_stack->push(indent_stack, first_indent); \
}

YY_DECL;

void yyerror(YYLTYPE *locp,
    const void *root,
    const char *filename,
    yyscan_t scanner,
    const char *msg);

void push_token(Token t) {
    Token *new_t = malloc(sizeof(*new_t));
    memcpy(new_t, &t, sizeof(*new_t));
    new_t->loc = save_loc;
    safe_method_call(tok_queue, push, new_t);
}

int handle_indentation(int indent_len) {
    int *top_indent = NULL;
    safe_method_call(indent_stack, top, &top_indent);
    if (*top_indent < indent_len) {
        push_token((Token){ .type=T_INDENT });
        int *new_indent = malloc(sizeof(*new_indent));
        *new_indent = indent_len;
        safe_method_call(indent_stack, push, new_indent);
    } else {
        /* Pop the indent stack until the top is less or equal to the
         * current indentation, emitting T_OUTDENT each time. If the current
         * indentation level doesn't match any in the stack, output an
         * indentation error. */
        while (indent_stack->size(indent_stack) && *top_indent > indent_len) {
            safe_method_call(indent_stack, pop, &top_indent);
            free(top_indent);
            push_token((Token){ .type=T_OUTDENT });
            push_token((Token){ .type=T_NEWLINE });
            safe_method_call(indent_stack, top, &top_indent);
        }
        if (*top_indent < indent_len) {
            return 1;
        }
    }
    return 0;
}

int pop_token_queue(YYSTYPE *lval, enum yytokentype *type, YYLTYPE *loc) {
    if (tok_queue->size(tok_queue) != 0) {
        Token *t;
        safe_method_call(tok_queue, pop, &t);
        *type = t->type;
        *lval = t->value;
        *loc  = t->loc;
        free(t);
        /*
        switch(*type) {
            case T_NEWLINE:
                printf("NEWLINE\n");
                return 1;
            case T_INDENT:
                printf("INDENT\n");
                return 1;
            case T_OUTDENT:
                printf("OUTDENT\n");
                return 1;
            case T_FUNC:
                printf("FUNC\n");
                return 1;
            case T_RETURN:
                printf("RETURN\n");
                return 1;
            case T_REF:
                printf("REF\n");
                return 1;
            case T_DEFINE:
                printf("DEFINE\n");
                return 1;
            case T_ARROW:
                printf("ARROW\n");
                return 1;
            case T_CLASS:
                printf("CLASS\n");
                return 1;
            case T_ERROR:
                printf("ERROR\n");
                return 1;
            case T_INT:
                printf("INT     (%d)\n", lval->int_val);
                return 1;
            case T_DOUBLE:
                printf("DOUBLE  (%f)\n", lval->double_val);
                return 1;
            case T_IDENT:
                printf("IDENT   (%s)\n", lval->str_val);
                return 1;
        }
        printf("LITERAL (%c)\n", *type);
         */
        return 1;
    }
    return 0;
}

%}

%option warn nodefault
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge bison-locations
%option noinput
%option yylineno
%option noyywrap

%x normal

%%

%{
    enum yytokentype type;
    if (pop_token_queue(yylval, &type, yylloc)) {
        return type;
    }
%}


^[ \t]*[\n\r]+          // Ignore empty lines
^[ \t]*[/]{2}.*[\n\r]+  // Ignore lines with just comments

^" "+\t {
    yyerror(yylloc, NULL, filename, yyscanner,
        "inconsistent use of tabs and spaces in indentation");
    return T_ERROR;
}

^\t+" " {
    yyerror(yylloc, NULL, filename, yyscanner,
        "inconsistent use of tabs and spaces in indentation");
    return T_ERROR;
}

^" "+ {
    if (indent_type == NOT_SET) {
        indent_type = SPACES;
    } else if (indent_type == TABS) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "inconsistent use of tabs and spaces in indentation");
        return T_ERROR;
    }
    indent += yyleng;
}
^"\t"+ {
    if (indent_type == NOT_SET) {
        indent_type = TABS;
    } else if (indent_type == SPACES) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "inconsistent use of tabs and spaces in indentation");
        return T_ERROR;
    }
    indent += yyleng;
}

<<EOF>> {
    int *top_indent = NULL;
    if (tok_queue->size(tok_queue) > 0) {
        Token *tok;
        safe_method_call(tok_queue, back, &tok);
        if (tok->type != T_NEWLINE) {
            push_token((Token){.type=T_NEWLINE});
        }
    }
    while (indent_stack->size(indent_stack) > 1) {
        safe_method_call(indent_stack, pop, &top_indent);
        free(top_indent);
        push_token((Token){.type=T_OUTDENT});
        push_token((Token){.type=T_NEWLINE});
    }
    enum yytokentype type;
    if (pop_token_queue(yylval, &type, yylloc)) {
        return type;
    }
    return *yytext;
}

. {
    unput(*yytext);
    BEGIN(normal);
    if (handle_indentation(indent)) {
        indent = 0;
        yyerror(yylloc, NULL, filename, yyscanner,
            "indentation does not match any outer indentation level");
        return T_ERROR;
    }
    indent = 0;
}

<normal>{
    [/]{2}.*                // Ignore comments
    [\\][ \t]*[\n\r]+[ \t]* // Ignore escaped newlines

    func   { push_token((Token){.type=T_FUNC}); }
    return { push_token((Token){.type=T_RETURN}); }
    ref    { push_token((Token){.type=T_REF}); }
    class  { push_token((Token){.type=T_CLASS}); }
    ->     { push_token((Token){.type=T_ARROW}); }
    :=     { push_token((Token){.type=T_DEFINE}); }
    [=:\(\),\[\]] {
        push_token((Token){.type=*yytext});
    }

    [0-9]+ {
        push_token((Token){.type=T_INT,    .value.int_val=atoi(yytext)});
    }
    [0-9]+"."[0-9]* {
        push_token((Token){.type=T_DOUBLE, .value.double_val=atof(yytext)});
    }
    [a-zA-Z_][a-zA-Z0-9_]* {
        char *val = NULL;
        asprintf(&val, "var_%s", yytext);
        push_token((Token){.type=T_IDENT,  .value.str_val=val});
    }

    [\n\r]+ {
        BEGIN(INITIAL);
        push_token((Token){.type=T_NEWLINE});
        indent = 0;
        enum yytokentype type;
        if (pop_token_queue(yylval, &type, yylloc)) {
            return type;
        }
    }

    ; {
        push_token((Token){.type=T_NEWLINE});
        enum yytokentype type;
        if (pop_token_queue(yylval, &type, yylloc)) {
            return type;
        }
    }

    [ \t]+ // Ignore non-leading whitespace
    [\~\!\%\^\*\/\+\-\=]+ {
        char *val;
        asprintf(&val, "0x%X", yytext[0]);
        for (int i = 1; yytext[i] != '\0'; i++) {
            append_string(&val, "%X", yytext[i]);
        }
        push_token((Token){.type=T_IDENT, .value.str_val=val});
    }
    . {
        char *val = NULL;
        safe_asprintf(&val, "0x%X", (int)*yytext);
        push_token((Token){.type=T_IDENT, .value.str_val=val});
    }
}

%%
