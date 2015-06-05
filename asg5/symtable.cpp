// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: symtable.cpp,v 1.1 2015-05-22 15:22:23-07 - - $

#include <string>
#include <vector>
using namespace std;

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lyutils.h"
#include "astree.h"
#include "symtable.h"
#include "typecheck.h"
#include "emit.h"

symbol_table *structs = new symbol_table();
vector<symbol_table*> idents;
symbol *proto = NULL;

vector<symbol_table*> symbol_stack {NULL};
vector<size_t> block_stack {0};
size_t next_block = 1, depth = 0;
int need_line = 0;

FILE *out = NULL;

const char *attr_string[] = { "void", "bool", "char", "int", "null",
	"string", "struct", "array", "function", "prototype", "variable",
	"field", "", "param", "lval", "const",
	"vreg", "vaddr"
};

unordered_map<int,int> attr_type = {{TOK_VOID, ATTR_void},
	{TOK_BOOL, ATTR_bool}, {TOK_CHAR, ATTR_char}, {TOK_INT, ATTR_int},
	{TOK_STRING, ATTR_string}, {TOK_TYPEID, ATTR_typeid}
};

unordered_map<int,vector<int>> sym_attrs = {
	{'=', {ATTR_vreg}},
	{TOK_EQ, {ATTR_bool, ATTR_vreg}},
	{TOK_NE, {ATTR_bool, ATTR_vreg}},
	{TOK_LT, {ATTR_bool, ATTR_vreg}},
	{TOK_LE, {ATTR_bool, ATTR_vreg}},
	{TOK_GT, {ATTR_bool, ATTR_vreg}},
	{TOK_GE, {ATTR_bool, ATTR_vreg}},
	{'+', {ATTR_int, ATTR_vreg}},
	{'-', {ATTR_int, ATTR_vreg}},
	{'*', {ATTR_int, ATTR_vreg}},
	{'/', {ATTR_int, ATTR_vreg}},
	{'%', {ATTR_int, ATTR_vreg}},
	{TOK_POS, {ATTR_int, ATTR_vreg}},
	{TOK_NEG, {ATTR_int, ATTR_vreg}},
	{'!', {ATTR_bool, ATTR_vreg}},
	{TOK_ORD, {ATTR_int, ATTR_vreg}},
	{TOK_CHR, {ATTR_char, ATTR_vreg}},
	{TOK_NEW, {ATTR_vreg}},
	{TOK_NEWSTRING, {ATTR_string, ATTR_vreg}},
	{TOK_NEWARRAY, {ATTR_array, ATTR_vreg}},
	{TOK_CALL, {ATTR_vreg}},
	{TOK_INDEX, {ATTR_vaddr, ATTR_lval}},
	{'.', {ATTR_vaddr, ATTR_lval}},
	{TOK_INTCON, {ATTR_int, ATTR_const}},
	{TOK_CHARCON, {ATTR_char, ATTR_const}},
	{TOK_STRINGCON, {ATTR_string, ATTR_const}},
	{TOK_FALSE, {ATTR_bool, ATTR_const}},
	{TOK_TRUE, {ATTR_bool, ATTR_const}},
	{TOK_NULL, {ATTR_null, ATTR_const}}
};

symbol *get_struct (const string *key) {
	return (*structs)[key];
}

attr_bitset get_attrs (int symbol) {
	attr_bitset attributes = 0;
	vector<int> attrs = sym_attrs[symbol];
	for (size_t i = 0; i < attrs.size(); i++) {
		attributes[attrs[i]] = 1;
	}
	return attributes;
}

void set_ast_node (astree *node, symbol *val) {
	if (val != NULL) {
		node->blocknr = block_stack.back();
		node->attributes = val->attributes;
		node->type = {val->type_name, val};
	} else {
		if (node->attributes != 0) return;
		node->blocknr = block_stack.back();
		node->attributes = get_attrs (node->symbol);
	}
}

void set_values (symbol *sym, astree *node) {
	sym->filenr = node->filenr;
	sym->linenr = node->linenr;
	sym->offset = node->offset;
	sym->blocknr = block_stack.back();
}

symbol *new_symbol (astree *node) {
	symbol *sym = new symbol();
	sym->attributes = 0;
	set_values (sym, node);
	sym->fields = NULL;
	sym->parameters = NULL;
	sym->type_name = NULL;
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
	idents.push_back (symbol_stack.back());
	symbol_stack.pop_back();
}

string get_attrstring (const string *type_name,
	attr_bitset attributes) {
	int need_space = 0;
	string attrstring = "";
	for (size_t attr = 0; attr < attributes.size(); attr++) {
		if (attributes[attr]
			&& attr != ATTR_struct
			&& attr != ATTR_prototype) {
			if (need_space) attrstring += " ";
			attrstring += attr_string[attr];
			if (attr == ATTR_typeid && type_name != NULL) {
				attrstring += "struct \""
					+ string (type_name->c_str()) + "\"";
			}
			need_space = 1;
		}
	}
	return attrstring;
}

void sym_print (const string *name, symbol *sym) {
	if (sym->blocknr == 0 && !sym->attributes[ATTR_field]) {
		if (need_line) fprintf (out, "\n");
		need_line = 1;
	}
	for (size_t i = 0; i < depth; i++) {
		fprintf (out, "   ");
	}
	string attrs = get_attrstring (sym->type_name, sym->attributes);
	fprintf (out, "%s (%ld.%ld.%ld) {%ld} %s\n", name->c_str(),
		sym->filenr, sym->linenr, sym->offset, sym->blocknr,
		attrs.c_str());
}

template <typename T>
void err_print (const string *key, T *val, char err) {
	string error = "%: ";
	switch (err) {
		case 'f':
			error += "declared function differs from prototype";
			break;
		case 'i':
			error += "identifier previously declared";
			break;
		case 't':
			error += "reference to incomplete type";
			break;
		case 'u':
			error += "reference to undeclared identifier";
			break;
	}
	error += ": %s (%ld.%ld.%ld)\n";
	errprintf (error.c_str(), key->c_str(), 
		val->filenr, val->linenr, val->offset);
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
			if (val->attributes[ATTR_function]
			&& (*table)[key]->attributes[ATTR_prototype]) {
				proto = (*table)[key];
				sym_print (key, val);
			} else {
				err_print (key, val, 'i');
			}
			table = new symbol_table();
			(*table)[key] = val;
			idents.push_back (table);
		} else {
			(*table)[key] = val;
			sym_print (key, val);
		}
	}
}

symbol_entry typeid_check (astree *type, attr_bitset attributes) {
	const string *key = type->lexinfo;
	if ((*structs)[key] == NULL) {
		symbol *val = new_symbol (type);
		(*structs)[key] = val;
	}
	if ((*structs)[key]->fields == NULL) {
		if (!attributes[ATTR_field]) {
			err_print (key, type, 't');
		}
	}
	return {key, (*structs)[key]};
}

symbol_entry define_ident (astree *type, int attr) {
	attr_bitset attributes = 0;
	if (attr) attributes[attr] = 1;
	if (attr == ATTR_prototype) attributes[ATTR_function] = 1;
	if (attributes[ATTR_variable] | attributes[ATTR_param]) {
		attributes[ATTR_variable] = 1;
		attributes[ATTR_lval] = 1;
	}
	astree *ident = type->children[0];
	if (type->symbol == TOK_ARRAY) {
		attributes[ATTR_array] = 1;
		ident = type->children[1];
		type = type->children[0];
	}
	attributes[attr_type[type->symbol]] = 1;
	if (attributes[ATTR_typeid]) {
		ident->type = typeid_check (type, attributes);
	}
	const string *key = ident->lexinfo;
	symbol *val = new_symbol (ident);
	val->attributes = attributes;
	val->type_name = ident->type.first;
	ident->blocknr = block_stack.back();
	ident->attributes = val->attributes;
	if (!attributes[ATTR_field]) {
		intern_symtable (key, val);
	}
	return {key, val};
}

void define_struct (astree *node) {
	attr_bitset attributes = 0;
	attributes[ATTR_struct] = 1;
	attributes[ATTR_typeid] = 1;
	astree *type_id = node->children[0];
	const string *key = type_id->lexinfo;
	
	symbol *val = (*structs)[key];
	if (val == NULL) {
		val = new_symbol (type_id);
		(*structs)[key] = val;
	} else if (val->fields == NULL) {
		set_values (val, type_id);
	}
	if (val->fields != NULL) {
		err_print (key, type_id, 'i');
		return;
	}
	val->attributes = attributes;
	val->type_name = key;
	set_ast_node (type_id, val);
	sym_print (key, val);
	symbol_table *fields = new symbol_table();
	val->fields = fields;
	depth++;
	for (size_t child = 1; child < node->children.size(); child++) {
		symbol_entry entry = 
			define_ident (node->children[child], ATTR_field);
		key = entry.first;
		val = entry.second;
		if ((*fields)[key] == NULL) {
			(*fields)[key] = val;
			sym_print (key, val);
		} else {
			err_print (key, val, 'i');
		}
	}
	depth--;
}

void proto_check (const string *key, symbol *last, astree *param) {
	symbol *p_param = proto->parameters;
	symbol_table *p_table = NULL;
	if (p_param != NULL) {
		for (size_t i = 0; i < idents.size(); i++) {
			symbol_table *table = idents[i];
			for (auto it = table->begin(); it != table->end(); it++) {
				if (it->second == proto->parameters) {
					p_table = table;
				}
			}
		}
	}
	for (size_t child = 0; child < param->children.size(); child++) {
		symbol_entry ent =
			define_ident (param->children[child], ATTR_param);
		last->parameters = ent.second;
		last = last->parameters;
		if (p_param != NULL) {
			if (((*p_table)[ent.first] != p_param)
			| (p_param->attributes != last->attributes)) {
				err_print (key, last, 'f');
				break;
			}
			p_param = p_param->parameters;
		} else {
			err_print (key, last, 'f');
			break;
		}
	}
	if (p_param != NULL) err_print (key, last, 'f');
	proto->attributes[ATTR_prototype] = 0;
	proto = NULL;
}

void define_func (astree *node, int attr) {
	symbol_entry ent = define_ident (node->children[0], attr);
	symbol *last = ent.second;
	new_block();
	astree *param = node->children[1];
	if (proto != NULL) {
		proto_check (ent.first, last, param);
	} else {
		for (size_t child = 0; child < param->children.size();
			child++) {
			ent = define_ident (param->children[child], ATTR_param);
			last->parameters = ent.second;
			last = last->parameters;
		}
	}
	if (attr == ATTR_prototype) return;
	astree *block = node->children[2];
	block->blocknr = block_stack.back();
	for (size_t child = 0; child < block->children.size(); child++) {
		astree *stmt = block->children[child];
		if (stmt->symbol == TOK_VARDECL) {
			fprintf (out, "\n");
			break;
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
				set_ast_node (node, (*table)[key]);
			}
		}
	}
	if (!defined) {
		node->blocknr = block_stack.back();
		err_print (key, node, 'u');
	}
}

void ref_new (astree *node) {
	set_ast_node (node, NULL);
	astree *type = node->children[0];
	typeid_check (type, 0);
	node->attributes[ATTR_typeid] = 1;
	node->type.first = type->lexinfo;
}

void ref_newarray (astree *node) {
	set_ast_node (node, NULL);
	astree *type = node->children[0];
	if (type->symbol == TOK_TYPEID) {
		typeid_check (type, 0);
	}
}

static int define (astree *node) {
	int block = 0;
	switch (node->symbol) {
		case TOK_STRUCT:
			define_struct (node);
			struct_queue_add (node);
			break;
		case TOK_BLOCK:
			if (node->blocknr != 0) break;
			block = 1;
			new_block();
			node->blocknr = block_stack.back();
			break;
		case TOK_FUNCTION:
			block = 1;
			define_func (node, ATTR_function);
			func_queue_add (node);
			break;
		case TOK_PROTOTYPE:
			block = 1;
			define_func (node, ATTR_prototype);
			proto_queue_add (node);
			break;
		case TOK_VARDECL:
			set_ast_node (node, NULL);
			define_ident (node->children[0], ATTR_variable);
			gvar_queue_add (node);
			break;
		case TOK_IDENT:
			ref_ident (node);
			break;
		case TOK_NEW:
			ref_new (node);
			break;
		case TOK_NEWARRAY:
			ref_newarray (node);
			break;
		case TOK_STRINGCON:
			set_ast_node (node, NULL);
			sconst_queue_add (node);
			break;
		default:
			set_ast_node (node, NULL);
			break;
	}
	return block;
}

static void scan_astree (astree *root) {
	if (root == NULL) return;
	int block = define (root);
	for (size_t child = 0; child < root->children.size(); child++) {
		scan_astree (root->children[child]);
	}
	if (block) exit_block();
	type_check (root);
}

void dump_symtable (FILE *sym_file) {
	out = sym_file;
	scan_astree (yyparse_astree);
	idents.push_back (symbol_stack.back());
	symbol_stack.pop_back();
}

void free_table (symbol_table *table) {
	for (auto it : (*table)) {
		symbol *sym = it.second;
		if (sym != NULL) {
			if (sym->fields != NULL) {
				free_table (sym->fields);
			}
			delete sym;
		}
	}
	delete table;
}

void free_symtable () {
	for (size_t i = 0; i < idents.size(); i++) {
		if (idents[i] != NULL) {
			free_table (idents[i]);
		}
	}
	free_table (structs);
}
