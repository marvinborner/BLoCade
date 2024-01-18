// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <term.h>
#include <spec.h>
#include <parse.h>
#include <log.h>

#define BIT_AT(i) ((term[(i) / 8] & (1 << (7 - ((i) % 8)))) >> (7 - ((i) % 8)))

// parses bloc's bit-encoded blc
// 010M -> abstraction of M
// 00MN -> application of M and N
// 1X0 -> bruijn index, amount of 1s in X
// 011I -> 2B index to entry
static struct term *parse_bloc_bblc(const char *term, size_t length,
				    size_t *bit)
{
	struct term *res = 0;
	if (!BIT_AT(*bit) && BIT_AT(*bit + 1) && !BIT_AT(*bit + 2)) {
		(*bit) += 3;
		res = new_term(ABS);
		res->u.abs.term = parse_bloc_bblc(term, length, bit);
	} else if (!BIT_AT(*bit) && !BIT_AT(*bit + 1)) {
		(*bit) += 2;
		res = new_term(APP);
		res->u.app.lhs = parse_bloc_bblc(term, length, bit);
		res->u.app.rhs = parse_bloc_bblc(term, length, bit);
	} else if (BIT_AT(*bit)) {
		const size_t cur = *bit;
		while (BIT_AT(*bit))
			(*bit)++;
		res = new_term(VAR);
		res->u.var.index = *bit - cur - 1;
		(*bit)++;
	} else if (!BIT_AT(*bit) && BIT_AT(*bit + 1) && BIT_AT(*bit + 2)) {
		(*bit) += 3;

		// selected bit pattern, see readme
		int sel = (2 << (BIT_AT(*bit) * 2 + BIT_AT(*bit + 1) + 2));
		(*bit) += 2;

		res = new_term(REF);
		size_t index = 0;
		for (int i = 0; i < sel; i++) {
			index |= BIT_AT(*bit) << i;
			(*bit) += 1;
		}

		// normalize to entry index
		res->u.ref.index = length - index - 2;
	} else {
		(*bit)++;
		res = parse_bloc_bblc(term, length, bit);
	}
	return res;
}

struct bloc_parsed *parse_bloc(const void *bloc)
{
	const struct bloc_header *header = bloc;
	if (memcmp(header->identifier, BLOC_IDENTIFIER,
		   (size_t)BLOC_IDENTIFIER_LENGTH)) {
		fatal("invalid BLoC identifier!\n");
		return 0;
	}

	struct bloc_parsed *parsed = malloc(sizeof(*parsed));
	parsed->length = header->length;
	parsed->entries = malloc(header->length * sizeof(struct term *));

	const struct bloc_entry *current = (const void *)&header->entries;
	for (size_t i = 0; i < parsed->length; i++) {
		size_t len = 0;
		parsed->entries[i] = parse_bloc_bblc((const char *)current,
						     parsed->length, &len);
		current =
			(const struct bloc_entry *)(((const char *)current) +
						    (len / 8) + (len % 8 != 0));
	}

	return parsed;
}

void free_bloc(struct bloc_parsed *bloc)
{
	for (size_t i = 0; i < bloc->length; i++) {
		free_term(bloc->entries[i]);
	}

	free(bloc->entries);
	free(bloc);
}
