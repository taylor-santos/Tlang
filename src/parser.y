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
    #ifndef YY_TYPEREF_YY_SCANNER_T
    #define YY_TYPEREF_YY_SCANNER_T
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

%token             INDENT OUTDENT ERROR NEWLINE FUNC RETURN REF ARROW CALL
%token<int_val>    INT_LIT
%token<double_val> DOUBLE_LIT
%token<str_val>    IDENT

%type<ast>       file statement l_value r_expr sub_expr expression
%type<vec>       stmts opt_args args opt_named_args named_args call_list
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
    IDENT ';' type_def NEWLINE
        {
            $$ = new_TypedVarNode(&@$, $1, $3);
        }
  | expression NEWLINE
        {
            $$ = $1;
        }
  | RETURN expression NEWLINE
        {
            $$ = new_ReturnNode(&@$, $2);
        }
  | RETURN NEWLINE
        {
            $$ = new_ReturnNode(&@$, NULL);
        }

expression:
    call_list
        {
            $$ = new_ExpressionNode(&@$, $1);
        }
  | l_value '=' expression
        {
            $$ = new_AssignmentNode(&@$, $1, $3);
        }

call_list:
    sub_expr
        {
            $$ = new_Vector(0);
            safe_method_call($$, append, $1);
        }
  | call_list sub_expr
        {
            $$ = $1;
            safe_method_call($$, append, $2);
        }

sub_expr:
    r_expr
        {
            $$ = $1;
        }
  | '(' call_list ')'
        {
            $$ = new_ParenNode(&@$, new_ExpressionNode(&@$, $2));
        }
  | CALL
        {
            $$ = new_CallNode(&@$);
        }
  | REF
        {
            $$ = new_RefNode(&@$);
        }


l_value:
    IDENT
        {
            $$ = new_VariableNode(&@$, $1);
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
    FUNC '(' opt_named_args ')' ARROW type_def
        {
            safe_function_call(new_FuncType, $3, $6, &$$);
        }
  | FUNC '(' opt_named_args ')'
        {
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, $3, none, &$$);
        }
  | FUNC ARROW type_def
        {
            const Vector *args = new_Vector(0);
            safe_function_call(new_FuncType, args, $3, &$$);
        }
  | FUNC
        {
            const Vector *args = new_Vector(0);
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, args, none, &$$);
        }

func_def:
    FUNC '(' opt_args ')' ARROW type_def
        {
            safe_function_call(new_FuncType, $3, $6, &$$);
        }
  | FUNC '(' opt_args ')'
        {
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, $3, none, &$$);
        }
  | FUNC ARROW type_def
        {
            const Vector *args = new_Vector(0);
            safe_function_call(new_FuncType, args, $3, &$$);
        }
  | FUNC
        {
            const Vector *args = new_Vector(0);
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, args, none, &$$);
        }
  | '(' type_def ')' FUNC '(' opt_args ')' ARROW type_def
        {
            safe_function_call(new_FuncType, $6, $9, &$$);
            safe_function_call(new_NamedArg, NULL, $2, &$$->extension);
        }
  | '(' type_def ')' FUNC '(' opt_args ')'
        {
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, $6, none, &$$);
            safe_function_call(new_NamedArg, NULL, $2, &$$->extension);
        }
  | '(' type_def ')' FUNC ARROW type_def
        {
            const Vector *args = new_Vector(0);
            safe_function_call(new_FuncType, args, $6, &$$);
            safe_function_call(new_NamedArg, NULL, $2, &$$->extension);
        }
  | '(' type_def ')' FUNC
        {
            const Vector *args = new_Vector(0);
            VarType *none = NULL;
            safe_function_call(new_NoneType, &none);
            safe_function_call(new_FuncType, args, none, &$$);
            safe_function_call(new_NamedArg, NULL, $2, &$$->extension);
        }

r_expr:
    l_value
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
  | '(' IDENT ':' type_def ')' named_func_def NEWLINE INDENT stmts OUTDENT
        {
            safe_function_call(new_NamedArg, $2, $4, &$6->extension);
            $$ = new_FunctionNode(&@$, $6, $9);
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
