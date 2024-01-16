// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#ifndef BLOCADE_TARGET_H
#define BLOCADE_TARGET_H

#include <stdio.h>

#include <parse.h>

struct target_spec {
	const char *name;
	void (*exec)(struct bloc_parsed *bloc, FILE *file);
};

void exec_target(char *name, struct bloc_parsed *bloc, FILE *file);

#endif
