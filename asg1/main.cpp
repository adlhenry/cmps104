// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: main.cpp,v 1.1 2015-03-01 16:26:03-08 - - $

#include <string>
#include <vector>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auxlib.h"
#include "stringset.h"
/*#include "lyutils.h"
#include "astree.h"
#include "emit.h"*/

// Temporary bypass variables
FILE *yyin;
int yy_flex_debug;
int yydebug;

const string cpp_name = "/usr/bin/cpp";
string yyin_cpp_command;

// Open a pipe from the C preprocessor.
// Exit failure if can't.
// Assignes opened pipe to FILE *yyin.
void yyin_cpp_popen (const char *filename) {
	yyin_cpp_command = cpp_name + " " + filename;
	yyin = popen (yyin_cpp_command.c_str(), "r");
	if (yyin == NULL) {
		syserrprintf (yyin_cpp_command.c_str());
		exit (get_exitstatus());
	}
}

void yyin_cpp_pclose (void) {
	int pclose_rc = pclose (yyin);
	eprint_status (yyin_cpp_command.c_str(), pclose_rc);
	if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

bool want_echo () {
	return not (isatty (fileno (stdin)) and isatty (fileno (stdout)));
}

void scan_opts (int argc, char **argv) {
	int option;
	opterr = 0;
	yy_flex_debug = 0;
	yydebug = 0;
	for(;;) {
		option = getopt (argc, argv, "@:ely");
		if (option == EOF) break;
		switch (option) {
			case '@': set_debugflags (optarg);   break;
			case 'l': yy_flex_debug = 1;         break;
			case 'y': yydebug = 1;               break;
			default:  errprintf ("%:bad option (%c)\n", optopt); break;
		}
	}
	if (optind > argc) {
		errprintf ("Usage: %s [-ly] [filename]\n", get_execname());
		exit (get_exitstatus());
	}
	const char* filename = optind == argc ? "-" : argv[optind];
	yyin_cpp_popen (filename);
	DEBUGF ('m', "filename = %s, yyin = %p, fileno (yyin) = %d\n",
			filename, yyin, fileno (yyin));
	//scanner_newfilename (filename);
}


// Test harness to tokenize FILE* yyin
void tokenize () {
	for (;;) {
		char line[1024];
		char *rc = fgets (line, 1024, yyin);
		if (rc == NULL) break;
		
		char *token = strtok_r (line, " \t\n", &lpos);
		intern_stringset (token);
	}
}

int main (int argc, char **argv) {
	//int parsecode = 0;
	set_execname (argv[0]);
	DEBUGSTMT ('m',
		for (int argi = 0; argi < argc; ++argi) {
			eprintf ("%s%c", argv[argi], argi < argc - 1 ? ' ' : '\n');
		}
	);
	scan_opts (argc, argv);
	/*scanner_setecho (want_echo());
	parsecode = yyparse();
	if (parsecode) {
		errprintf ("%:parse failed (%d)\n", parsecode);
	}else {
		DEBUGSTMT ('a', dump_astree (stderr, yyparse_astree); );
		emit_sm_code (yyparse_astree);
	}
	free_ast (yyparse_astree);*/
	tokenize();
	yyin_cpp_pclose();
	/*DEBUGSTMT ('s', dump_stringset (stderr); );
	yylex_destroy();*/
	return get_exitstatus();
}
