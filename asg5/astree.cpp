// Author: Adam Henry, adlhenry@ucsc.edu
// $Id: astree.cpp,v 1.1 2015-05-22 15:22:23-07 - - $

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "stringset.h"
#include "astree.h"

astree *new_astree (int symbol, int filenr, int linenr, int offset,
					const char *lexinfo) {
	astree *tree = new astree();
	tree->symbol = symbol;
	tree->filenr = filenr;
	tree->linenr = linenr;
	tree->offset = offset;
	tree->lexinfo = intern_stringset (lexinfo);
	tree->blocknr = 0;
	tree->attributes = 0;
	tree->type = {NULL, NULL};
	DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
			tree, tree->filenr, tree->linenr, tree->offset,
			get_yytname (tree->symbol), tree->lexinfo->c_str());
	return tree;
}

astree *adopt1 (astree *root, astree *child) {
	root->children.push_back (child);
	DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
			root, root->lexinfo->c_str(),
			child, child->lexinfo->c_str());
	return root;
}

astree *adopt2 (astree *root, astree *left, astree *right) {
	adopt1 (root, left);
	adopt1 (root, right);
	return root;
}

astree *adopt1sym (astree *root, astree *child, int symbol) {
	root = adopt1 (root, child);
	root->symbol = symbol;
	return root;
}

astree *adopt3fn (astree *child1, astree *child2, astree *child3) {
	astree *root = NULL;
	if (child3->symbol == ';') {
		root = new_astree (TOK_PROTOTYPE, child1->filenr,
			child1->linenr, child1->offset, "<<PROTOTYPE>>");
		adopt1 (root, child1);
		adopt1 (root, child2);
	} else {
		root = new_astree (TOK_FUNCTION, child1->filenr,
			child1->linenr, child1->offset, "<<FUNCTION>>");
		adopt1 (root, child1);
		adopt1 (root, child2);
		adopt1 (root, child3);
	}
	return root;
}

astree *change_sym (astree *root, int symbol) {
	root->symbol = symbol;
	return root;
}

static void dump_node (FILE *outfile, astree *node) {
	const char *tname = get_yytname (node->symbol);
	if (strstr (tname, "TOK_") == tname) tname += 4;
	fprintf (outfile, "%s \"%s\" (%ld.%ld.%ld) {%ld} %s", tname,
		node->lexinfo->c_str(), node->filenr, node->linenr,
		node->offset, node->blocknr, get_attrstring (node->type.first,
		node->attributes));
	if (node->symbol == TOK_IDENT) {
		symbol *sym = node->type.second;
		if (sym != NULL) {
			fprintf (outfile, " (%ld.%ld.%ld)", 
				sym->filenr, sym->linenr, sym->offset);
		} else {
			fprintf (outfile, "(not declared)");
		}
	}
}

static void dump_astree_rec (FILE *outfile, astree *root, int depth) {
	if (root == NULL) return;
	for (int i = 0; i < depth * 3; i++) {
		if (i % 3 == 0) {
			fprintf (outfile, "|");
		} else {
			fprintf (outfile, " ");
		}
	}
	dump_node (outfile, root);
	fprintf (outfile, "\n");
	for (size_t child = 0; child < root->children.size(); ++child) {
		dump_astree_rec (outfile, root->children[child], depth + 1);
	}
}

void dump_astree (FILE *outfile, astree *root) {
	dump_astree_rec (outfile, root, 0);
	fflush (NULL);
}

void yyprint (FILE *outfile, unsigned short toknum, astree *yyvaluep) {
	DEBUGF ('f', "toknum = %d, yyvaluep = %p\n", toknum, yyvaluep);
	if (is_defined_token (toknum)) {
		dump_node (outfile, yyvaluep);
	}else {
		fprintf (outfile, "%s(%d)\n", get_yytname (toknum), toknum);
	}
	fflush (NULL);
}

void free_ast (astree *root) {
	while (not root->children.empty()) {
		astree *child = root->children.back();
		root->children.pop_back();
		free_ast (child);
	}
	DEBUGF ('f', "free [%X]-> %d:%d.%d: %s: \"%s\")\n",
			(uintptr_t) root, root->filenr, root->linenr, root->offset,
			get_yytname (root->symbol), root->lexinfo->c_str());
	delete root;
}

void free_ast2 (astree *tree1, astree *tree2) {
	free_ast (tree1);
	free_ast (tree2);
}
