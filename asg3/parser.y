%{
// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: parser.y,v 1.1 2015-03-31 12:37:36-07 - - $

#include "lyutils.h"
#include "astree.h"
#include "assert.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%expect 35
%destructor { error_destructor ($$); } <>

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_WHILE TOK_RETURN TOK_STRUCT TOK_ARRAY
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_IDENT
%token TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_ROOT TOK_DECLID TOK_TYPEID TOK_NEWARRAY
%token TOK_NEWSTRING TOK_IFELSE TOK_RETURNVOID
%token TOK_BLOCK TOK_VARDECL TOK_FUNCTION
%token TOK_PARAMLIST TOK_PROTOTYPE

%right TOK_IF TOK_ELSE
%right '='
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left '+' '-'
%left '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_ORD TOK_CHR
%left TOK_INDEX TOK_FIELD TOK_CALL
%nonassoc TOK_NEW
%nonassoc TOK_PAREN

%start start

%%

start     : program                       { $$ = yyparse_astree = $1; }
          ;

program   : program structdef             { $$ = adopt1 ($1, $2); }
          | program function              { $$ = adopt1 ($1, $2); }
          | program statement             { $$ = adopt1 ($1, $2); }
          | program error '}'             { free_ast ($3); $$ = $1; }
          | program error ';'             { free_ast ($3); $$ = $1; }
          |                               { $$ = new_parseroot (); }
          ;

structdef : W1 '}'                        { free_ast ($2); $$ = $1; }
          ;

W1        : TOK_STRUCT TOK_IDENT '{'      { free_ast ($3);
                                            change_sym ($2, TOK_TYPEID);
                                            $$ = adopt1 ($1, $2); }
          | W1 fielddecl ';'              { free_ast ($3);
                                            $$ = adopt1 ($1, $2); }
          ;

fielddecl : basetype TOK_IDENT            { change_sym ($2, TOK_FIELD);
                                            $$ = adopt1 ($1, $2); }
          | basetype TOK_ARRAY TOK_IDENT  { change_sym ($3, TOK_FIELD);
                                            $$ = adopt2 ($2, $1, $3); }
          ;

basetype  : TOK_VOID                      { $$ = $1; }
          | TOK_BOOL                      { $$ = $1; }
          | TOK_CHAR                      { $$ = $1; }
          | TOK_INT                       { $$ = $1; }
          | TOK_STRING                    { $$ = $1; }
          | TOK_IDENT                     { $$ = change_sym 
                                            ($1, TOK_TYPEID); }
          ;

function  : identdecl X1 ')' block        { free_ast ($3);
                                            $$ = adopt3fn 
                                            ($1, $2, $4); }
          ;

X1        : '('                           { $$ = change_sym 
                                            ($1, TOK_PARAMLIST); }
          | X2 identdecl                  { $$ = adopt1 ($1, $2); }
          | X3 ',' identdecl              { free_ast ($2);
                                            $$ = adopt1 ($1, $3); }
          ;

X2        : '('                           { $$ = change_sym 
                                            ($1, TOK_PARAMLIST); }
          ;

X3        : X2 identdecl                  { $$ = adopt1 ($1, $2); }
          | X3 ',' identdecl              { free_ast ($2);
                                            $$ = adopt1 ($1, $3); }
          ;

identdecl : basetype TOK_IDENT            { change_sym ($2, TOK_DECLID);
                                            $$ = adopt1 ($1, $2); }
          | basetype TOK_ARRAY TOK_IDENT  { change_sym ($3, TOK_DECLID);
                                            $$ = adopt2 ($2, $1, $3); }
          ;

block     : Y1 '}'                        { free_ast ($2); $$ = $1; }
          | ';'                           { $$ = $1; }
          ;

Y1        : '{'                           { $$ = change_sym 
                                            ($1, TOK_BLOCK); }
          | Y1 statement                  { $$ = adopt1 ($1, $2); }
          ;

statement : block                         { $$ = $1; }
          | vardecl                       { $$ = $1; }
          | while                         { $$ = $1; }
          | ifelse                        { $$ = $1; }
          | return                        { $$ = $1; }
          | expr ';'                      { free_ast ($2); $$ = $1; }
          ;

vardecl   : identdecl '=' expr ';'        { free_ast ($4);
                                            change_sym 
                                            ($2, TOK_VARDECL);
                                            $$ = adopt2 ($2, $1, $3); }
          ;

while     : TOK_WHILE '(' expr ')' statement 
                                          { free_ast2 ($2, $4);
                                            $$ = adopt2 ($1, $3, $5); }
          ;

ifelse    : TOK_IF '(' expr ')' statement { free_ast2 ($2, $4);
                                            $$ = adopt2 ($1, $3, $5); }
          | TOK_IF '(' expr ')' statement 
            TOK_ELSE statement            { free_ast2 ($2, $4);
                                            free_ast ($6);
                                            change_sym ($1, TOK_IFELSE);
                                            adopt2 ($1, $3, $5);
                                            $$ = adopt1 ($1, $7); }
          ;

return    : TOK_RETURN expr ';'           { free_ast ($3);
                                            $$ = adopt1 ($1, $2); }
          | TOK_RETURN ';'                { free_ast ($2);
                                            $$ = change_sym 
                                            ($1, TOK_RETURNVOID); }
          ;

expr      : expr '+' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr '-' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr '*' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr '/' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr '%' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr '=' expr                 { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_EQ expr              { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_NE expr              { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LT expr              { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LE expr              { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GT expr              { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GE expr              { $$ = adopt2 ($2, $1, $3); }
          | '+' expr %prec TOK_POS        { $$ = adopt1sym 
                                            ($1, $2, TOK_POS); }
          | '-' expr %prec TOK_NEG        { $$ = adopt1sym 
                                            ($1, $2, TOK_NEG); }
          | '!' expr                      { $$ = adopt1 ($1, $2); }
          | TOK_ORD expr                  { $$ = adopt1 ($1, $2); }
          | TOK_CHR expr                  { $$ = adopt1 ($1, $2); }
          | allocator                     { $$ = $1; }
          | call                          { $$ = $1; }
          | '(' expr ')' %prec TOK_PAREN  { free_ast2 ($1, $3);
                                            $$ = $2; }
          | variable                      { $$ = $1; }
          | constant                      { $$ = $1; }
          ;

allocator : TOK_NEW TOK_IDENT '(' ')'     { free_ast2 ($3, $4);
                                            change_sym ($2, TOK_TYPEID);
                                            $$ = adopt1 ($1, $2); }
          | TOK_NEW TOK_STRING '(' expr ')' 
                                          { free_ast2 ($2, $3);
                                            free_ast ($5);
                                            $$ = adopt1sym 
                                            ($1, $4, TOK_NEWSTRING); }
          | TOK_NEW basetype '[' expr ']' { free_ast2 ($3, $5);
                                            change_sym 
                                            ($1, TOK_NEWARRAY);
                                            $$ = adopt2 ($1, $2, $4); }
          ;

call      : Z1 ')'                        { free_ast ($2); $$ = $1; }
          ;

Z1        : TOK_IDENT '(' %prec TOK_CALL  { $$ = adopt1sym 
                                            ($2, $1, TOK_CALL); }
          | Z2 expr                       { $$ = adopt1 ($1, $2); }
          | Z3 ',' expr                   { free_ast ($2);
                                            $$ = adopt1 ($1, $3); }
          ;

Z2        : TOK_IDENT '(' %prec TOK_CALL  { $$ = adopt1sym 
                                            ($2, $1, TOK_CALL); }
          ;

Z3        : Z2 expr                       { $$ = adopt1 ($1, $2); }
          | Z3 ',' expr                   { free_ast ($2);
                                            $$ = adopt1 ($1, $3); }
          ;

variable  : TOK_IDENT                     { $$ = $1; }
          | expr '[' expr ']' %prec TOK_INDEX 
                                          { free_ast ($4);
                                            change_sym ($2, TOK_INDEX);
                                            $$ = adopt2 ($2, $1, $3); }
          | expr '.' TOK_IDENT %prec TOK_FIELD 
                                          { change_sym ($3, TOK_FIELD);
                                            $$ = adopt2 ($2, $1, $3); }
          ;

constant  : TOK_INTCON                    { $$ = $1; }
          | TOK_CHARCON                   { $$ = $1; }
          | TOK_STRINGCON                 { $$ = $1; }
          | TOK_FALSE                     { $$ = $1; }
          | TOK_TRUE                      { $$ = $1; }
          | TOK_NULL                      { $$ = $1; }
          ;

%%

const char *get_yytname (int symbol) {
    return yytname [YYTRANSLATE (symbol)];
}

bool is_defined_token (int symbol) {
    return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*static void* yycalloc (size_t size) {
    void* result = calloc (1, size);
    assert (result != NULL);
    return result;
}*/
