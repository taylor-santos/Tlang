%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h> // strdup()
    #include "Tlang_parser.h"
    #include "Tlang_scanner.h"

    #define YYERROR_VERBOSE

    void yyerror(YYLTYPE *locp,
                 const char *filename,
                 yyscan_t scanner,
                 const char *msg);
    int yywrap();
%}

%code provides {
    #define YY_DECL int yylex (YYSTYPE *yylval_param, \
        YYLTYPE *yylloc_param, \
        const char *filename, \
        yyscan_t yyscanner)
    YY_DECL;
}

%code requires {
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
    #define UNUSED __attribute__ ((unused))
}

%define api.pure full
%locations
%param { const char *filename } { yyscan_t scanner }

%union {
    int int_val;
    double double_val;
    char *str_val;
}

%token<int_val>    INT_LIT
%token<double_val> DOUBLE_LIT
%token<str_val>    IDENT STR_LIT CHAR_LIT

%start file

%%

file:
    %empty

%%

void yyerror(YYLTYPE *locp,
    const char *filename,
    UNUSED yyscan_t scanner,
    const char *msg
) {
    fprintf(stderr,
        "%s:%d:%d: %s\n",
        filename,
        locp->first_line,
        locp->first_column,
        msg
    );
}
