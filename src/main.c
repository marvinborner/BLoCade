// Copyright (c) 2024, Marvin Borner <dev@marvinborner.de>
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <target.h>
#include <term.h>
#include <log.h>
#include <parse.h>

// automatically generated using gengetopt
#include "cmdline.h"

#define BUF_SIZE 1024
static char *read_stdin(void)
{
	debug("reading from stdin\n");
	freopen(NULL, "rb", stdin);
	size_t size = 1;
	char *string = malloc(sizeof(char) * BUF_SIZE);
	if (!string)
		fatal("out of memory!\n");

	char ch;
	while (fread(&ch, sizeof(char), 1, stdin) == 1) {
		if (size % BUF_SIZE == 0) {
			string = realloc(string,
					 sizeof(char) * (size + BUF_SIZE));
			if (!string)
				fatal("out of memory!\n");
		}
		string[size - 1] = ch;
		size++;
	}
	string[size - 1] = '\0';

	if (ferror(stdin)) {
		free(string);
		fatal("can't read from stdin\n");
	}
	return string;
}

static char *read_file(FILE *f)
{
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = malloc(fsize + 1);
	if (!string)
		fatal("out of memory!\n");
	int ret = fread(string, fsize, 1, f);

	if (ret != 1) {
		free(string);
		fatal("can't read file: %s\n", strerror(errno));
	}

	string[fsize] = 0;
	return string;
}

static char *read_path(const char *path)
{
	debug("reading from %s\n", path);
	FILE *f = fopen(path, "rb");
	if (!f)
		fatal("can't open file %s: %s\n", path, strerror(errno));
	char *string = read_file(f);
	fclose(f);
	return string;
}

int main(int argc, char **argv)
{
	struct gengetopt_args_info args;
	if (cmdline_parser(argc, argv, &args))
		exit(1);

	debug_enable(args.verbose_flag);

	char *input;
	if (args.input_arg[0] == '-') {
		input = read_stdin();
	} else {
		input = read_path(args.input_arg);
	}

	if (!input)
		return 1;

	if (args.target_arg) {
		struct bloc_parsed *bloc = parse_bloc(input);

		FILE *file =
			args.output_arg ? fopen(args.output_arg, "wb") : stdout;
		exec_target(args.target_arg, bloc, file);
		fclose(file);

		free(input);
		free_bloc(bloc);
		return 0;
	}

	fatal("invalid options: use --help for information\n");
	return 1;
}
