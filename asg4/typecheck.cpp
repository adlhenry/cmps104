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

using type_pair = pair<const string*,attr_bitset>;

void err_print (astree *node, type_pair type1, type_pair type2) {
	string error = "%: ";
	error += "%s expects type %s but operand is of type %s";
	error += " at (%ld.%ld.%ld)\n";
	errprintf (error.c_str(), node->lexinfo->c_str(),
		get_attrstring (type1.first, type1.second),
		get_attrstring (type2.first, type2.second),
		node->filenr, node->linenr, node->offset);
}

attr_bitset get_type (astree *node) {
	attr_bitset type = node->attributes;
	for (size_t attr = ATTR_function; attr < type.size(); attr++) {
		if (attr != ATTR_typeid) type[attr] = 0;
	}
	return type;
}

void check_vardecl (astree *node) {
	astree *type = node->children[0];
	astree *expr = node->children[1];
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		type = type->children[0];
		ident = type->children[1];
	}
	attr_bitset i_type = get_type (ident);	
	attr_bitset e_type = get_type (expr);
	type_pair type1 = {ident->type.first, i_type};
	type_pair type2 = {expr->type.first, e_type};
	if (i_type != e_type) {
		if (!i_type[ATTR_string]
			| !i_type[ATTR_typeid]
			| !i_type[ATTR_array]) {
			err_print (node, type1, type2);
		} else if (!e_type[ATTR_null]) {
			err_print (node, type1, type2);
		}
	} else {
		if (i_type[ATTR_typeid]
			&& (ident->type.first != expr->type.first)) {
			err_print (node, type1, type2);
		}
	}
}

void check_control (astree *node) {
	astree *expr = node->children[0];
	attr_bitset c_type = 0;
	c_type[ATTR_bool] = 1;
	attr_bitset e_type = get_type (expr);
	type_pair type1 = {NULL, c_type};
	type_pair type2 = {expr->type.first, e_type};
	if (c_type != e_type) {
		err_print (node, type1, type2);
	}
}
  
void check_assing (astree *node) {
	astree *expr1 = node->children[0];
	astree *expr2 = node->children[1];
	attr_bitset e1_type = get_type (expr1);	
	attr_bitset e2_type = get_type (expr2);
	type_pair type1 = {expr1->type.first, e1_type};
	type_pair type2 = {expr2->type.first, e2_type};
	if (!expr1->attributes[ATTR_lval]) {
		attr_bitset l_type = e1_type;
		l_type[ATTR_lval] = 1;
		type2 = type1;
		type1.second = l_type;
		err_print (node, type1, type2);
	} else if (e1_type != e2_type) {
		err_print (node, type1, type2);
	}
	node->attributes |= e1_type;
	node->type.first = expr1->type.first;
}

void type_check (astree *node) {
	switch (node->symbol) {
		case TOK_VARDECL:
			check_vardecl (node);
			break;
		case TOK_WHILE:
			check_control (node);
			break;
		case TOK_IF:
			check_control (node);
			break;
		case TOK_IFELSE:
			check_control (node);
			break;
		case '=':
			check_assing (node);
			break;
	}
}
