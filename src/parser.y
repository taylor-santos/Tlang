%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h> // strdup()
    #include "Tlang_parser.h"
    #include "Tlang_scanner.h"

    #define YYERROR_VERBOSE
    #define RED     "\033[0;91m"
    #define WHITE   "\033[0m"
    #define ERROR   RED "error: " WHITE

    void yyerror(YYLTYPE *locp,
                 const ASTNode **root,
                 const char *filename,
                 yyscan_t scanner,
                 const char *msg);
%}

%code provides {
    #define YY_DECL int yylex (YYSTYPE *yylval_param, \
        YYLTYPE *yylloc_param, \
        const char *filename, \
        yyscan_t yyscanner)
    YY_DECL;
}

%code requires {
    #include "ast.h"
    #include "vector.h"
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
    #define UNUSED __attribute__ ((unused))
}

%define api.pure full
%locations
%parse-param { const ASTNode **root }
%param { const char *filename } { yyscan_t scanner }

%union {
    int     int_val;
    double  double_val;
    char    *str_val;
    ASTNode const *ast;
    Vector  const *vec;
}

%token             INDENT OUTDENT ERROR NEWLINE
%token<int_val>    INT_LIT
%token<double_val> DOUBLE_LIT
%token<str_val>    IDENT

%type<ast> file statement
%type<vec> stmts

%start file

%%

file:
    %empty
        {
            *root = new_ProgramNode(&@$, 0);
        }
  | stmts
        {
            *root = new_ProgramNode(&@$, $1);
        }

stmts:
    statement
        {
            $$ = new_Vector(0);
            $$->append($$, $1);
        }
  | stmts statement
        {
            $$ = $1;
            $$->append($$, $2);
        }

statement:
    IDENT '=' expr NEWLINE
        {
            $$ = new_LeafNode(&@$);
            free($1);
        }

expr:
    IDENT
        {
            free($1);
        }
  | INT_LIT
  | DOUBLE_LIT

%%

void yyerror(YYLTYPE *locp,
    UNUSED const ASTNode **root,
    const char *filename,
    UNUSED yyscan_t scanner,
    const char *msg
) {
    fprintf(stderr,
        "%s:%d:%d: " ERROR "%s\n",
        filename,
        locp->first_line,
        locp->first_column,
        msg
    );
}
