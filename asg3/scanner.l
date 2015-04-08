%{
// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: scanner.l,v 1.1 2015-03-31 12:37:36-07 - - $

#include "auxlib.h"
#include "lyutils.h"

#define YY_USER_ACTION  { scanner_useraction (); }
#define IGNORE(THING)   { }

%}

%option 8bit
%option debug
%option nodefault
%option nounput
%option noyywrap
%option verbose
%option warn

LETTER          [A-Za-z_]
DIGIT           [0-9]
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
NOTIDENT        ({DIGIT}({LETTER}|{DIGIT})*)
INTCON          ({DIGIT}+)
CHAR            ([^\\'\n]|\\[\\'"0nt])
SCHAR           ([^\\"\n]|\\[\\'"0nt])
NOTCHAR         (\\[^\\'"0nt])
CHARN           ({CHAR}|{NOTCHAR})
SCHARN          ({SCHAR}|{NOTCHAR})
CHARCON         ('{CHAR}')
STRINGCON       (\"{SCHAR}*\")
NOTCHARCON      ('{CHARN}*|'{CHAR}*{NOTCHAR}{CHARN}*'|'{CHAR}{2,}'|'')
NOTSTRINGCON    (\"{SCHARN}*|\"{SCHAR}*{NOTCHAR}{SCHARN}*\")

%%

"#".*           { scanner_include(); }
[ \t]+          { IGNORE (white space) }
\n              { scanner_newline(); }

"[]"            { return yylval_token (TOK_ARRAY); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }
";"             { return yylval_token (';'); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
"="             { return yylval_token ('='); }
"=="            { return yylval_token (TOK_EQ); }
"!="            { return yylval_token (TOK_NE); }
"<"             { return yylval_token (TOK_LT); }
"<="            { return yylval_token (TOK_LE); }
">"             { return yylval_token (TOK_GT); }
">="            { return yylval_token (TOK_GE); }
"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }

"void"          { return yylval_token (TOK_VOID); }
"bool"          { return yylval_token (TOK_BOOL); }
"char"          { return yylval_token (TOK_CHAR); }
"int"           { return yylval_token (TOK_INT); }
"string"        { return yylval_token (TOK_STRING); }
"struct"        { return yylval_token (TOK_STRUCT); }
"if"            { return yylval_token (TOK_IF); }
"else"          { return yylval_token (TOK_ELSE); }
"while"         { return yylval_token (TOK_WHILE); }
"return"        { return yylval_token (TOK_RETURN); }
"false"         { return yylval_token (TOK_FALSE); }
"true"          { return yylval_token (TOK_TRUE); }
"null"          { return yylval_token (TOK_NULL); }
"ord"           { return yylval_token (TOK_ORD); }
"chr"           { return yylval_token (TOK_CHR); }
"new"           { return yylval_token (TOK_NEW); }

{IDENT}         { return yylval_token (TOK_IDENT); }
{INTCON}        { return yylval_token (TOK_INTCON); }
{CHARCON}       { return yylval_token (TOK_CHARCON); }
{STRINGCON}     { return yylval_token (TOK_STRINGCON); }

{NOTIDENT}      { scanner_badtoken (yytext);
                 return yylval_token (TOK_IDENT); }
{NOTCHARCON}    { scanner_badtoken (yytext);
                 return yylval_token (TOK_CHARCON); }
{NOTSTRINGCON}  { scanner_badtoken (yytext);
                 return yylval_token (TOK_STRINGCON); }

.               { scanner_badchar (*yytext); }

%%
