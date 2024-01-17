// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <target.h>
#include <parse.h>
#include <log.h>

static void fprint_blc(struct term *term, struct bloc_parsed *bloc, FILE *file)
{
	switch (term->type) {
	case ABS:
		fprintf(file, "00");
		fprint_blc(term->u.abs.term, bloc, file);
		break;
	case APP:
		fprintf(file, "01");
		fprint_blc(term->u.app.lhs, bloc, file);
		fprint_blc(term->u.app.rhs, bloc, file);
		break;
	case VAR:
		for (int i = 0; i <= term->u.var.index; i++)
			fprintf(file, "1");
		fprintf(file, "0");
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);
		fprint_blc(bloc->entries[bloc->length - term->u.ref.index - 2],
			   bloc, file);
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

static void write_blc(struct bloc_parsed *bloc, FILE *file)
{
	fprint_blc(bloc->entries[bloc->length - 1], bloc, file);
	fprintf(file, "\n");
}

struct target_spec target_blc = {
	.name = "blc",
	.exec = write_blc,
};
