// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: stringset.h,v 1.1 2015-05-22 15:22:23-07 - - $

#ifndef __STRINGSET__
#define __STRINGSET__

#include <string>
#include <unordered_set>
using namespace std;

#include <stdio.h>

const string *intern_stringset (const char*);

void dump_stringset (FILE*);

#endif
