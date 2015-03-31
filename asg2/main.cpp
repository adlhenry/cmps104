// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: main.cpp,v 1.1 2015-03-26 14:33:42-07 - - $

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
#include "lyutils.h"
#include "stringset.h"
#include "astree.h"

const string cpp_name = "/usr/bin/cpp";
string yyin_cpp_command;
string cpp_opts = "";

// Open a file
FILE *file_open (const char *filename, const char *mode) {
	FILE *file = fopen (filename, mode);
	if (file == NULL) {
		errprintf ("%: failed to open file: %s\n", filename);
		exit (get_exitstatus());
	}
	return file;
}

// Open a pipe to CPP
void yyin_cpp_popen (const char *filename) {
	yyin_cpp_command = cpp_name + " " + cpp_opts + filename;
	yyin = popen (yyin_cpp_command.c_str(), "r");
	if (yyin == NULL) {
		syserrprintf (yyin_cpp_command.c_str());
		exit (get_exitstatus());
	}
}

// Close the pipe to CPP
void yyin_cpp_pclose () {
	int pclose_rc = pclose (yyin);
	eprint_status (yyin_cpp_command.c_str(), pclose_rc);
	if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

bool want_echo () {
	return not (isatty (fileno (stdin)) and isatty (fileno (stdout)));
}

// Scan the user options
const char *scan_opts (int argc, char **argv) {
	int option;
	opterr = 0;
	yy_flex_debug = 0;
	yydebug = 0;
	for(;;) {
		option = getopt (argc, argv, "@:ely");
		if (option == EOF) break;
		switch (option) {
			case '@': set_debugflags (optarg);   break;
			case 'D': cpp_opts = string("-D ") + optarg; break;
			case 'l': yy_flex_debug = 1;         break;
			case 'y': yydebug = 1;               break;
			default:  errprintf ("%: bad option (%c)\n", optopt); break;
		}
	}
	if (optind > argc) {
		errprintf ("Usage: %s [-ly] [filename]\n", get_execname());
		exit (get_exitstatus());
	}
	return optind == argc ? "-" : argv[optind];
}

// Tokenize FILE* yyin
void yytokenize () {
	for (;;) {
		if (yylex() == YYEOF) break;
	}
}

// Check for .oc extension and return the basename
string str_basename (const char *filename) {
	string str_basename = basename (filename);
	size_t index = str_basename.find(".oc");
	if (index == string::npos) {
		errprintf ("%: missing or improper suffix %s\n", 
					str_basename.c_str());
		exit (get_exitstatus());
	}
	return str_basename.substr (0, index);
}

// Create the stringset file
void string_set (string filename) {
	FILE *str_file = file_open (filename.c_str(), "w");
	dump_stringset (str_file);
	fclose (str_file);
}

// Create the tokenset file
void token_set (string filename) {
	FILE *tok_file = file_open (filename.c_str(), "w");
	scanner_tokfile (tok_file);
	yytokenize();
	fclose (tok_file);
}

int main (int argc, char **argv) {
	//int parsecode = 0;
	set_execname (argv[0]);
	DEBUGSTMT ('m',
		for (int argi = 0; argi < argc; ++argi) {
			eprintf ("%s%c", argv[argi], argi < argc - 1 ? ' ' : '\n');
		}
	);
	const char* filename = scan_opts (argc, argv);
	string basename = str_basename (filename);
	yyin_cpp_popen (filename);
	DEBUGF ('m', "filename = %s, yyin = %p, fileno (yyin) = %d\n",
			filename, yyin, fileno (yyin));
	scanner_newfilename (filename);
	token_set (basename + ".tok");
	//scanner_setecho (want_echo());
	/*parsecode = yyparse();
	if (parsecode) {
		errprintf ("%:parse failed (%d)\n", parsecode);
	}else {
		DEBUGSTMT ('a', dump_astree (stderr, yyparse_astree); );
		emit_sm_code (yyparse_astree);
	}
	free_ast (yyparse_astree);*/
	yyin_cpp_pclose();
	string_set (basename + ".str");
	yylex_destroy();
	return get_exitstatus();
}
