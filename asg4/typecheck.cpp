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
vector<astree*> return_stack {NULL};

unordered_map<int,int> attr_type2 = {{TOK_VOID, ATTR_void},
	{TOK_BOOL, ATTR_bool}, {TOK_CHAR, ATTR_char}, {TOK_INT, ATTR_int},
	{TOK_STRING, ATTR_string}, {TOK_TYPEID, ATTR_typeid}
};

void err_print (astree *node, type_pair type1, char err) {
	string error = "%: ";
	switch (err) {
		case 'v':
			error += "%s declares identifier of type %s";
			break;
		case 'f':
			error += "%s expects return type %s, ";
			error += "but is missing return statement";
			break;
		case 'i':
			error += "%s operator passed non-indexable type %s";
			break;
	}
	error += " at (%ld.%ld.%ld)\n";
	errprintf (error.c_str(), node->lexinfo->c_str(),
		get_attrstring (type1.first, type1.second),
		node->filenr, node->linenr, node->offset);
}

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
		ident = type->children[1];
		type = type->children[0];
	}
	attr_bitset i_type = get_type (ident);
	attr_bitset e_type = get_type (expr);
	type_pair type1 = {ident->type.first, i_type};
	type_pair type2 = {expr->type.first, e_type};
	if (i_type[ATTR_void]) {
		err_print (node, type1, 'v');
	} else if (i_type != e_type) {
		if (!i_type[ATTR_string]
			&& !i_type[ATTR_typeid]
			&& !i_type[ATTR_array]) {
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
	astree *expr1 = node->children[0];
	attr_bitset b_type = 0;
	b_type[ATTR_bool] = 1;
	attr_bitset e1_type = get_type (expr1);
	type_pair type0 = {NULL, b_type};
	type_pair type1 = {expr1->type.first, e1_type};
	if (e1_type != b_type) {
		err_print (node, type0, type1);
	}
}

void get_return (astree *root) {
	if (root == NULL) return;
	int sym = root->symbol;
	if ((sym == TOK_RETURN) | (sym == TOK_RETURNVOID)) {
		return_stack.push_back (root);
	}
	for (size_t child = 0; child < root->children.size(); child++) {
		get_return (root->children[child]);
	}
}

void check_return (astree *node) {
	astree *type = node->children[0];
	astree *ident = type->children[0];
	astree *block = node->children[2];
	if (type->symbol == TOK_ARRAY) {
		ident = type->children[1];
		type = type->children[0];
	}
	attr_bitset i_type = get_type (ident);
	type_pair type1 = {ident->type.first, i_type};
	get_return (block);
	if (!i_type[ATTR_void] && return_stack.back() == NULL) {
		err_print (ident, type1, 'f');
	}
	while (return_stack.back() != NULL) {
		astree *ret = return_stack.back();
		attr_bitset e_type = 0;
		e_type[ATTR_void] = 1;
		type_pair type2 = {NULL, e_type};
		if (ret->symbol == TOK_RETURNVOID) {
			if (!i_type[ATTR_void]) {
				err_print (ret, type1, type2);
			}
			return_stack.pop_back();
			continue;
		}
		astree *expr = ret->children[0];
		e_type = get_type (expr);
		type2 = {expr->type.first, e_type};
		if (i_type[ATTR_void]) {
			err_print (ret, type1, type2);
		} else if (i_type != e_type) {
			if (!i_type[ATTR_string]
				&& !i_type[ATTR_typeid]
				&& !i_type[ATTR_array]) {
				err_print (ret, type1, type2);
			} else if (!e_type[ATTR_null]) {
				err_print (ret, type1, type2);
			}
		} else {
			if (i_type[ATTR_typeid]
				&& (ident->type.first != expr->type.first)) {
				err_print (ret, type1, type2);
			}
		}
		return_stack.pop_back();
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

void check_eq (astree *node) {
	astree *expr1 = node->children[0];
	astree *expr2 = node->children[1];
	attr_bitset e1_type = get_type (expr1);	
	attr_bitset e2_type = get_type (expr2);
	type_pair type1 = {expr1->type.first, e1_type};
	type_pair type2 = {expr2->type.first, e2_type};
	if (e1_type != e2_type) {
		err_print (node, type1, type2);
	}
}

void check_boolop (astree *node) {
	astree *expr1 = node->children[0];
	astree *expr2 = node->children[1];
	attr_bitset p_type = 0;
	p_type[ATTR_bool] = p_type[ATTR_char] = p_type[ATTR_int] = 1;
	attr_bitset e1_type = get_type (expr1);	
	attr_bitset e2_type = get_type (expr2);
	type_pair type0 = {NULL, p_type};
	type_pair type1 = {expr1->type.first, e1_type};
	type_pair type2 = {expr2->type.first, e2_type};
	if (!(e1_type[ATTR_bool] | e1_type[ATTR_char]
		| e1_type[ATTR_int])) {
		err_print (node, type0, type1);
	} else if (e1_type != e2_type) {
		err_print (node, type1, type2);
	}
}

void check_binop (astree *node) {
	astree *expr1 = node->children[0];
	astree *expr2 = node->children[1];
	attr_bitset i_type = 0;
	i_type[ATTR_int] = 1;
	attr_bitset e1_type = get_type (expr1);	
	attr_bitset e2_type = get_type (expr2);
	type_pair type0 = {NULL, i_type};
	type_pair type1 = {expr1->type.first, e1_type};
	type_pair type2 = {expr2->type.first, e2_type};
	if (e1_type != i_type) {
		err_print (node, type0, type1);
	} else if (e1_type != e2_type) {
		err_print (node, type1, type2);
	}
}

void check_sign (astree *node) {
	astree *expr1 = node->children[0];
	attr_bitset i_type = 0;
	i_type[ATTR_int] = 1;
	attr_bitset e1_type = get_type (expr1);	
	type_pair type0 = {NULL, i_type};
	type_pair type1 = {expr1->type.first, e1_type};
	if (e1_type != i_type) {
		err_print (node, type0, type1);
	}
}

void check_ord (astree *node) {
	astree *expr1 = node->children[0];
	attr_bitset c_type = 0;
	c_type[ATTR_char] = 1;
	attr_bitset e1_type = get_type (expr1);	
	type_pair type0 = {NULL, c_type};
	type_pair type1 = {expr1->type.first, e1_type};
	if (e1_type != c_type) {
		err_print (node, type0, type1);
	}
}

void check_newarray (astree *node) {
	astree *type = node->children[0];
	astree *expr = node->children[1];
	attr_bitset i_type = 0;
	i_type[ATTR_int] = 1;
	attr_bitset t_type = 0;
	t_type[attr_type2[type->symbol]] = 1;
	attr_bitset e_type = get_type (expr);	
	type_pair type0 = {NULL, i_type};
	type_pair type1 = {type->type.first, t_type};
	type_pair type2 = {expr->type.first, e_type};
	if (t_type[ATTR_void]) {
		err_print (node, type1, 'v');
	} else if (e_type != i_type) {
		err_print (node, type0, type2);
	}
	t_type[ATTR_array] = 1;
	node->attributes |= t_type;
	if (t_type[ATTR_typeid]) {
		node->type.first = type->lexinfo;
	}
}

void check_index (astree *node) {
	astree *expr1 = node->children[0];
	astree *expr2 = node->children[1];
	attr_bitset i_type = 0;
	i_type[ATTR_int] = 1;
	attr_bitset e1_type = get_type (expr1);	
	attr_bitset e2_type = get_type (expr2);
	type_pair type0 = {NULL, i_type};
	type_pair type1 = {expr1->type.first, e1_type};
	type_pair type2 = {expr2->type.first, e2_type};
	if (e1_type[ATTR_array]) {
		e1_type[ATTR_array] = 0;
		node->attributes |= e1_type;
		node->type.first = expr1->type.first;
	} else if (e1_type[ATTR_string]) {
		node->attributes[ATTR_char] = 1;
	} else {
		err_print (node, type1, 'i');
	}
	if (e2_type != i_type) {
		err_print (node, type0, type2);
	}
}

void type_check (astree *node) {
	int sym = node->symbol;
	if (sym == TOK_VARDECL) check_vardecl (node);
	if ((sym == TOK_WHILE) | (sym == TOK_IF) | (sym == TOK_IFELSE)) {
		check_control (node);
	}
	if (sym == TOK_FUNCTION) check_return (node);
	if (sym == '=') check_assing (node);
	if ((sym == TOK_EQ) | (sym == TOK_NE)) check_eq (node);
	if ((sym == TOK_LT) | (sym == TOK_LE) | (sym == TOK_GT)
		| (sym == TOK_GE)) {
		check_boolop (node);
	
	}
	if ((sym == '+') | (sym == '-') | (sym == '*')
		| (sym == '/') | (sym == '%')) {
		check_binop (node);
	
	}
	if ((sym == TOK_POS) | (sym == TOK_NEG)) check_sign (node);
	if (sym == '!') check_control (node);
	if (sym == TOK_ORD) check_ord (node);
	if (sym == TOK_CHR) check_sign (node);
	if (sym == TOK_NEWSTRING) check_sign (node);
	if (sym == TOK_NEWARRAY) check_newarray (node);
	if (sym == TOK_INDEX) check_index (node);
}
