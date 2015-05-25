// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: emit.cpp,v 1.1 2015-05-22 15:22:23-07 - - $

#include <vector>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "lyutils.h"
#include "astree.h"
#include "symtable.h"
#include "emit.h"

using type_pair = pair<const string*,attr_bitset>;

FILE *oil_file = NULL;
size_t register_number = 1;

vector<astree*> struct_queue;
vector<astree*> sconst_queue;
vector<astree*> gvar_queue;
vector<astree*> func_queue;

const char *type_string[] = { "void", "char", "char", "int", "null",
	"char*", "struct", "*"
};

void struct_queue_add (astree *node) {
	struct_queue.push_back (node);
}

void sconst_queue_add (astree *node) {
	sconst_queue.push_back (node);
}

void gvar_queue_add (astree *node) {
	if (node->blocknr == 0) {
		gvar_queue.push_back (node);
	}
}

void func_queue_add (astree *node) {
	func_queue.push_back (node);
}

string get_type (type_pair type) {
	string type_str = "";
	if (type.second[ATTR_typeid]) {
		type_str += "struct s_";
		type_str += type.first->c_str();
		type_str += "*";
	}
	for (size_t attr = 0; attr < ATTR_function; attr++) {
		if (type.second[attr]) {
			type_str += type_string[attr];
		}
	}
	return type_str;
}

astree *get_ident (astree *type) {
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		ident = type->children[1];
	}
	return ident;
}

void emit_struct (astree *node) {
	symbol_entry type = node->children[0]->type;
	const string *s_name = type.first;
	fprintf (oil_file, "struct s_%s {\n", s_name->c_str());
	for (size_t child = 1; child < node->children.size(); child++) {
		astree *ident = get_ident (node->children[child]);
		const string *f_name = ident->lexinfo;
		type_pair i_type = {ident->type.first, ident->attributes};
		string f_type = get_type (i_type);
		fprintf (oil_file, "        %s f_%s_%s;\n", f_type.c_str(),
			s_name->c_str(), f_name->c_str());
		
	}
	fprintf (oil_file, "};\n");
}

void emit_sconst (astree *node) {
	const string *s_name = node->lexinfo;
	fprintf (oil_file, "char* s%ld = %s;\n", register_number++,
		s_name->c_str());
}

void emit_gvar (astree *node) {
	astree *ident = get_ident (node->children[0]);
	const string *name = ident->lexinfo;
	type_pair i_type = {ident->type.first, ident->attributes};
	string type = get_type (i_type);
	fprintf (oil_file, "%s __%s;\n", type.c_str(), name->c_str());
}

void emit_param (astree *node) {
	astree *ident = get_ident (node);
	const string *name = ident->lexinfo;
	type_pair i_type = {ident->type.first, ident->attributes};
	string type = get_type (i_type);
	fprintf (oil_file, "        %s _%ld_%s", type.c_str(),
		ident->blocknr, name->c_str());
}

void emit_func (astree *node) {
	astree *ident = get_ident (node->children[0]);
	astree *param = node->children[1];
	astree *block = node->children[2];
	const string *name = ident->lexinfo;
	type_pair i_type = {ident->type.first, ident->attributes};
	string type = get_type (i_type);
	fprintf (oil_file, "%s __%s (\n", type.c_str(), name->c_str());
	for (size_t child = 0; child < param->children.size(); child++) {
		emit_param (param->children[child]);
		if (child == param->children.size() - 1) {
			fprintf (oil_file, ")\n");
		} else {
			fprintf (oil_file, ",\n");
		}
	}
	fprintf (oil_file, "{\n");
	
	fprintf (oil_file, "}\n");
}

void emit_queue (void (*emit)(astree*), vector<astree*> queue) {
	for (size_t i = 0; i < queue.size(); i++) {
		emit (queue[i]);
	}
}

void emit_code (FILE *out) {
	oil_file = out;
	emit_queue (&emit_struct, struct_queue);
	emit_queue (&emit_sconst, sconst_queue);
	emit_queue (&emit_gvar, gvar_queue);
	emit_queue (&emit_func, func_queue);
	//emit_main (yyparse_astree);
}
