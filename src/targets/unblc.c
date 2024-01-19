// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <target.h>
#include <parse.h>
#include <log.h>

struct context {
	enum { WRITE_BITS, WRITE_ASCII } type;
	FILE *file;
	char *byte;
	int *bit;
};

static void write_context(struct context *context, const char *bits)
{
	if (context->type == WRITE_ASCII) {
		fprintf(context->file, "%s", bits);
		return;
	}

	// WRITE_BITS
	for (const char *p = bits; *p; p++) {
		if (*context->bit > 7) { // flush byte
			fwrite(context->byte, 1, 1, context->file);
			*context->byte = 0;
			*context->bit = 0;
		}

		// TODO: which endianness should be default?
		if (*p & 1)
			*context->byte |= 1UL << *context->bit;
		(*context->bit)++;
	}
}

static void write_unblc(struct term *term, struct bloc_parsed *bloc,
			struct context *context)
{
	switch (term->type) {
	case ABS:
		write_context(context, "00");
		write_unblc(term->u.abs.term, bloc, context);
		break;
	case APP:
		write_context(context, "01");
		write_unblc(term->u.app.lhs, bloc, context);
		write_unblc(term->u.app.rhs, bloc, context);
		break;
	case VAR:
		for (int i = 0; i <= term->u.var.index; i++)
			write_context(context, "1");
		write_context(context, "0");
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);
		write_unblc(bloc->entries[term->u.ref.index], bloc, context);
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

static void write_unblc_ascii(struct bloc_parsed *bloc, FILE *file)
{
	struct context context = {
		.type = WRITE_ASCII,
		.file = file,
	};
	write_unblc(bloc->entries[bloc->length - 1], bloc, &context);
}

static void write_unblc_bits(struct bloc_parsed *bloc, FILE *file)
{
	char byte = 0;
	int bit = 0;
	struct context context = {
		.type = WRITE_BITS,
		.file = file,
		.byte = &byte,
		.bit = &bit,
	};
	write_unblc(bloc->entries[bloc->length - 1], bloc, &context);
	if (bit)
		fwrite(&byte, 1, 1, file);
}

struct target_spec target_unblc = {
	.name = "unblc",
	.exec = write_unblc_ascii,
};

struct target_spec target_unbblc = {
	.name = "unbblc",
	.exec = write_unblc_bits,
};
