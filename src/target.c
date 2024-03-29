// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <string.h>

#include <log.h>
#include <target.h>

extern struct target_spec target_unblc;
extern struct target_spec target_unbblc;
extern struct target_spec target_blc;
extern struct target_spec target_bblc;

static struct target_spec *targets[] = {
	&target_unblc,
	&target_unbblc,
	&target_blc,
	&target_bblc,
};

void exec_target(char *name, struct bloc_parsed *bloc, FILE *file)
{
	int count = sizeof(targets) / sizeof(struct target_spec *);
	for (int i = 0; i < count; i++) {
		if (!strcmp(targets[i]->name, name)) {
			targets[i]->exec(bloc, file);
			return;
		}
	}

	fprintf(stderr, "available targets:\n");
	for (int i = 0; i < count; i++)
		fprintf(stderr, "\t%s\n", targets[i]->name);

	fatal("unknown target %s\n", name);
}
