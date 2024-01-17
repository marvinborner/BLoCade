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

static void bitmap_deps(struct bloc_parsed *bloc, char *bitmap,
			struct term *term)
{
	switch (term->type) {
	case ABS:
		bitmap_deps(bloc, bitmap, term->u.abs.term);
		break;
	case APP:
		bitmap_deps(bloc, bitmap, term->u.app.lhs);
		bitmap_deps(bloc, bitmap, term->u.app.rhs);
		break;
	case VAR:
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);
		bitmap[bloc->length - term->u.ref.index - 2] = 1;
		// for recursive bitmapping, not needed for DFS
		/* bitmap_deps( */
		/* 	bloc, bitmap, */
		/* 	bloc->entries[bloc->length - term->u.ref.index - 2]); */
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

static void print_deps(size_t length, char *bitmap)
{
	for (size_t i = 0; i < length; i++) {
		if (bitmap[i])
			printf("%ld,", i);
	}
	printf("\n");
}

#define PERMANENT_MARK 0x1
#define TEMPORARY_MARK 0x2
static void visit(char **bitmaps, char *marks, size_t length, size_t n)
{
	if (marks[n] & PERMANENT_MARK)
		return;
	if (marks[n] & TEMPORARY_MARK)
		fatal("dependency graph has a cycle (infinite term)\n");

	marks[n] |= TEMPORARY_MARK;
	for (size_t i = 0; i < length; i++) {
		if (bitmaps[n][i]) {
			visit(bitmaps, marks, length, i);
		}
	}
	marks[n] &= ~TEMPORARY_MARK;

	marks[n] |= PERMANENT_MARK;
	printf("PUSH %ld\n", n);
}

static void topological_sort(char **bitmaps, size_t length)
{
	char *marks = calloc(length, 1);

	int marked = 1;
	while (marked) {
		marked = 0;
		for (size_t i = 0; i < length; i++) {
			if ((marks[i] & PERMANENT_MARK) == 0)
				marked = 1; // still has nodes without permanent
			if (marks[i])
				continue;
			visit(bitmaps, marks, length, i);
		}
	}

	free(marks);
}

static void write_blc(struct bloc_parsed *bloc, FILE *file)
{
	/* fprint_blc(bloc->entries[bloc->length - 1], bloc, file); */
	/* fprintf(file, "\n"); */

	char **bitmaps = malloc(bloc->length * sizeof(*bitmaps));
	for (size_t i = 0; i < bloc->length; i++) {
		char *bitmap = calloc(bloc->length, 1);
		bitmap_deps(bloc, bitmap, bloc->entries[i]);

		/* printf("entry %ld\n", bloc->length - i - 2); */
		/* printf("entry %ld\n", i); */
		/* print_deps(bloc->length, bitmap); */
		bitmaps[i] = bitmap;
	}
	/* printf("\n"); */

	topological_sort(bitmaps, bloc->length);

	for (size_t i = 0; i < bloc->length; i++) {
		free(bitmaps[i]);
	}
	free(bitmaps);
}

struct target_spec target_blc = {
	.name = "blc",
	.exec = write_blc,
};
