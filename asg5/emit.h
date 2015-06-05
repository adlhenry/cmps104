// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: emit.h,v 1.1 2015-05-22 15:22:23-07 - - $

#ifndef __EMIT_H__
#define __EMIT_H__

#include <string>
#include <bitset>
#include <unordered_map>
#include <utility>
using namespace std;

void struct_queue_add (astree *node);
void sconst_queue_add (astree *node);
void gvar_queue_add (astree *node);
void proto_queue_add (astree *node);
void func_queue_add (astree *node);
void emit_code (FILE *out);

#endif
