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

const char *type_string[] = { "", "char", "char", "int", "",
	"char*", "struct", "*"
};

const char *reg_string[] = { "", "c", "c", "i", "",
	"p", "p", "p"
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

string get_register (type_pair type) {
	string reg = "";
	for (size_t attr = 0; attr < ATTR_function; attr++) {
		if (type.second[attr]) {
			reg = reg_string[attr];
		}
	}
	if (type.second[ATTR_vaddr]) reg = "a";
	reg += to_string (register_number);
	register_number++;
	return reg;
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

string emit_expr (astree *node) {
	string reg = "";
	int sym = node->symbol;
	if (sym == '=') {
		string expr1 = emit_expr (node->children[0]);
		string expr2 = emit_expr (node->children[1]);
		fprintf (oil_file, "        %s = %s;\n", expr1.c_str(),
			expr2.c_str());
	}
	if ((sym == '+') | (sym == '-') | (sym == '*') | (sym == '/')
		| (sym == '%') | (sym == TOK_EQ) | (sym == TOK_NE)
		| (sym == TOK_LT) | (sym == TOK_LE)| (sym == TOK_GT)
		| (sym == TOK_GE)) {
		string expr1 = emit_expr (node->children[0]);
		string expr2 = emit_expr (node->children[1]);
		type_pair b_type = {node->type.first, node->attributes};
		string type = get_type (b_type);
		reg = get_register (b_type);
		fprintf (oil_file, "        %s %s = %s %s %s;\n", type.c_str(),
			reg.c_str(), expr1.c_str(), node->lexinfo->c_str(),
			expr2.c_str());
	}
	if ((sym == TOK_POS) | (sym == TOK_NEG) | (sym == '!')
		| (sym == TOK_ORD) | (sym == TOK_CHR)) {
		string unop = *node->lexinfo;
		if (sym == TOK_ORD) unop = "(int)";
		if (sym == TOK_CHR) unop = "(char)";
		string expr1 = emit_expr (node->children[0]);
		type_pair u_type = {node->type.first, node->attributes};
		string type = get_type (u_type);
		reg = get_register (u_type);
		fprintf (oil_file, "        %s %s = %s%s;\n", type.c_str(),
			reg.c_str(), unop.c_str(), expr1.c_str());
	}
	// allocator
	// call
	if (sym == TOK_IDENT) {
		reg = "_";
		if (node->blocknr != 0) reg += to_string (node->blocknr);
		reg += "_";
		reg += *node->lexinfo;
	}
	// index
	// field selection
	if (node->attributes[ATTR_const]) reg = *node->lexinfo;
	return reg;
}

void emit_block (astree *node, void (*emit_statement)(astree*)) {
	for (size_t child = 0; child < node->children.size(); child++) {
		emit_statement (node->children[child]);
	}
}

void emit_vardecl (astree *node) {
	astree *ident = get_ident (node->children[0]);
	astree *expr = node->children[1];
	string expr_str = emit_expr (expr);
	const string *name = ident->lexinfo;
	type_pair i_type = {ident->type.first, ident->attributes};
	string type = get_type (i_type);
	if (ident->blocknr == 0) {
		fprintf (oil_file, "        __%s", name->c_str());
	} else {
		fprintf (oil_file, "        %s _%ld_%s", type.c_str(),
			ident->blocknr, name->c_str());
	}
	fprintf (oil_file, " = %s;\n", expr_str.c_str());

}

void emit_statement (astree *node) {
	switch (node->symbol) {
		case TOK_BLOCK:
			emit_block (node, &emit_statement);
			break;
		case TOK_VARDECL:
			emit_vardecl (node);
			break;
		case TOK_WHILE:
			break;
		case TOK_IF:
			break;
		case TOK_IFELSE:
			break;
		case TOK_RETURN:
			break;
		case TOK_RETURNVOID:
			break;
		default:
			emit_expr (node);
			break;
	}
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
	emit_block (block, &emit_statement);
	fprintf (oil_file, "}\n");
}

void emit_queue (void (*emit)(astree*), vector<astree*> queue) {
	for (size_t i = 0; i < queue.size(); i++) {
		emit (queue[i]);
	}
}

void emit_main (astree *node) {
	for (size_t child = 0; child < node->children.size(); child++) {
		int sym = node->children[child]->symbol;
		if ((sym == TOK_STRUCT) | (sym == TOK_FUNCTION)
			| (sym == TOK_PROTOTYPE)) continue;
		emit_statement (node->children[child]);
	}
}

void emit_code (FILE *out) {
	oil_file = out;
	emit_queue (&emit_struct, struct_queue);
	emit_queue (&emit_sconst, sconst_queue);
	emit_queue (&emit_gvar, gvar_queue);
	emit_queue (&emit_func, func_queue);
	fprintf (oil_file, "void __ocmain (void)\n{\n");
	emit_main (yyparse_astree);
	fprintf (oil_file, "}\n");
}
