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

symbol_table *structs = new symbol_table();
vector<symbol_table*> idents;
symbol *proto = NULL;

vector<symbol_table*> symbol_stack {NULL};
vector<size_t> block_stack {0};
size_t next_block = 1, depth = 0;
int need_line = 0;

FILE *out = NULL;

const char *attr_string[] = { "void", "bool", "char", "int", "null",
	"string", "", "array", "function", "variable",
	"field", "", "param", "lval", "const",
	"vreg", "vaddr"
};

unordered_map<int,int> attr_type = {{TOK_VOID, ATTR_void}, {TOK_BOOL, ATTR_bool},
	{TOK_CHAR, ATTR_char}, {TOK_INT, ATTR_int}, {TOK_STRING, ATTR_string},
	{TOK_TYPEID, ATTR_typeid}
};

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

void attr_print (const string *type_name, attr_bitset attributes) {
	int need_space = 0;
	for (size_t attr = 0; attr < attributes.size(); attr++) {
		if (attributes[attr]) {
			if (need_space) fprintf (out, " ");
			fprintf (out, "%s", attr_string[attr]);
			if (attr == ATTR_typeid && type_name != NULL) {
				fprintf (out, "struct \"%s\"", type_name->c_str());
			}
			need_space = 1;
		}
	}
}

void sym_print (const string *name, symbol *sym) {
	if (sym->blocknr == 0 && !sym->attributes[ATTR_field]) {
		if (need_line) printf("\n");
		need_line = 1;
	}
	for (size_t i = 0; i < depth; i++) {
		fprintf (out, "   ");
	}
	fprintf (out, "%s (%ld.%ld.%ld) {%ld} ", name->c_str(), sym->filenr,
			sym->linenr, sym->offset, sym->blocknr);
	attr_print (sym->type_name, sym->attributes);
	fprintf (out, "\n");
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
			&& !(*table)[key]->attributes[ATTR_function]
			&& !(*table)[key]->attributes[ATTR_variable]) {
				proto = (*table)[key];
				sym_print (key, val);
			} else {
				err_print (key, val, 'i');
			}
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
	if (attributes[ATTR_variable] | attributes[ATTR_param]) {
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
			fields->insert (entry);
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
	proto->attributes[ATTR_function] = 1;
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
		for (size_t child = 0; child < param->children.size(); child++) {
			ent = define_ident (param->children[child], ATTR_param);
			last->parameters = ent.second;
			last = last->parameters;
		}
	}
	if (attr == ATTR_function) {
		astree *block = node->children[2];
		for (size_t child = 0; child < block->children.size(); child++) {
			astree *stmt = block->children[child];
			if (stmt->symbol == TOK_VARDECL) fprintf (out, "\n");
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
				
				symbol *val = (*table)[key];
				node->blocknr = block_stack.back();
				node->attributes = val->attributes;
				node->type = {val->type_name, val};
				
			}
		}
	}
	if (!defined) err_print (key, node, 'u');
}

static int scan_node (astree *node) {
	int block = 0;
	switch (node->symbol) {
		case TOK_STRUCT:
			define_struct (node);
			break;
		case TOK_BLOCK:
			block = 1;
			new_block();
			node->blocknr = block_stack.back();
			break;
		case TOK_FUNCTION:
			block = 1;
			define_func (node, ATTR_function);
			break;
		case TOK_PROTOTYPE:
			block = 1;
			define_func (node, 0);
			break;
		case TOK_VARDECL:
			define_ident (node->children[0], ATTR_variable);
			break;
		case TOK_IDENT:
			ref_ident (node);
			break;
		case TOK_NEW:
			typeid_check (node->children[0], 0);
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

void dump_symtable (FILE *sym_file) {
	out = sym_file;
	scan_astree (yyparse_astree);
}
