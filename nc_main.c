#include "nc_common.h"

/*
* The MIT License (MIT)
* 
* Copyright 2017 Yasuo Ozu
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of 
* this software and associated documentation files (the "Software"), to deal in th
* e Software without restriction, including without limitation the rights to use, 
* copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
* Software, and to permit persons to whom the Software is furnished to do so, subj
* ect to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all c
* opies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLI
* ED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR 
* A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYR
* IGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WIT
* H THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

int nc_read_options(int argc, char **argv, nc_context *ctx) {
	UNUSED(argc);
	int i = 0;
	char *s;
	nc_file *file;
	while ((s = argv[++i]) != NULL) {
		file = NULL;
		if (*s == '-') {
			s++;
			if (*s == '-') {
				s++;
				if (strcmp(s, "verbose") == 0) ctx->options.verbose = 1;
				else {
					E_FATAL("invalid option --%s", s);
					return 0;
				}
			} else {
				if (*s == '\0') {	// got '-' --- read stdin as file
					file = nc_file_create("-");
				} else while (*s != '\0') {
					if (*s == 'o') {
						s++;
						if (*s != '\0' || ctx->options.outfile != NULL) {
							E_FATAL("invalid option -o");
							return 0;
						} else {
							ctx->options.outfile = s = argv[++i];
							break;
						}
					} else if (*s == 'v') {
						ctx->options.verbose = 1;
					} else {
						E_FATAL("invalid option -%c", *s);
						return 0;
					}
					s++;
				}
			}
		} else {
			file = nc_file_create(s);
		}
		if (file != NULL) {
			if (file->fp == NULL) {
				E_FATAL("file open error: %s", file->fname == NULL ? "stdin" : file->fname);
				return 0;
			}
			if (ctx->file == NULL) ctx->file = file;
			else {
				nc_file *filep = ctx->file;
				while (filep->next != NULL) filep = filep->next;
				filep->next = file;
			}
		}
	}
	return 1;
}

int nc_main(int argc, char **argv) {
	char s[MAX_FNAME_LEN + 1], *c;
	nc_context ctx;
	ctx.file = NULL;
	ctx.buf = nc_vector_create();
	ctx.options.verbose = 0;
	ctx.options.outfile = NULL;
	if (!nc_read_options(argc, argv, &ctx)) return 1;
	if (ctx.file == NULL) {
		E_FATAL("no input files.");
		return 1;
	}
	if (ctx.options.outfile != NULL) {
		if (ctx.file->next != NULL) {
			E_FATAL("cannot specify -o with multiple files.");
			return 1;
		}
	} else if (ctx.file->next == NULL) {
		if (ctx.file->fname == NULL) { // stdin, etc
			ctx.options.outfile = "a.s";
		} else {
			strncpy(s, ctx.file->fname, MAX_FNAME_LEN - 2);
			s[MAX_FNAME_LEN - 2] = '\0';
			for (c = s; *c != '\0' && *c != '.'; c++);
			*c++ = '.'; *c++ = 's'; *c = '\0';
			ctx.options.outfile = s;
		}
	}

	while (ctx.file != NULL) {
		nc_token *token;
		while ((token = nc_next_token(&ctx))) nc_token_dump(&ctx, token);
		nc_file *file = ctx.file;
		ctx.file = file->next;
		nc_file_destroy(file);
	}
	return 0;
}

int main(int argc, char **argv) {
	return nc_main(argc, argv);
}
