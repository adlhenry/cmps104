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
size_t next_block = 1;

symbol *new_symbol (astree *node) {
	symbol *sym = new symbol();
	sym->attributes = 0;
	sym->fields = NULL;
	sym->filenr = node->filenr;
	sym->linenr = node->linenr;
	sym->offset = node->offset;
	sym->blocknr = next_block - 1;
	sym->parameters = NULL;
	return sym;
}

void new_block () {
	next_block++;
	symbol_stack.push_back (NULL);
}

void exit_block () {
	symbol_stack.pop_back();
}

void define_struct () {
	
}

void define_ident (astree *node) {
	symbol *ident = new_symbol (node);
	symbol_table *table = symbol_stack.back();
	if (table[node->lexinfo] != NULL) {
		// Double declaration error
	} else {
		table[node->lexinfo] = ident;
	}
}

static void scan_node (astree *node) {
	switch (node->symbol) {
		case TOK_STRUCT:
			break;
		case TOK_BLOCK:
			new_block();
			break;
		case TOK_FUNCTION:
			break;
		case TOK_PROTOTYPE:
			break;
		case TOK_VARDECL:
			define_ident (node);
			break;
		case TOK_IDENT:
			break;
	}
}

static void scan_astree (astree *root) {
	if (root == NULL) return;
	scan_node (root);
	for (size_t child = 0; child < root->children.size(); child++) {
		scan_astree (root->children[child]);
	}
}
