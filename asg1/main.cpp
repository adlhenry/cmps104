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

// Temporary bypass variables
FILE *yyin;
int yy_flex_debug;
int yydebug;

const string cpp_name = "/usr/bin/cpp";
string yyin_cpp_command;
string cpp_opts = "";

// Open a file
FILE *file_open (string filename, const char *mode) {
	FILE *file = fopen (filename.c_str(), mode);
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

// Test harness to tokenize FILE* yyin
void yytokenize () {
	for (;;) {
		char line[1024];
		char *rc = fgets (line, 1024, yyin);
		if (rc == NULL) break;
		char *token = strtok (line, " \t\n");
		while (token != NULL) {
			intern_stringset (token);
			token = strtok (NULL, " \t\n");
		}
	}
}

// Check for .oc extension and return the basename
string str_basename (const char *filename) {
	string str_basename = basename (filename);
	size_t index = str_basename.find(".oc");
	if (index == string::npos) {
		errprintf ("%: missing or improper suffix %s\n", str_basename.c_str());
		exit (get_exitstatus());
	}
	return str_basename.substr (0, index);
}

int main (int argc, char **argv) {
	set_execname (argv[0]);
	const char* filename = scan_opts (argc, argv);
	string basename = str_basename (filename);
	FILE *str_file = file_open (basename + ".str", "w");
	yyin_cpp_popen (filename);
	yytokenize();
	yyin_cpp_pclose();
	dump_stringset (str_file);
	fclose (str_file);
	return get_exitstatus();
}
