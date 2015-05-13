// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: symtable.h,v 1.1 2015-04-09 18:58:56-07 - - $

#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <string>
#include <bitset>
#include <unordered_map>
#include <utility>
using namespace std;

#include "auxlib.h"

struct symbol;

enum { ATTR_void, ATTR_bool, ATTR_char, ATTR_int, ATTR_null,
	ATTR_string, ATTR_struct, ATTR_array, ATTR_function,
	ATTR_prototype, ATTR_variable, ATTR_field, ATTR_typeid,
	ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,
	ATTR_bitset_size
};

using attr_bitset = bitset<ATTR_bitset_size>;
using symbol_table = unordered_map<const string*,symbol*>;
using symbol_entry = pair<const string*,symbol*>;

struct symbol {
	attr_bitset attributes;
	symbol_table *fields;
	size_t filenr, linenr, offset;
	size_t blocknr;
	symbol *parameters;
	const string *type_name;
};

symbol *get_struct (const string *key);
const char *get_attrstring (const string *type_name,
	attr_bitset attributes);
void dump_symtable (FILE *sym_file);

#endif
