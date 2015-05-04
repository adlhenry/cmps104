// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: typecheck.cpp,v 1.1 2015-04-09 18:58:56-07 - - $

#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "lyutils.h"
#include "astree.h"
#include "symtable.h"
#include "typecheck.h"

void err_print (astree *node, int type1, int type2, char err) {
	string error = "%: ";
	switch (err) {
		case 'v':
			error += "assignment expects type %d";
			error += " but operand is of type %d";
			break;
	}
	error += ": %s (%ld.%ld.%ld)\n";
	errprintf (error.c_str(), type1, type2, node->lexinfo->c_str(),
		node->filenr, node->linenr, node->offset);
}

void set_vardecl (astree *node) {
	astree *type = node->children[0];
	astree *expr = node->children[1];
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		type = type->children[0];
		ident = type->children[1];
	}
	attr_bitset i_attr = ident->attributes;
	attr_bitset e_attr = expr->attributes;
	int i_type = 0;
	int e_type = 0;
	for (int attr = 1; attr < ATTR_null; attr++) {
		if (i_attr[attr]) i_type = attr;
		if (e_attr[attr]) e_type = attr;
	}
	if (i_type != e_type) err_print (node, i_type, e_type, 'v');
	if (i_attr[ATTR_string]) i_type = ATTR_string;
	if (e_attr[ATTR_string]) e_type = ATTR_string;
	if (i_attr[ATTR_typeid]) i_type = ATTR_typeid;
	if (e_attr[ATTR_typeid]) e_type = ATTR_typeid;
	if (e_attr[ATTR_null]) e_type = ATTR_null;
	if (i_type != e_type) {
		if (e_type != ATTR_null) err_print (node, i_type, e_type, 'v');
	}
	if (i_attr[ATTR_array] != e_attr[ATTR_array]) {
		err_print (node, i_type, e_type, 'v');
	}
	if (i_attr[ATTR_typeid]) {
		if (ident->type.first != expr->type.first) {
			err_print (node, 0, 0, 'v');
		}
	}
}

static void scan_node (astree *node) {
	switch (node->symbol) {
		case TOK_VARDECL:
			set_vardecl (node);
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

void type_check () {
	scan_astree (yyparse_astree);
}
