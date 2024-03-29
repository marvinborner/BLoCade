// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#ifndef BLOCADE_TERM_H
#define BLOCADE_TERM_H

#include <stddef.h>

typedef enum { INV, ABS, APP, VAR, REF } term_type;

struct term {
	term_type type;
	union {
		struct {
			struct term *term;
		} abs;
		struct {
			struct term *lhs;
			struct term *rhs;
		} app;
		struct {
			int index;
		} var;
		struct {
			size_t index;
		} ref;
	} u;
	unsigned int meta; // arbitrary field for targets
};

struct term *new_term(term_type type);
void free_term(struct term *term);

#endif
