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

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_ORD TOK_CHR 

%token TOK_ROOT TOK_DECLID TOK_TYPEID TOK_FIELD TOK_INDEX
%token TOK_POS TOK_NEG TOK_CALL TOK_NEWARRAY TOK_NEWSTRING
%token TOK_IFELSE TOK_RETURNVOID TOK_BLOCK TOK_VARDECL
%token TOK_FUNCTION TOK_PARAMLIST TOK_PROTOTYPE

%right TOK_IF TOK_ELSE
%right '='
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left '+' '-'
%left '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_ORD TOK_CHR
%left TOK_ARRAY TOK_FIELD TOK_FUNCTION
%nonassoc TOK_NEW

%start start

%%

start     : program                             { yyparse_astree = $1; }
          ;

program   : program structdef                   { $$ = adopt1 ($1, $2); }
          | program function                    { $$ = adopt1 ($1, $2); }
          | program statement                   { $$ = adopt1 ($1, $2); }
          | program error '}'                   { $$ = $1; }
          | program error ';'                   { $$ = $1; }
          |                                     { $$ = new_parseroot (); }
          ;

structdef : TOK_STRUCT TOK_IDENT '{' fields '}' { $$ = adopt2syml ($1, $2, $4, TOK_TYPEID); }
          ;

fields    : fields fielddecl ';'                { $$ = adopt1 ($1, $2); }
          |
          ;

fielddecl : basetype TOK_IDENT                  { $$ = adopt1syml ($1, $2, TOK_FIELD); }
          | basetype TOK_ARRAY TOK_IDENT        { $$ = adopt2symr ($2, $1, $3, TOK_FIELD); }
          ;


basetype  : TOK_VOID                            { $$ = $1; }
          | TOK_BOOL                            { $$ = $1; }
          | TOK_CHAR                            { $$ = $1; }
          | TOK_INT                             { $$ = $1; }
          | TOK_STRING                          { $$ = $1; }
          | TOK_IDENT                           { $$ = change_sym ($1, TOK_TYPEID); }
          ;

function  : identdecl args ')' block            { $$ = adopt3 ($1, $2, $4); }
          ;

args      : args ',' identdecl                  { $$ = adopt1 ($1, $3); }
          | '(' identdecl                       { $$ = adopt1sym ($1, $2, TOK_PARAMLIST); }
          | '('                                 { $$ = change_sym ($1, TOK_PARAMLIST); }
          ;

identdecl : basetype TOK_IDENT                  { $$ = adopt1syml ($1, $2, TOK_DECLID); }
          | basetype TOK_ARRAY TOK_IDENT        { $$ = adopt2symr ($2, $1, $3, TOK_DECLID); }
          ;

block     : states '}'                          { $$ = $1; }
          | ';'                                 { $$ = $1; }
          ;

states    : states statement                    { $$ = adopt1 ($1, $2); }
          | '{'                                 { $$ = change_sym ($1, TOK_BLOCK); }
          ;

statement : block                               { $$ = $1; }
          | vardecl                             { $$ = $1; }
          | while                               { $$ = $1; }
          | ifelse                              { $$ = $1; }
          | return                              { $$ = $1; }
          | expr ';'                            { $$ = $1; }
          ;

vardecl   : identdecl '=' expr ';'              { $$ = adopt2sym ($2, $1, $3, TOK_VARDECL); }
          ;

while     : TOK_WHILE '(' expr ')' statement    { $$ = adopt2 ($1, $3, $5); }
          ;

ifelse    : TOK_IF '(' expr ')' statement       { $$ = adopt2 ($1, $3, $5); }
          | TOK_IF '(' expr ')' statement TOK_ELSE statement 
                                                { $$ = adopt2sym ($1, $3, $5, TOK_IFELSE); 
                                                adopt1 ($1, $7); }
          ;

return    : TOK_RETURN expr ';'                 { $$ = adopt1 ($1, $2); }
          | TOK_RETURN ';'                      { $$ = change_sym ($1, TOK_RETURNVOID); }
		  ;

expr      : binop                               { $$ = $1; }
          | unop                                { $$ = $1; }
          | allocator                           { $$ = $1; }
          | call                                { $$ = $1; }
          | '(' expr ')'                        { $$ = $2; }
          | variable                            { $$ = $1; }
          | constant                            { $$ = $1; }
          ;

binop     : expr '+' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr '-' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr '*' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr '/' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr '%' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr '=' expr                      { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_EQ expr                   { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_NE expr                   { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LT expr                   { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_LE expr                   { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GT expr                   { $$ = adopt2 ($2, $1, $3); }
          | expr TOK_GE expr                   { $$ = adopt2 ($2, $1, $3); }
          ;

unop      : '+' expr %prec TOK_POS             { $$ = adopt1sym ($1, $2, TOK_POS); }
          | '-' expr %prec TOK_NEG             { $$ = adopt1sym ($1, $2, TOK_NEG); }
          | '!' expr                           { $$ = adopt1 ($1, $2); }
          | TOK_ORD expr                       { $$ = adopt1 ($1, $2); }
          | TOK_CHR expr                       { $$ = adopt1 ($1, $2); }
          ; 

allocator : TOK_NEW TOK_IDENT '(' ')'          { $$ = adopt1syml ($1, $2, TOK_TYPEID); }
          | TOK_NEW  TOK_STRING '(' expr ')'   { $$ = adopt1sym ($1, $4, TOK_NEWSTRING); }
          | TOK_NEW basetype '[' expr ']'      { $$ = adopt2sym ($1, $2, $4, TOK_NEWARRAY); }
          ;

call      : TOK_IDENT exprs ')'                { $$ = adopt1 ($2, $1); }
          ;

exprs     : exprs ',' expr                     { $$ = adopt1 ($1, $3); }
          | '(' expr                           { $$ = adopt1sym ($1, $2, TOK_CALL); }
          | '('                                { $$ = change_sym ($1, TOK_CALL); }
          ;

variable  : TOK_IDENT                          { $$ = $1; }
          | expr '[' expr ']'                  { $$ = adopt2sym ($2, $1, $3, TOK_INDEX); }
          | expr '.' TOK_IDENT                 { $$ = adopt2symr ($2, $1, $3, TOK_FIELD); }
          ;

constant  : TOK_INTCON                         { $$ = $1; }
          | TOK_CHARCON                        { $$ = $1; }
          | TOK_STRINGCON                      { $$ = $1; }
          | TOK_FALSE                          { $$ = $1; }
          | TOK_TRUE                           { $$ = $1; }
          | TOK_NULL                           { $$ = $1; }
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
