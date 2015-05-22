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

FILE *oil_file = NULL;

void emit_code (FILE *out) {
	oil_file = out;
}
