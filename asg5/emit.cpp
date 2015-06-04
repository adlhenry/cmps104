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

void emit_block (astree *node);
string emit_expr (astree *node);
void emit_statement (astree *node);

FILE *oil_file = NULL;
size_t register_number = 1;
unordered_map<const string*,size_t> sconst_register;

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
	if (type.second[ATTR_typeid]) reg = "p";
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
	size_t reg = register_number++;
	sconst_register[node->lexinfo] = reg;
	fprintf (oil_file, "char* s%ld = %s;\n", reg, s_name->c_str());
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
	emit_block (block);
	fprintf (oil_file, "}\n");
}

void emit_block (astree *node) {
	for (size_t child = 0; child < node->children.size(); child++) {
		emit_statement (node->children[child]);
	}
}

void emit_vardecl (astree *node) {
	astree *ident = get_ident (node->children[0]);
	string expr = emit_expr (node->children[1]);
	const string *name = ident->lexinfo;
	type_pair i_type = {ident->type.first, ident->attributes};
	string type = get_type (i_type);
	if (ident->blocknr == 0) {
		fprintf (oil_file, "        __%s", name->c_str());
	} else {
		fprintf (oil_file, "        %s _%ld_%s", type.c_str(),
			ident->blocknr, name->c_str());
	}
	fprintf (oil_file, " = %s;\n", expr.c_str());
}

void emit_while (astree *node) {
	fprintf (oil_file, "while_%ld_%ld_%ld:;\n",
		node->filenr, node->linenr, node->offset);
	string expr = emit_expr (node->children[0]);
	fprintf (oil_file, "        if (!%s) goto break_%ld_%ld_%ld;\n",
		expr.c_str(), node->filenr, node->linenr, node->offset);
	emit_statement (node->children[1]);
	fprintf (oil_file, "        goto while_%ld_%ld_%ld;\n",
		node->filenr, node->linenr, node->offset);
	fprintf (oil_file, "break_%ld_%ld_%ld:;\n",
		node->filenr, node->linenr, node->offset);
}

void emit_if (astree *node) {
	string expr = emit_expr (node->children[0]);
	fprintf (oil_file, "        if (!%s) goto fi_%ld_%ld_%ld;\n",
		expr.c_str(), node->filenr, node->linenr, node->offset);
	emit_statement (node->children[1]);
	fprintf (oil_file, "fi_%ld_%ld_%ld:;\n",
		node->filenr, node->linenr, node->offset);
}

void emit_ifelse (astree *node) {
	string expr = emit_expr (node->children[0]);
	fprintf (oil_file, "        if (!%s) goto else_%ld_%ld_%ld;\n",
		expr.c_str(), node->filenr, node->linenr, node->offset);
	emit_statement (node->children[1]);
	fprintf (oil_file, "        goto fi_%ld_%ld_%ld;\n",
		node->filenr, node->linenr, node->offset);
	fprintf (oil_file, "else_%ld_%ld_%ld:;\n",
		node->filenr, node->linenr, node->offset);
	emit_statement (node->children[2]);
	fprintf (oil_file, "fi_%ld_%ld_%ld:;\n",
		node->filenr, node->linenr, node->offset);
}

void emit_return (astree *node) {
	string expr = emit_expr (node->children[0]);
	fprintf (oil_file, "        return %s;\n", expr.c_str());
}

void emit_returnvoid () {
	fprintf (oil_file, "        return;\n");
}

void emit_asign (astree *node) {
	string expr1 = emit_expr (node->children[0]);
	string expr2 = emit_expr (node->children[1]);
	fprintf (oil_file, "        %s = %s;\n", expr1.c_str(),
		expr2.c_str());
}

string emit_binop (astree *node) {
	string expr1 = emit_expr (node->children[0]);
	string expr2 = emit_expr (node->children[1]);
	type_pair b_type = {node->type.first, node->attributes};
	string type = get_type (b_type);
	string reg = get_register (b_type);
	fprintf (oil_file, "        %s %s = %s %s %s;\n", type.c_str(),
		reg.c_str(), expr1.c_str(), node->lexinfo->c_str(),
		expr2.c_str());
	return reg;
}

string emit_unop (astree *node, string unop) {
	string expr1 = emit_expr (node->children[0]);
	type_pair u_type = {node->type.first, node->attributes};
	string type = get_type (u_type);
	string reg = get_register (u_type);
	fprintf (oil_file, "        %s %s = %s%s;\n", type.c_str(),
		reg.c_str(), unop.c_str(), expr1.c_str());
	return reg;
}

string emit_new (astree *node, string expr) {
	type_pair type = {node->type.first, node->attributes};
	string type1 = get_type (type);
	string type2 = type1.substr (0, type1.length() - 1);
	string reg = get_register (type);
	fprintf (oil_file, "        %s %s = xcalloc (%s, sizeof (%s));\n",
		type1.c_str(), reg.c_str(), expr.c_str(), type2.c_str());
	return reg;
}

string emit_call (astree *node) {
	string reg = "";
	string ident = *node->children[0]->lexinfo;
	vector<string> argreg;
	for (size_t child = 1; child < node->children.size(); child++) {
		argreg.push_back (emit_expr (node->children[child]));
	}
	if (node->attributes[ATTR_void]) {
		fprintf (oil_file, "        __%s (", ident.c_str());
	} else {
		type_pair c_type = {node->type.first, node->attributes};
		string type = get_type (c_type);
		reg = get_register (c_type);
		fprintf (oil_file, "        %s %s = __%s (",
			type.c_str(), reg.c_str(), ident.c_str());
	}
	for (size_t arg = 0; arg < argreg.size(); arg++) {
		fprintf (oil_file, "%s", argreg[arg].c_str());
		if (arg < argreg.size() - 1) {
			fprintf (oil_file, ", ");
		}
	}
	fprintf (oil_file, ");\n");
	return reg;
}

string emit_ident (astree *node) {
	size_t blocknr = node->blocknr;
	symbol *sym = node->type.second;
	if (sym != NULL) blocknr = sym->blocknr;
	string expr = "_";
	if (blocknr != 0) expr += to_string (blocknr);
	expr += "_";
	return expr += *node->lexinfo;
}

string emit_index (astree *node) {
	string expr1 = emit_expr (node->children[0]);
	string expr2 = emit_expr (node->children[1]);
	type_pair i_type = {node->type.first, node->attributes};
	string type = get_type (i_type);
	string reg = get_register (i_type);
	fprintf (oil_file, "        %s* %s = &%s[%s];\n",
		type.c_str(), reg.c_str(), expr1.c_str(), expr2.c_str());
	return string ("*") + reg;
}

string emit_select (astree *node) {
	string expr = emit_expr (node->children[0]);
	string s_name = *node->children[0]->type.first;
	string field = *node->children[1]->lexinfo;
	type_pair f_type = {node->type.first, node->attributes};
	string type = get_type (f_type);
	string reg = get_register (f_type);
	fprintf (oil_file, "        %s* %s = &%s->f_%s_%s;\n",
		type.c_str(), reg.c_str(), expr.c_str(),
		s_name.c_str(), field.c_str());
	return string ("*") + reg;
}

string emit_const (astree *node) {
	string expr = *node->lexinfo;
	if (node->attributes[ATTR_int]) {
		size_t pos = expr.find_first_not_of ("0", 0);
		if (pos != string::npos) {
			expr = expr.substr (pos);
		} else {
			expr = "0";
		}
	}
	if (node->attributes[ATTR_string]) {
		expr = "s";
		expr += to_string (sconst_register[node->lexinfo]);
	}
	if (node->attributes[ATTR_null]) expr = to_string (0);
	if (node->attributes[ATTR_bool]) {
		if (expr == "false") {
			expr = to_string (0);
		} else {
			expr = to_string (1);
		}
	}
	return expr;
}

string emit_expr (astree *node) {
	string expr = "";
	int sym = node->symbol;
	if (sym == '=') emit_asign (node);
	if ((sym == '+') | (sym == '-') | (sym == '*') | (sym == '/')
		| (sym == '%') | (sym == TOK_EQ) | (sym == TOK_NE)
		| (sym == TOK_LT) | (sym == TOK_LE)| (sym == TOK_GT)
		| (sym == TOK_GE)) {
		expr = emit_binop (node);
	}
	if ((sym == TOK_POS) | (sym == TOK_NEG) | (sym == '!')) {
			expr = emit_unop (node, *node->lexinfo);
	}
	if (sym == TOK_ORD) expr = emit_unop (node, "(int)");
	if (sym == TOK_CHR) expr = emit_unop (node, "(char)");
	if (sym == TOK_NEW) expr = emit_new (node, "1");
	if (sym == TOK_NEWSTRING) {
		expr = emit_new (node, emit_expr (node->children[0]));
	}
	if (sym == TOK_NEWARRAY) {
		expr = emit_new (node, emit_expr (node->children[1]));
	}
	if (sym == TOK_CALL) expr = emit_call (node);
	if (sym == TOK_IDENT) expr = emit_ident (node);
	if (sym == TOK_INDEX) expr = emit_index (node);
	if (sym == '.') expr = emit_select (node);
	if (node->attributes[ATTR_const]) {
		expr = emit_const (node);
	}
	return expr;
}

void emit_statement (astree *node) {
	switch (node->symbol) {
		case TOK_BLOCK:
			emit_block (node);
			break;
		case TOK_VARDECL:
			emit_vardecl (node);
			break;
		case TOK_WHILE:
			emit_while (node);
			break;
		case TOK_IF:
			emit_if (node);
			break;
		case TOK_IFELSE:
			emit_ifelse (node);
			break;
		case TOK_RETURN:
			emit_return (node);
			break;
		case TOK_RETURNVOID:
			emit_returnvoid ();
			break;
		default:
			emit_expr (node);
			break;
	}
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
