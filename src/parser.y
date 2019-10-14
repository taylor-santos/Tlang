%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h> // strdup()
    #include <stdarg.h>
    #include "Tlang_parser.h"
    #include "Tlang_scanner.h"
    #include "safe.h"

    #define YY_ERROR_VERBOSE
    // Uncomment the following line to enable Bison debug tracing
    // int yydebug = 1;
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
    #ifndef YY_TYPET_REF_YY_SCANNER_T
    #define YY_TYPET_REF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
}

%define api.pure full
%define parse.error verbose
%define parse.lac full
%define parse.trace
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

%token             T_INDENT T_OUTDENT T_ERROR T_NEWLINE T_FUNC T_RETURN T_REF
                   T_ARROW T_CLASS
%token<int_val>    T_INT
%token<double_val> T_DOUBLE
%token<str_val>    T_IDENT

%type<ast>       file statement l_value r_expr sub_expr expression
%type<vec>       stmts opt_args args opt_named_args named_args call_list
                 opt_inheritance inheritance opt_stmts
%type<var_type>  type_def opt_return_type
%type<func_type> func_def named_func_def
%type<named_arg> named_type_def opt_extension opt_named_extension

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
    T_IDENT ':' type_def T_NEWLINE
        {
            $$ = new_TypedVarNode(&@$, $1, $3);
        }
  | expression
        {
            $$ = $1;
        }
  | T_RETURN expression
        {
            $$ = new_ReturnNode(&@$, $2);
        }
  | T_RETURN T_NEWLINE
        {
            $$ = new_ReturnNode(&@$, NULL);
        }

expression:
    call_list T_NEWLINE
        {
            $$ = new_ExpressionNode(&@$, $1);
        }
  | opt_named_extension named_func_def opt_stmts
        {
            $2->extension = $1;
            $$ = new_FunctionNode(&@$, $2, $3);
        }
  | T_CLASS opt_inheritance opt_stmts
        {
            $$ = new_ClassNode(&@$, $2, $3);
        }
  | l_value '=' expression
        {
            $$ = new_AssignmentNode(&@$, $1, $3);
        }

opt_stmts:
    T_NEWLINE
        {
            $$ = new_Vector(0);
        }
  | T_NEWLINE T_INDENT stmts T_OUTDENT T_NEWLINE
        {
            $$ = $3;
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
  | T_REF sub_expr
        {
            $$ = new_RefNode(&@$, $2);
        }

l_value:
    T_IDENT
        {
            $$ = new_VariableNode(&@$, $1);
        }

type_def:
    T_IDENT
        {
            safe_function_call(new_VarType, $1, &$$);
            free($1);
        }
  | T_CLASS
        {
            safe_function_call(new_ClassType, &$$);
        }
  | func_def
        {
            safe_function_call(new_VarType_from_FuncType, $1, &$$);
        }
        
named_func_def:
    T_FUNC opt_named_args opt_return_type
        {
            safe_function_call(new_FuncType, $2, $3, &$$);
        }

func_def:
    opt_extension T_FUNC opt_args opt_return_type
        {
            safe_function_call(new_FuncType, $3, $4, &$$);
            $$->extension = $1;
        }

opt_extension:
    %empty
        {
            $$ = NULL;
        }
  | '(' type_def ')'
        {
            safe_function_call(new_NamedArg, NULL, $2, &$$);
        }

opt_args:
    %empty
        {
            $$ = new_Vector(0);
        }
  | '(' ')'
        {
            $$ = new_Vector(0);
        }
  | '(' args ')'
        {
            $$ = $2;
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

opt_return_type:
    %empty
        {
            safe_function_call(new_NoneType, &$$);
        }
  | T_ARROW type_def
        {
            $$ = $2;
        }

r_expr:
    l_value
        {
            $$ = $1;
        }
  | T_INT
        {
            $$ = new_IntNode(&@$, $1);
        }
  | T_DOUBLE
        {
            $$ = new_DoubleNode(&@$, $1);
        }

opt_named_extension:
    %empty
        {
            $$ = NULL;
        }
  | '(' T_IDENT ':' type_def ')'
        {
            safe_function_call(new_NamedArg, $2, $4, &$$);
        }

opt_named_args:
    %empty
        {
            $$ = new_Vector(0);
        }
  | '(' ')'
        {
            $$ = new_Vector(0);
        }
  | '(' named_args ')'
        {
            $$ = $2;
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
    T_IDENT ':' type_def
        {
            safe_function_call(new_NamedArg, $1, $3, &$$);
        }

opt_inheritance:
    %empty
        {
            $$ = new_Vector(0);
        }
  | ':' inheritance
        {
            $$ = $2;
        }

inheritance:
    T_IDENT
        {
            $$ = new_Vector(0);
            safe_method_call($$, append, $1);
        }
  | inheritance ',' T_IDENT
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
