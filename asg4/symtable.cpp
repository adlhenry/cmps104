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
	block_stack.push_back (next_block);
	next_block++;
	symbol_stack.push_back (NULL);
}

void exit_block () {
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
	printf ("%s (%ld.%ld.%ld) {%ld} ", name->c_str(), sym->filenr,
			sym->linenr, sym->offset, sym->blocknr);
	attr_print (sym->attributes);
	printf ("\n");
}

void define_struct () {
	
}

void define_var (astree *node) {
	astree *type = node->children[0];
	astree *ident = type->children[0];
	attr_bitset attributes = 0;
	attributes[ATTR_variable] = 1;
	attributes[ATTR_lval] = 1;
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
			break;
		case TOK_PROTOTYPE:
			break;
		case TOK_VARDECL:
			define_var (node);
			break;
		case TOK_IDENT:
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
