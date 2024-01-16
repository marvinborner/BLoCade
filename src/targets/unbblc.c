// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <target.h>
#include <parse.h>
#include <log.h>

static void write_bit(char val, FILE *file, char *byte, int *bit)
{
	if (*bit > 7) { // flush byte
		fwrite(byte, 1, 1, file);
		*byte = 0;
		*bit = 0;
	}

	// TODO: which endianness should be default?
	if (val)
		*byte |= 1UL << *bit;
	/* *byte |= 1UL << (7 - *bit); */
	(*bit)++;
}

static void fprint_unbblc(struct term *term, struct bloc_parsed *bloc,
			  FILE *file, char *byte, int *bit)
{
	switch (term->type) {
	case ABS:
		write_bit(0, file, byte, bit);
		write_bit(0, file, byte, bit);
		fprint_unbblc(term->u.abs.term, bloc, file, byte, bit);
		break;
	case APP:
		write_bit(0, file, byte, bit);
		write_bit(1, file, byte, bit);
		fprint_unbblc(term->u.app.lhs, bloc, file, byte, bit);
		fprint_unbblc(term->u.app.rhs, bloc, file, byte, bit);
		break;
	case VAR:
		for (int i = 0; i <= term->u.var.index; i++)
			write_bit(1, file, byte, bit);
		write_bit(0, file, byte, bit);
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);
		fprint_unbblc(
			bloc->entries[bloc->length - term->u.ref.index - 2],
			bloc, file, byte, bit);
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

static void write_unbblc(struct bloc_parsed *bloc, FILE *file)
{
	char byte = 0;
	int bit = 0;
	fprint_unbblc(bloc->entries[bloc->length - 1], bloc, file, &byte, &bit);

	if (bit)
		fwrite(&byte, 1, 1, file);
}

struct target_spec target_unbblc = {
	.name = "unbblc",
	.exec = write_unbblc,
};
