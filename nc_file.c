#include "nc_common.h"

GLOBAL nc_file *nc_file_create(const char *fname){
	nc_file *file = (nc_file *) malloc(sizeof(nc_file));
	file->parent = NULL;
	file->next = NULL;
	file->line = file->col = 0;
	file->unget_buf_top = 0;
	if (strcmp(fname, "-") == 0) {
		file->fname = NULL;
		file->fp = stdin;
	} else {
		file->fname = fname;
		file->fp = fopen(fname, "rb");
	}
	return file;
}

GLOBAL void nc_file_destroy(nc_file *file) {
	if (file == NULL) return;
	if (file->fp != NULL) fclose(file->fp);
	free(file);
}

GLOBAL void nc_ungetc(nc_context *ctx, int c) {
	if (c == -1) {
		if (ctx->file->unget_buf_top > 0 &&
			ctx->file->unget_buf[ctx->file->unget_buf_top - 1] == -1)
			return;
	}
	assert(ctx->file->unget_buf_top < MAX_UNGET);
	ctx->file->unget_buf[ctx->file->unget_buf_top++] = c;
}

static int nc_getc0(nc_context *ctx) {
	nc_file *file;
	int c;
	for (;;) {
		file = ctx->file;
		if (file->unget_buf_top) {
			c = file->unget_buf[--file->unget_buf_top];
		} else {
			c = fgetc(file->fp);
		}
		if (c != -1 || file->parent == NULL) break;
		ctx->file = file->parent;
		nc_file_destroy(file);
	}
	return c;
}

static char nc_getc1(nc_context *ctx) {
	int c = nc_getc0(ctx);
	if (c == 0x0d) {	// CR
		c = nc_getc0(ctx);
		if (c != 0x0a) {	// LF
			nc_ungetc(ctx, c);
			c = 0x0d;
		}
	}
	if ((0x00 <= c && c <= 0x08) ||
		(0x0b <= c && c <= 0x1f) ||
		(0x7f <= c && c <= 0x9f)) {
		E_FATAL("invalid charcode: %d", c);
		c = -1;
	}
	if (c == -1) c = 0;
	assert(0 <= c && c <= 0xFF);
	return (char) c;
}

static char nc_getc2(nc_context *ctx) {
	char c = nc_getc1(ctx);
	if (c == '\\') {
		c = nc_getc1(ctx);
		if (c == '\n') {
			c = nc_getc1(ctx);
		} else {
			nc_ungetc(ctx, c);
			c = '\\';
		}
	}
	return c;
}

static char nc_getc3(nc_context *ctx) {
	char c = nc_getc2(ctx);
	if (c == '?') {
		c = nc_getc2(ctx);
		if (c == '\\') {
			c = nc_getc2(ctx);
			if (c == '?') {
				nc_ungetc(ctx, '?');
			} else {
				nc_ungetc(ctx, c);
				nc_ungetc(ctx, '\\');
				c = '?';
			}
		} else if (c == '?') {
			c = nc_getc2(ctx);
			if      (c == '=') c = '#';
			else if (c == '(') c = '[';
			else if (c == '/') c = '\\';
			else if (c == ')') c = ']';
			else if (c == '\'') c = '^';
			else if (c == '<') c = '{';
			else if (c == '!') c = '|';
			else if (c == '>') c = '}';
			else if (c == '-') c = '~';
			else {
				nc_ungetc(ctx, c);
				nc_ungetc(ctx, '?');
				c = '?';
			}
		} else {
			nc_ungetc(ctx, c);
			c = '?';
		}
	}
	return c;
}

static char unget_unicode(nc_context *ctx, unsigned int val) {
	if (val < 0x80) return val & 0x7F;
	else if (val < 0x800) {
		nc_ungetc(ctx, 0xFF & (0x80 | (val & 0x3F)));
		return 0xFF & (0xC0 | (val >> 6));
	} else if (val < 0x10000) {
		nc_ungetc(ctx, 0xFF & (0x80 | (val & 0x3F)));
		nc_ungetc(ctx, 0xFF & (0x80 | ((val >> 6) & 0x3F)));
		return 0xFF & (0xE0 | (val >> 12));
	} else if (val < 0x200000) {
		nc_ungetc(ctx, 0xFF & (0x80 | (val & 0x3F)));
		nc_ungetc(ctx, 0xFF & (0x80 | ((val >> 6) & 0x3F)));
		nc_ungetc(ctx, 0xFF & (0x80 | ((val >> 12) & 0x3F)));
		return 0xFF & (0xF0 | (val >> 18));
	}
	E_FATAL("invalid universal character name.");
	return 0;
}

static char nc_getc4(nc_context *ctx) {
	char c = nc_getc3(ctx);
	if (c == '\\') {
		c = nc_getc3(ctx);
		if (c == 'u' || c == 'U') {
			int i = c == 'u' ? 4 : 8;
			unsigned int val = 0;
			while (i-- > 0) {
				c = nc_getc3(ctx);
				if      ('0' <= c && c <= '9') val = val * 16 + (c - '0');
				else if ('a' <= c && c <= 'f') val = val * 16 + (c - 'a' + 10);
				else if ('A' <= c && c <= 'F') val = val * 16 + (c - 'A' + 10);
				else {
					E_FATAL("Universal char name error");
					return -1;
				}
			}
			if (val <= 0x20 ||
				(0x7f <= val && val <= 0x9f) ||
				(0xd800 <= val && val <= 0xdfff)) {
				E_FATAL("Universal char name error");
				return -1;
			}
			c = unget_unicode(ctx, val);
		} else {
			nc_ungetc(ctx, c);
			c = '\\';
		}
	}
	return c;
}

GLOBAL char nc_getc(nc_context *ctx) {
	char c = nc_getc4(ctx);
	if (c == '/') {
		c = nc_getc4(ctx);
		if (c == '/') {
			while (c != '\0' && c != '\n') c = nc_getc4(ctx);
		} else if (c == '*') {
			do {
				c = nc_getc4(ctx);
				if (c == '*') {
					c = nc_getc4(ctx);
					if (c == '/') break;
				}
			} while (c != '\0');
			if (c == '\0') {
				E_FATAL("comment reached EOF.");
			} else c = ' ';
		} else {
			nc_ungetc(ctx, c);
			c = '/';
		}
	}
	return c;
}

