// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: symtable.cpp,v 1.1 2015-04-09 18:58:56-07 - - $

#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "lyutils.h"
#include "astree.h"
#include "symtable.h"

symbol_table *idents;
symbol_table *structs;

vector<symbol_table*> symbol_stack;
vector<size_t> block_stack;
size_t next_block = 0;
size_t depth = 0;

const char *attr_string[] = { "void", "bool", "char", "int", "null",
	"string", "struct", "array", "function", "variable",
	"field", "typeid", "param", "lval", "const",
	"vreg", "vaddr"
};

unordered_map<int,int> attr_type = {{TOK_VOID, ATTR_void}, {TOK_BOOL, ATTR_bool},
	{TOK_CHAR, ATTR_char}, {TOK_INT, ATTR_int}, {TOK_STRING, ATTR_string},
	{TOK_TYPEID, ATTR_typeid}
};

symbol *new_symbol (astree *node) {
	symbol *sym = new symbol();
	sym->attributes = 0;
	sym->fields = NULL;
	sym->filenr = node->filenr;
	sym->linenr = node->linenr;
	sym->offset = node->offset;
	sym->blocknr = block_stack.back();
	sym->parameters = NULL;
	return sym;
}

void new_block () {
	depth++;
	block_stack.push_back (next_block);
	next_block++;
	symbol_stack.push_back (NULL);
}

void exit_block () {
	depth--;
	block_stack.pop_back();
	symbol_stack.pop_back();
}

void attr_print (attr_bitset attributes) {
	int need_space = 0;
	for (size_t attr = 0; attr < attributes.size(); attr++) {
		if (attributes[attr]) {
			if (need_space) printf (" ");
			printf ("%s", attr_string[attr]);
			need_space = 1;
		}
	}
}

void sym_print (const string *name, symbol *sym) {
	for (size_t i = 1; i < depth; i++) {
		printf ("   ");
	}
	printf ("%s (%ld.%ld.%ld) {%ld} ", name->c_str(), sym->filenr,
			sym->linenr, sym->offset, sym->blocknr);
	attr_print (sym->attributes);
	printf ("\n");
}

void intern_symtable (const string *key, symbol *val) {
	symbol_table *table = symbol_stack.back();
	if (table == NULL) {
		symbol_stack.pop_back();
		table = new symbol_table();
		(*table)[key] = val;
		symbol_stack.push_back (table);
		sym_print (key, val);
	} else {
		if ((*table)[key] != NULL) {
			// Check existing prototype against function
			errprintf (
				"%: identifier previously declared: %s (%ld.%ld.%ld)\n",
				key->c_str(), val->filenr, val->linenr, val->offset);
		} else {
			(*table)[key] = val;
			sym_print (key, val);
		}
	}
}

symbol *define_ident (astree *type, int attr) {
	attr_bitset attributes = 0;
	if (attr) attributes[attr] = 1;
	if (!attributes[ATTR_function]) {
		attributes[ATTR_variable] = 1;
		attributes[ATTR_lval] = 1;
	}
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		type = type->children[0];
		ident = type->children[1];
	}
	attributes[attr_type[type->symbol]] = 1;
	
	const string *key = ident->lexinfo;
	symbol *sym = new_symbol (ident);
	sym->attributes = attributes;
	intern_symtable (key, sym);
	return sym;
}

void define_struct () {
	
}

void define_func (astree *node) {
	symbol *last = define_ident (node->children[0], ATTR_function);
	new_block();
	astree *param = node->children[1];
	for (size_t child = 0; child < param->children.size(); child++) {
		last->parameters = 
		define_ident (param->children[child], ATTR_param);
		last = last->parameters;
	}
}

void ref_ident (astree *node) {
	int defined = 0;
	const string *key = node->lexinfo;
	for (size_t end = symbol_stack.size(); end >= 1; end--) {
		symbol_table *table = symbol_stack[end - 1];
		if (table != NULL) {
			if ((*table)[key] != NULL) {
				defined = 1;
			}
		}
	}
	if (!defined) {
		errprintf (
			"%: reference to undeclared identifier: %s (%ld.%ld.%ld)\n",
			key->c_str(), node->filenr, node->linenr, node->offset);
	}
}

static int scan_node (astree *node) {
	int block = 0;
	switch (node->symbol) {
		case TOK_STRUCT:
			break;
		case TOK_BLOCK:
			new_block();
			block = 1;
			break;
		case TOK_FUNCTION:
			block = 1;
			define_func (node);
			break;
		case TOK_PROTOTYPE:
			break;
		case TOK_VARDECL:
			define_ident (node->children[0], 0);
			break;
		case TOK_IDENT:
			ref_ident (node);
			break;
	}
	return block;
}

static void scan_astree (astree *root) {
	if (root == NULL) return;
	int block = scan_node (root);
	for (size_t child = 0; child < root->children.size(); child++) {
		scan_astree (root->children[child]);
	}
	if (block) exit_block();
}

void dump_symtable () {
	new_block();
	scan_astree (yyparse_astree);
}
