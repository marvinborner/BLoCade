// Copyright (c) 2023, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#ifndef BLOC_PARSE_H
#define BLOC_PARSE_H

#include <stddef.h>

#include <term.h>
#include <spec.h>

struct bloc_parsed {
	size_t length;
	struct term **entries;
};

struct bloc_parsed *parse_bloc(const void *bloc);
void free_bloc(struct bloc_parsed *bloc);

#endif
