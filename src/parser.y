%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h> // strdup()
    #include "Tlang_parser.h"
    #include "Tlang_scanner.h"
    #include "safe.h"

    #define YYERROR_VERBOSE
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
}

%define api.pure full
%locations
%parse-param { const ASTNode **root }
%param { const char *filename } { yyscan_t scanner }

%union {
    int      int_val;
    double   double_val;
    char     *str_val;
    ASTNode  const *ast;
    Vector   const *vec;
    VarType  *var_type;
    NamedArg *named_arg;
    FuncType *func_type;
}

%token             INDENT OUTDENT ERROR NEWLINE FUNC RETURN
%token<int_val>    INT_LIT
%token<double_val> DOUBLE_LIT
%token<str_val>    IDENT

%type<ast>       file statement assignment lvalue expr
%type<vec>       stmts opt_args args opt_named_args named_args
%type<var_type>  type_def
%type<func_type> func_def named_func_def
%type<named_arg> named_type_def

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
            safe_method_call($$, append, $1);
        }
  | stmts statement
        {
            $$ = $1;
            safe_method_call($$, append, $2);
        }

statement:
    assignment NEWLINE
        {
            $$ = $1;
        }
  | RETURN expr NEWLINE
        {
            $$ = new_ReturnNode(&@$, $2);
        }
  | RETURN NEWLINE
        {
            $$ = new_ReturnNode(&@$, NULL);
        }

assignment:
    expr
        {
            $$ = $1;
        }
  | lvalue '=' assignment
        {
            $$ = new_AssignmentNode(&@$, $1, $3);
        }

lvalue:
    IDENT
        {
            $$ = new_VariableNode(&@$, $1);
        }
  | IDENT ':' type_def
        {
            $$ = new_TypedVarNode(&@$, $1, $3);
        }

type_def:
    IDENT
        {
            safe_function_call(new_VarType, $1, &$$);
            free($1);
        }
  | func_def
        {
            safe_function_call(new_VarType_from_FuncType, $1, &$$);
        }
        
named_func_def:
    FUNC '(' opt_named_args ')' ':' type_def
        {
            safe_function_call(new_FuncType, $3, $6, &$$);
        }
  | FUNC '(' opt_named_args ')'
        {
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, $3, none, &$$);
        }
        
func_def:
    FUNC '(' opt_args ')' ':' type_def
        {
            safe_function_call(new_FuncType, $3, $6, &$$);
        }
  | FUNC '(' opt_args ')'
        {
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, $3, none, &$$);
        }

expr:
    lvalue
        {
            $$ = $1;
        }
  | INT_LIT
        {
            $$ = new_IntNode(&@$, $1);
        }
  | DOUBLE_LIT
        {
            $$ = new_DoubleNode(&@$, $1);
        }
  | named_func_def NEWLINE INDENT stmts OUTDENT
        {
            $$ = new_FunctionNode(&@$, $1, $4);
        }

opt_named_args:
    %empty
        {
            $$ = new_Vector(0);
        }
  | named_args
        {
            $$ = $1;
        }

named_args:
    named_type_def
        {
            $$ = new_Vector(0);
            safe_method_call($$, append, $1);
        }
  | named_args ',' named_type_def
        {
            $$ = $1;
            safe_method_call($$, append, $3);
        }

named_type_def:
    IDENT ':' type_def
        {
            safe_function_call(new_NamedArg, $1, $3, &$$);
        }

opt_args:
    %empty
        {
            $$ = new_Vector(0);
        }
  | args
        {
            $$ = $1;
        }

args:
    type_def
        {
            $$ = new_Vector(0);
            NamedArg *arg = NULL;
            safe_function_call(new_NamedArg, NULL, $1, &arg);
            safe_method_call($$, append, arg);
        }
  | named_type_def
        {
            $$ = new_Vector(0);
            safe_method_call($$, append, $1);
        }
  | args ',' type_def
        {
            $$ = $1;
            NamedArg *arg = NULL;
            safe_function_call(new_NamedArg, NULL, $3, &arg);
            safe_method_call($$, append, arg);
        }
  | args ',' named_type_def
          {
              $$ = $1;
              safe_method_call($$, append, $3);
          }

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
