// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

// TODO: Instead of unsharing open entries, they could be abstracted into closed terms
// and then resubstituted using the Jedi spaceship invasion technique.
// This would only help in very rare edge cases though

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <target.h>
#include <parse.h>
#include <log.h>

#define META_CLOSED 0x1
#define META_OPEN 0x2

static void fprint_blc_substituted(struct term *term, struct bloc_parsed *bloc,
				   size_t *positions_inv, void **closed,
				   int depth, FILE *file)
{
	switch (term->type) {
	case ABS:
		fprintf(file, "00");
		fprint_blc_substituted(term->u.abs.term, bloc, positions_inv,
				       closed, depth + 1, file);
		break;
	case APP:
		fprintf(file, "01");
		fprint_blc_substituted(term->u.app.lhs, bloc, positions_inv,
				       closed, depth, file);
		fprint_blc_substituted(term->u.app.rhs, bloc, positions_inv,
				       closed, depth, file);
		break;
	case VAR:
		for (int i = 0; i <= term->u.var.index; i++)
			fprintf(file, "1");
		fprintf(file, "0");
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);

		int reffed = bloc->length - term->u.ref.index - 2;
		if (closed[reffed]) {
			debug("closed ref %d\n", reffed);
			int index = depth + positions_inv[reffed];
			for (int i = 0; i <= index; i++)
				fprintf(file, "1");
			fprintf(file, "0");
		} else {
			fprint_blc_substituted(bloc->entries[reffed], bloc,
					       positions_inv, closed, depth,
					       file);
		}
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

static void fprint_blc(size_t *positions, void *closed,
		       struct bloc_parsed *bloc, FILE *file)
{
	size_t *positions_inv =
		calloc(bloc->length * sizeof(*positions_inv), 1);

	size_t end = 0;
	for (size_t i = 0; i < bloc->length; i++) {
		if (!positions[i]) {
			end = i;
			break;
		}
		positions_inv[bloc->length - positions[i]] = i;
		fprintf(file, "0100"); // ([
		/* fprintf(file, "(["); */
	}

	/* fprintf(file, "0"); */

	for (size_t i = end; i > 0; i--) {
		/* fprintf(file, "]"); // implicit */
		fprint_blc_substituted(
			bloc->entries[bloc->length - positions[i - 1]], bloc,
			positions_inv, closed, 0, file);
		/* fprintf(file, "<%ld>", bloc->length - positions[i - 1]); */
		/* fprintf(file, ")"); // implicit */
	}

	free(positions_inv);
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
		break;
	default:
		fatal("invalid type %d\n", term->type);
	}
}

// TODO: memoize META_CLOSED with term->meta (more complex than you'd think!)
static unsigned int annotate(struct bloc_parsed *bloc, struct term *term,
			     int depth)
{
	switch (term->type) {
	case ABS:
		return annotate(bloc, term->u.abs.term, depth + 1);
	case APP:;
		unsigned int left = annotate(bloc, term->u.app.lhs, depth);
		unsigned int right = annotate(bloc, term->u.app.rhs, depth);
		if ((left & META_OPEN) || (right & META_OPEN))
			return META_OPEN;
		return META_CLOSED;
	case VAR:
		if (term->u.var.index >= depth)
			return META_OPEN;
		return META_CLOSED;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);
		struct term *sub =
			bloc->entries[bloc->length - term->u.ref.index - 2];
		return annotate(bloc, sub, depth);
	default:
		fatal("invalid type %d\n", term->type);
	}
}

#define PERMANENT_MARK 0x1
#define TEMPORARY_MARK 0x2
static void visit(char **bitmaps, char *marks, size_t *positions,
		  size_t *position, size_t length, size_t n)
{
	if (!bitmaps[n]) {
		marks[n] |= PERMANENT_MARK;
		return;
	}

	if (marks[n] & PERMANENT_MARK)
		return;
	if (marks[n] & TEMPORARY_MARK)
		fatal("dependency graph has a cycle (infinite term)\n");

	marks[n] |= TEMPORARY_MARK;
	for (size_t i = 0; i < length; i++) {
		if (bitmaps[n][i]) {
			visit(bitmaps, marks, positions, position, length, i);
		}
	}
	marks[n] &= ~TEMPORARY_MARK;
	marks[n] |= PERMANENT_MARK;

	positions[*position] = length - n; // deliberate offset of +2
	(*position)++;
}

static size_t *topological_sort(char **bitmaps, size_t length)
{
	char *marks = calloc(length, 1);
	size_t *positions = calloc(length * sizeof(*positions), 1);

	size_t position = 0;

	int marked = 1;
	while (marked) { // TODO: outer loop can be removed?
		marked = 0;
		for (size_t i = 0; i < length; i++) {
			if ((marks[i] & PERMANENT_MARK) == 0)
				marked = 1; // still has nodes without permanent
			if (marks[i] & 0x3)
				continue;
			visit(bitmaps, marks, positions, &position, length, i);
		}
	}

	free(marks);
	return positions;
}

static void write_blc(struct bloc_parsed *bloc, FILE *file)
{
	char **bitmaps = malloc(bloc->length * sizeof(*bitmaps));
	for (size_t i = 0; i < bloc->length; i++) {
		unsigned int meta = annotate(bloc, bloc->entries[i], 0);
		if (!(meta & META_CLOSED)) {
			bitmaps[i] = 0;
			continue;
		}

		char *bitmap = calloc(bloc->length, 1);
		bitmap_deps(bloc, bitmap, bloc->entries[i]);
		bitmaps[i] = bitmap;
	}

	size_t *positions = topological_sort(bitmaps, bloc->length);
	fprint_blc(positions, bitmaps, bloc, file);
	/* fprintf(file, "\n"); */

	for (size_t i = 0; i < bloc->length; i++) {
		/* printf("%ld: %ld, ", i, positions[i]); */
		if (bitmaps[i])
			free(bitmaps[i]);
	}
	free(bitmaps);
	free(positions);
}

struct target_spec target_blc = {
	.name = "blc",
	.exec = write_blc,
};
