// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: lyutils.cpp,v 1.1 2015-03-26 14:33:42-07 - - $

#include <vector>
#include <string>
using namespace std;

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "auxlib.h"

astree *yyparse_astree = NULL;
int scan_linenr = 1;
int scan_offset = 0;
bool scan_echo = false;
vector<string> included_filenames;
FILE *tok_file = NULL;

// Specify the token file
void scanner_tokfile (FILE *out) {
	tok_file = out;
}

const string *scanner_filename (int filenr) {
	return &included_filenames.at(filenr);
}

void scanner_newfilename (const char *filename) {
	included_filenames.push_back (filename);
}

void scanner_newline (void) {
	++scan_linenr;
	scan_offset = 0;
}

void scanner_setecho (bool echoflag) {
	scan_echo = echoflag;
}

void scanner_useraction (void) {
	if (scan_echo) {
		if (scan_offset == 0) printf (";%5d: ", scan_linenr);
		printf ("%s", yytext);
	}
	scan_offset += yyleng;
}

void yyerror (const char *message) {
	assert (not included_filenames.empty());
	errprintf ("%: %s: %d: %s\n",
				included_filenames.back().c_str(), scan_linenr, message);
}

void scanner_badchar (unsigned char bad) {
	char char_rep[16];
	sprintf (char_rep, isgraph ((int) bad) ? "%c" : "\\%03o", bad);
	errprintf ("%: %s: %d: invalid source character (%s)\n",
				included_filenames.back().c_str(), scan_linenr, char_rep);
}

void scanner_badtoken (char *lexeme) {
	errprintf ("%: %s: %d: invalid token (%s)\n",
				included_filenames.back().c_str(), scan_linenr, lexeme);
}

// Print the token
void print_token (astree *node) {
	fprintf (tok_file, "%4ld %3ld.%03ld %4d %-13s (%s)\n",
		node->filenr, node->linenr, node->offset,
		node->symbol, get_yytname (node->symbol),
		node->lexinfo->c_str());
}

// Print the directive
void print_directive (int linenr, const char *filename) {
	fprintf (tok_file, "# %d \"%s\"\n", linenr, filename);
}

int yylval_token (int symbol) {
	int offset = scan_offset - yyleng;
	yylval = new_astree (symbol, included_filenames.size() - 1,
						scan_linenr, offset, yytext);
	print_token (yylval);
	return symbol;
}

void error_destructor (astree *tree) {
	if (tree == yyparse_astree) return;
	DEBUGSTMT ('a', dump_astree (stderr, tree); );
	free_ast (tree);
}

astree *new_parseroot (void) {
	yyparse_astree = new_astree (TOK_ROOT, 0, 0, 0, "<<ROOT>>");
	return yyparse_astree;
}

void scanner_include (void) {
	scanner_newline();
	char filename[strlen (yytext) + 1];
	int linenr;
	int scan_rc = sscanf (yytext, "# %d \"%[^\"]\"", &linenr, filename);
	if (scan_rc != 2) {
		errprintf ("%: %d: [%s]: invalid directive, ignored\n",
				scan_rc, yytext);
	} else {
		print_directive (linenr, filename);
		scanner_newfilename (filename);
		scan_linenr = linenr - 1;
		DEBUGF ('m', "filename=%s, scan_linenr=%d\n",
				included_filenames.back().c_str(), scan_linenr);
	}
}
