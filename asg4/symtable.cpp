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

/*attr_bitset set_attr (astree *node, astree *type, astree *ident) {
	attr_bitset attributes = 0;
	attributes[ATTR_variable] = 1;
	attributes[ATTR_lval] = 1;
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		type = type->children[0];
		ident = type->children[1];
	}
	attributes[attr_type[type->symbol]] = 1;
	return attributes;
}*/

symbol *intern_symtable (const string *key, symbol *val) {
	symbol_table *table = symbol_stack.back();
	if (table == NULL) {
		symbol_stack.pop_back();
		table = new symbol_table();
		(*table)[key] = val;
		symbol_stack.push_back (table);
		sym_print (key, val);
	} else {
		if ((*table)[key] != NULL) {
			errprintf (
				"%: identifier previously declared: %s (%ld.%ld.%ld)\n",
				key->c_str(), val->filenr, val->linenr, val->offset);
			return NULL;
		} else {
			(*table)[key] = val;
			sym_print (key, val);
		}
	}
	return val;
}

void define_struct () {
	
}

symbol_entry define_ident (astree *type) {
	attr_bitset attributes = 0;
	attributes[ATTR_variable] = 1;
	attributes[ATTR_lval] = 1;
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		type = type->children[0];
		ident = type->children[1];
	}
	attributes[attr_type[type->symbol]] = 1;
	
	const string *key = ident->lexinfo;
	symbol *var = new_symbol (ident);
	var->attributes = attributes;
	intern_symtable (key, var);
	return make_pair (key, var);
}

void define_func (astree *node) {
	attr_bitset attributes = 0;
	attributes[ATTR_function] = 1;
	astree *type = node->children[0];
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		type = type->children[0];
		ident = type->children[1];
	}
	attributes[attr_type[type->symbol]] = 1;
	
	const string *key = ident->lexinfo;
	symbol *func = new_symbol (ident);
	func->attributes = attributes;
	func = intern_symtable (key, func);
	
	new_block();
	astree *param = node->children[1];
	symbol_entry entry = define_ident (param->children[0]);
	symbol *last = entry.second;
	last->attributes[ATTR_param] = 1;
	sym_print (entry.first, entry.second);
	func->parameters = last;
	for (size_t child = 1; child < param->children.size(); child++) {
		last->attributes[ATTR_param] = 1;
		entry = define_ident (param->children[child]);
		last->parameters = entry.second;
		last = last->parameters;
		sym_print (entry.first, entry.second);
	}
	last->attributes[ATTR_param] = 1;
	last->parameters = NULL;
}

void define_var (astree *node) {
	attr_bitset attributes = 0;
	attributes[ATTR_variable] = 1;
	attributes[ATTR_lval] = 1;
	astree *type = node->children[0];
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		type = type->children[0];
		ident = type->children[1];
	}
	attributes[attr_type[type->symbol]] = 1;
	
	const string *key = ident->lexinfo;
	symbol *var = new_symbol (ident);
	var->attributes = attributes;
	symbol_table *table = symbol_stack.back();
	if (table == NULL) {
		symbol_stack.pop_back();
		table = new symbol_table();
		(*table)[key] = var;
		symbol_stack.push_back (table);
		sym_print (key, var);
	} else {
		if ((*table)[key] != NULL) {
			errprintf (
				"%: identifier previously declared: %s (%ld.%ld.%ld)\n",
				key->c_str(), var->filenr, var->linenr, var->offset);
		} else {
			(*table)[key] = var;
			sym_print (key, var);
		}
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
			define_var (node);
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
