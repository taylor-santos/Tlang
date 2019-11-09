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
    int       int_val;
    double    double_val;
    char      *str_val;
    ASTNode   const *ast;
    Vector    const *vec;
    VarType   *type;
}

%token T_INDENT    "indent"
%token T_OUTDENT   "outdent"
%token T_ERROR     "error"
%token T_NEWLINE   "line break"
%token T_FUNC      "func"
%token T_RETURN    "return"
%token T_REF       "ref"
%token T_HOLD      "!"
%token T_ARROW     "->"
%token T_CLASS     "class"
%token T_DEFINE    ":="
%token END 0       "end of file"
%token<int_val>    T_INT "integer"
%token<double_val> T_DOUBLE "double"
%token<str_val>    T_IDENT "identifier"


%type<ast>  File Return Statement Variable Expression SubExpr Value
            FuncBlock ClassBlock
%type<vec>  Statements ReturnStatements OneLineStatements
            OneLineReturnStatements OptRefExpr ExprChain OptTypes Tuple
            OptArgTypes
            OptNameArgs NameArgs TypesList TypeTuple
%type<type> FuncType OptType Type TypeDef

%start File

%%

File:
    %empty
    {
        *root = new_ProgramNode(&@$, NULL);
    }
  | Statements
    {
        *root = new_ProgramNode(&@$, $1);
    }

Statements:
    Statement T_NEWLINE
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | OneLineStatements Statement T_NEWLINE
    {
        $$ = $1;
        $$->append($$, $2);
    }
  | Statements Statement T_NEWLINE
    {
        $$ = $1;
        $$->append($$, $2);
    }

ReturnStatements:
    Statements
    {
        $$ = $1;
    }
  | Return T_NEWLINE
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | Statements Return T_NEWLINE
    {
        $$ = $1;
        $$->append($$, $2);
    }

OneLineStatements:
    Statement ';'
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | OneLineStatements Statement ';'
    {
        $$ = $1;
        $$->append($$, $2);
    }

OneLineReturnStatements:
    Statement
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | Return
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | OneLineStatements Statement
    {
        $$ = $1;
        $$->append($$, $2);
    }
  | OneLineStatements Return
    {
        $$ = $1;
        $$->append($$, $2);
    }

Return:
    T_RETURN
    {
        $$ = new_ReturnNode(&@$, NULL);
    }
  | T_RETURN Expression
    {
        $$ = new_ReturnNode(&@$, $2);
    }

Statement:
    Expression
    {
        $$ = $1;
    }
  | Variable T_DEFINE Expression
    {
        $$ = new_DefNode(&@$, $1, $3);
    }

Variable:
    T_IDENT
    {
        $$ = new_VariableNode(&@$, $1);
    }
  | T_IDENT ':' Type
    {
        $$ = new_TypedVarNode(&@$, $1, $3);
    }

Expression:
    OptRefExpr
    {
        $$ = new_ExpressionNode(&@$, $1);
    }
  | OptRefExpr T_HOLD
    {
        $1->append($1, new_HoldNode(&@2));
        $$ = new_ExpressionNode(&@$, $1);
    }
  | FuncBlock
    {
        $$ = $1;
    }
  | ClassBlock
    {
        $$ = $1;
    }

OptRefExpr:
    ExprChain
    {
        $$ = $1;
    }
  | T_REF SubExpr
    {
        $$ = new_Vector(0);
        $$->append($$, new_RefNode(&@1, $2));
    }
  | ExprChain T_REF SubExpr
    {
        $$ = $1;
        $$->append($$, new_RefNode(&@2, $3));
    }

ExprChain:
    SubExpr
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
    }
  | ExprChain SubExpr
    {
        $$ = $1;
        $$->append($$, $2);
    }

SubExpr:
    Value
    {
        $$ = $1;
    }
  | '(' Tuple ')'
    {
        $$ = new_TupleNode(&@$, $2);
    }
  | '(' Expression ')'
    {
        $$ = new_ParenNode(&@$, $2);
    }

Value:
    T_IDENT
    {
        $$ = new_VariableNode(&@$, $1);
    }
  | T_INT
    {
        $$ = new_IntNode(&@$, $1);
    }
  | T_DOUBLE
    {
        $$ = new_DoubleNode(&@$, $1);
    }

FuncType:
    T_FUNC OptArgTypes OptType
    {
        new_FuncType($2, $3, &$$);
    }

FuncBlock:
    T_FUNC OptNameArgs OptType T_NEWLINE T_INDENT ReturnStatements T_OUTDENT
    {
        VarType *signature = NULL;
        new_FuncType($2, $3, &signature);
        $$ = new_FunctionNode(&@$, signature, $6);
    }
  | T_FUNC OptNameArgs OptType '{' OneLineReturnStatements '}'
    {
        VarType *signature = NULL;
        new_FuncType($2, $3, &signature);
        $$ = new_FunctionNode(&@$, signature, $5);
    }
  | T_FUNC OptNameArgs OptType T_ARROW Expression
    {
        VarType *signature = NULL;
        new_FuncType($2, $3, &signature);
        const Vector *statements = new_Vector(0);
        const ASTNode *ret = new_ReturnNode(&@5, $5);
        statements->append(statements, ret);
        $$ = new_FunctionNode(&@$, signature, statements);
    }

ClassBlock:
    T_CLASS OptTypes T_NEWLINE T_INDENT Statements T_OUTDENT
    {
        $$ = new_ClassNode(&@$, $2, $5);
    }

OptTypes:
    %empty
    {
        $$ = new_Vector(0);
    }
  | ':' TypesList
    {
        $$ = $2;
    }

Tuple:
    ExprChain ',' ExprChain
    {
        $$ = new_Vector(0);
        $$->append($$, new_ExpressionNode(&@$, $1));
        $$->append($$, new_ExpressionNode(&@$, $3));
    }
  | Tuple ',' ExprChain
    {
        $$ = $1;
        const ASTNode *expr = new_ExpressionNode(&@3, $3);
        $$->append($$, expr);
    }

OptType:
    %empty
    {
        $$ = NULL;
    }
  | ':' Type
    {
        $$ = $2;
    }

Type:
    TypeDef
    {
        $$ = $1;
    }
  | T_REF TypeDef
    {
        new_RefType(&$$, $2);
    }

TypeDef:
    T_IDENT
    {
        new_ObjectType(&$$);
        $$->object->className = $1;
    }
  | FuncType
    {
        $$ = $1;
    }
  | '(' TypeTuple ')'
    {
        new_TupleType(&$$, $2);
    }

OptArgTypes:
    %empty
    {
        $$ = new_Vector(0);
    }
  | '(' ')'
    {
        $$ = new_Vector(0);
    }
  | '(' TypesList ')'
    {
        $$ = $2;
    }

OptNameArgs:
    %empty
    {
        $$ = new_Vector(0);
    }
  | '(' ')'
    {
        $$ = new_Vector(0);
    }
  | '(' NameArgs ')'
    {
        $$ = $2;
    }

NameArgs:
    T_IDENT ':' Type
    {
        $$ = new_Vector(0);
        NamedType *arg = NULL;
        new_NamedType($1, $3, &arg);
        $$->append($$, arg);
    }
  | NameArgs ',' T_IDENT ':' Type
    {
        $$ = $1;
        NamedType *arg = NULL;
        new_NamedType($3, $5, &arg);
        $$->append($$, arg);
    }

TypesList:
    Type
    {
        $$ = new_Vector(0);
        //Function definitions expect args to be named
        NamedType *arg = NULL;
        new_NamedType(NULL, $1, &arg);
        $$->append($$, arg);
    }
  | TypesList ',' Type
    {
        $$ = $1;
        //Function definitions expect args to be named
        NamedType *arg = NULL;
        new_NamedType(NULL, $3, &arg);
        $$->append($$, arg);
    }

TypeTuple:
    Type ',' Type
    {
        $$ = new_Vector(0);
        $$->append($$, $1);
        $$->append($$, $3);
    }
  | TypeTuple ',' Type
    {
        $$ = $1;
        $$->append($$, $3);
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
