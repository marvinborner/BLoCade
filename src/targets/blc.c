// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

// TODO: Instead of unsharing open entries, they could be abstracted into closed terms
// and then resubstituted using the Jedi spaceship invasion technique.
// This would only help in very rare edge cases though

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <target.h>
#include <parse.h>
#include <log.h>

#define META_CLOSED 0x1
#define META_OPEN 0x2

static void fprint_blc_substituted(struct term *term, struct bloc_parsed *bloc,
				   size_t *positions_inv, void **closed,
				   int depth, size_t position, FILE *file)
{
	switch (term->type) {
	case ABS:
		fprintf(file, "00");
		fprint_blc_substituted(term->u.abs.term, bloc, positions_inv,
				       closed, depth + 1, position, file);
		break;
	case APP:
		fprintf(file, "01");
		fprint_blc_substituted(term->u.app.lhs, bloc, positions_inv,
				       closed, depth, position, file);
		fprint_blc_substituted(term->u.app.rhs, bloc, positions_inv,
				       closed, depth, position, file);
		break;
	case VAR:
		for (int i = 0; i <= term->u.var.index; i++)
			fprintf(file, "1");
		fprintf(file, "0");
		break;
	case REF:
		if (term->u.ref.index + 1 >= bloc->length)
			fatal("invalid ref index %ld\n", term->u.ref.index);

		if (closed[term->u.ref.index]) {
			int index =
				depth +
				(positions_inv[term->u.ref.index] - position) -
				1;
			debug("index=%d depth=%ld ref=%ld inv=%ld pos=%ld sub=%ld\n",
			      index, depth, term->u.ref.index,
			      positions_inv[term->u.ref.index], position,
			      positions_inv[term->u.ref.index] - position);
			assert(index >= 0);
			for (int i = 0; i <= index; i++)
				fprintf(file, "1");
			fprintf(file, "0");
		} else {
			fprint_blc_substituted(bloc->entries[term->u.ref.index],
					       bloc, positions_inv, closed,
					       depth, position, file);
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

	// find abstraction count (end) and write header
	size_t end = 0;
	while (1) {
		end++;
		if (end >= bloc->length || !positions[end])
			break;
		fprintf(file, "0100"); // ([
	}

	// create inv, s.t. ref inc0 -> pos lr
	for (size_t i = 0; i < end; i++) {
		debug("%ld: %ld\n", positions[i] - 1, end - i - 1);
		positions_inv[positions[i] - 1] = end - i - 1;
	}

	for (size_t i = end; i > 0; i--) {
		fprint_blc_substituted(bloc->entries[positions[i - 1] - 1],
				       bloc, positions_inv, closed, 0, end - i,
				       file);
	}

	free(positions_inv);
}

// TODO: memoize META_CLOSED with term->meta (more complex than you'd think!)
// TODO: memoize bitmap deps (should be easy)
static unsigned int annotate(struct bloc_parsed *bloc, struct term *term,
			     char *bitmap, int depth)
{
	switch (term->type) {
	case ABS:
		return annotate(bloc, term->u.abs.term, bitmap, depth + 1);
	case APP:;
		unsigned int left =
			annotate(bloc, term->u.app.lhs, bitmap, depth);
		unsigned int right =
			annotate(bloc, term->u.app.rhs, bitmap, depth);
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
		bitmap[term->u.ref.index] = 1;
		return annotate(bloc, bloc->entries[term->u.ref.index], bitmap,
				depth);
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

	positions[*position] = n + 1;
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
		char *bitmap = calloc(bloc->length, 1);
		unsigned int meta = annotate(bloc, bloc->entries[i], bitmap, 0);
		if (!(meta & META_CLOSED)) {
			free(bitmap);
			bitmaps[i] = 0; // won't be needed
			continue;
		}
		bitmaps[i] = bitmap;
	}

	size_t *positions = topological_sort(bitmaps, bloc->length);
	fprint_blc(positions, bitmaps, bloc, file);

	for (size_t i = 0; i < bloc->length; i++) {
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
