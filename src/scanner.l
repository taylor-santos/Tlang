%{
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "queue.h"
#include "Tlang_parser.h"

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
        } else { \
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
#define RED     "\033[0;91m"
#define WHITE   "\033[0m"
// Call the member function pointer 'fn' of struct* 'obj' with any given
// arguments. If the function returns nonzero, print an internal compiler error
// and exit the process.
#define safe_call(obj, fn, ...) { \
    if (obj->fn(obj, __VA_ARGS__)) { \
        fprintf(stderr, \
                "%s:%d: " RED "internal compiler error: " WHITE #obj "." #fn \
                "(" #__VA_ARGS__ ") failed\n", \
                __FILE__, \
                __LINE__); \
        exit(EXIT_FAILURE); \
    } \
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
    safe_call(tok_queue, push, new_t);
}

int handle_indentation(int indent_len) {
    int *top_indent = NULL;
    safe_call(indent_stack, top, &top_indent);
    if (*top_indent < indent_len) {
        push_token((Token){ .type=INDENT });
        int *new_indent = malloc(sizeof(*new_indent));
        *new_indent = indent_len;
        safe_call(indent_stack, push, new_indent);
    } else {
        /* Pop the indent stack until the top is less or equal to the
         * current indentation, emitting OUTDENT each time. If the current
         * indentation level doesn't match any in the stack, output an
         * indentation error. */
        while (indent_stack->size(indent_stack) && *top_indent > indent_len) {
            safe_call(indent_stack, pop, &top_indent);
            free(top_indent);
            push_token((Token){ .type=OUTDENT });
            safe_call(indent_stack, top, &top_indent);
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
        safe_call(tok_queue, pop, &t);
        *type = t->type;
        *lval = t->value;
        *loc  = t->loc;
        free(t);
        switch(*type) {
            case NEWLINE:
                printf("NEWLINE\n");
                break;
            case INDENT:
                printf("INDENT\n");
                break;
            case OUTDENT:
                printf("OUTDENT\n");
                break;
            case ERROR:
                printf("ERROR\n");
                break;
            case INT_LIT:
                printf("INT_LIT    (%d)\n", lval->int_val);
                break;
            case DOUBLE_LIT:
                printf("DOUBLE_LIT (%f)\n", lval->double_val);
                break;
            case IDENT:
                printf("IDENT      (%s)\n", lval->str_val);
                break;
            default:
                printf("LITERAL    (%c)\n", *type);
        }
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

%%

%{
    enum yytokentype type;
    if (pop_token_queue(yylval, &type, yylloc)) {
        return type;
    }
%}

[/]{2}.*                // Ignore comments
^[ \t]+[\n\r]+          // Ignore empty lines
^[ \t]+[/]{2}.+[\n\r]+  // Ignore lines with just comments
[\\][ \t]*[\n\r]+[ \t]* // Ignore escaped newlines

[0-9]+ {
    push_token((Token){.type=INT_LIT,    .value.int_val=atoi(yytext)});
}
[0-9]+"."[0-9]+ {
    push_token((Token){.type=DOUBLE_LIT, .value.double_val=atof(yytext)});
}
[a-zA-Z_][a-zA-Z0-9_]* {
    push_token((Token){.type=IDENT,      .value.str_val=strdup(yytext)});
}

"\n" {
    push_token((Token){.type=NEWLINE});
    indent = 0;
    enum yytokentype type;
    if (pop_token_queue(yylval, &type, yylloc)) {
        return type;
    }
}

^" "+ {
    if (indent_type == NOT_SET) {
        indent_type = SPACES;
    } else if (indent_type == TABS) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "inconsistent use of tabs and spaces in indentation");
        return ERROR;
    }
    if (handle_indentation(yyleng)) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "indentation does not match any outer indentation level");
        return ERROR;
    }
}
^"\t"+ {
    if (indent_type == NOT_SET) {
        indent_type = TABS;
    } else if (indent_type == SPACES) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "inconsistent use of tabs and spaces in indentation");
        return ERROR;
    }
    if (handle_indentation(yyleng)) {
        yyerror(yylloc, NULL, filename, yyscanner,
            "indentation does not match any outer indentation level");
        return ERROR;
    }
}

[ \t]+                  // Ignore non-leading whitespace

. {
    push_token((Token){.type=*yytext});
}

<<EOF>> {
    int *top_indent = NULL;
    while (indent_stack->size(indent_stack) > 1) {
        safe_call(indent_stack, pop, &top_indent);
        free(top_indent);
        push_token((Token){.type=OUTDENT});
    }
    enum yytokentype type;
    if (pop_token_queue(yylval, &type, yylloc)) {
        return type;
    }
    if (tok_queue->size(tok_queue)) {
        unput(*yytext);
    } else {
        return *yytext;
    }
}


%%
