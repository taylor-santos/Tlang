%{
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "Tlang_parser.h"

const Stack *indent;

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for (int i = 0; yytext[i] != '\0'; i++) { \
        if (yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 1; \
        } else { \
            yylloc->last_column++; \
        } \
    }
#define YY_USER_INIT { \
    indent = new_Stack(1); \
    indent->push(indent, 0); \
}
YY_DECL;

void yyerror(YYLTYPE *locp,
    const char *filename,
    yyscan_t scanner,
    const char *msg);

%}

%option warn nodefault
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge bison-locations
%option noinput
%option nounput
%option yylineno
%option noyywrap

%x COMMENT

%%

"//".*
"/*" {
    BEGIN(COMMENT);
}
<COMMENT>{
    "*/"    { BEGIN(INITIAL); }
    [^*\n]+
    "*"
    \n
    <<EOF>> {
        yyerror(yylloc, filename, NULL, "error: unterminated comment");
        return 0;
    }
}

[0-9]+ {
    yylval->int_val = atoi(yytext);
    return INT_LIT;
}
[0-9]+"."[0-9]+ {
    yylval->double_val = atof(yytext);
    return DOUBLE_LIT;
}
[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->str_val = strdup(yytext);
    return IDENT;
}

[ \t]+
"\n"

. {
    return *yytext;
}

%%
