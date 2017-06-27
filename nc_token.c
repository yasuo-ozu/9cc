#include "nc_common.h"

GLOBAL nc_token *nc_token_create() {
	nc_token *token = calloc(1, sizeof(nc_token));
	token->text = nc_string_create(NULL);
	return token;
}

GLOBAL void nc_token_destroy(nc_token *token) {
	if (token == NULL) return;
	nc_string_destroy(token->text);
	free(token);
}

GLOBAL void nc_token_dump(nc_context *ctx, nc_token *token) {
	if (token->type == TOKEN_NULL) printf("NULL\n");
	else if (token->type == TOKEN_SYMBOL) printf("SYMBOL: %s\n", token->text->body);
	else if (token->type == TOKEN_IDENT) printf("IDENT: %s\n", token->text->body);
	else if (token->type == TOKEN_KEYWORD) printf("KEYWORD: %s\n", token->text->body);
	else if (token->type == TOKEN_LITERAL) {
		printf("LITERAL: (");
		nc_type_dump(ctx, token->value->type);
		printf(") ");
		nc_variable_dump(ctx, token->value);
		printf("\n");
	}
}

static void nc_next_token_literal(nc_context *ctx, char c, nc_token *token) {
	token->type = TOKEN_LITERAL;
	char a = c;
	int count = 0, cval = 0;
	nc_string *str;
	nc_type *type = nc_type_create();
	type->kind = TYPEKIND_INTEGER;
	type->align_size = type->size = 1;
	type->is_const = true;
	type->is_unsigned = false;
	if (a == '"') {
		str = nc_string_create(NULL);
		nc_type *type2 = nc_type_create();
		type2->kind = TYPEKIND_POINTER;
		type2->pointer_to = type;
		type2->is_const = true;
		type2->size = 4;
		type = type2;
	}
	while ((c = nc_getc(ctx)) != a) {
		if (c == '\n' || c == '\0') {
			E_FATAL("Literal reached end of line.");
			break;
		} else if (c == '\\') {
			c = nc_getc(ctx);
			if      (c == 'n') c = '\n';
			else if (c == 't') c = '\t';
			else if (c == 'v') c = '\v';
			else if (c == 'b') c = '\b';
			else if (c == 'r') c = '\r';
			else if (c == 'f') c = '\f';
			else if (c == 'a') c = '\a';
			else if (c == 'x') {
				int val = 0;
				for (;;) {
					c = nc_getc(ctx);
					if      ('0' <= c && c <= '9') val = val * 16 + (c - '0');
					else if ('a' <= c && c <= 'f') val = val * 16 + (c - 'a' + 10);
					else if ('A' <= c && c <= 'F') val = val * 16 + (c - 'A' + 10);
					else break;
				}
				nc_ungetc(ctx, c);
				c = (char) val;
			} else if ('0' <= c && c <= '9') {
				int val = 0;
				while ('0' <= c && c <= '7') {
					val = val * 8 + (c - '0');
					c = nc_getc(ctx);
				}
				nc_ungetc(ctx, c);
				c = (char) val;
			}
		}
		if (a == '"') nc_string_append(str, c);
		else cval = (cval << 8) + c;
		count++;
	}
	if (a == '\'' && count > 1) type->size = 4;
	if (a == '\'' && count == 0) {
		E_FATAL("char literal must include some chars.");
	}
	nc_variable *var = nc_variable_create();
	var->type = type;
	var->value = malloc(type->size);
	if (a == '"') {
		*(unsigned *) var->value = ctx->buf->length;	// fake address
		nc_vector_append(ctx->buf, str);
	} else {
		if (type->size == 4) *(int *) var->value = cval;
		else *(char *) var->value = (char) cval;
	}
	token->value = var;
}

const char *token_keyword_table[] = {
	"_Bool", "char", "short", "int", "long", "signed", "unsigned", "float", "double", "_Complex", "_Imaginary",
	"struct", "union", "enum", "volatile", "const", "restrict", "auto", "extern", "static", "register",
	"typedef", "void", "if", "else", "switch", "case", "default", "for", "do", "while", "goto", "continue", "break", "return",
	"inline", "sizeof", NULL
};
static void nc_next_token_ident(nc_context *ctx, char c, nc_token *token) {
	int i;
	token->type = TOKEN_IDENT;
	do {
		nc_string_append(token->text, c);
		c = nc_getc(ctx);
	} while (c == '_' || ('A' <= c && c <= 'Z') ||
			('a' <= c && c <= 'z') || ('0' <= c && c <= '9'));
	for (i = 0; token_keyword_table[i] != NULL; i++) {
		if (strcmp(token_keyword_table[i], token->text->body) == 0) {
			token->type = TOKEN_KEYWORD;
			break;
		}
	}
	nc_ungetc(ctx, c);
}

static void nc_next_token_numdbl(nc_context *ctx, char c, nc_token *token) {
	token->type = TOKEN_LITERAL;
	bool isHex = false, isOct = false, isFloating = false;
	if (c == '0') {
		nc_string_append(token->text, c);
		c = nc_getc(ctx);
		if (c == 'x' || c == 'X') {
			isHex = true;
			do {
				nc_string_append(token->text, c);
				c = nc_getc(ctx);
			} while (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') 
					|| ('a' <= c && c <= 'z'));
		} else {
			isOct = true;
			while ('0' <= c && c <= '7') {
				nc_string_append(token->text, c);
				c = nc_getc(ctx);
			}
		}
	} else {
		while ('0' <= c && c <= '9') {
			nc_string_append(token->text, c);
			c = nc_getc(ctx);
		}
	}
	if (c == '.') {
		isFloating = true;
		nc_string_append(token->text, c);
		c = nc_getc(ctx);
		while (('0' <= c && c <= '9') ||
				(isHex && (('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')))) {
			nc_string_append(token->text, c);
			c = nc_getc(ctx);
		}
	}
	if ((isHex && isFloating && (c == 'P' || c == 'p')) ||
			(!isHex && (c == 'E' || c == 'e'))) {
		isFloating = true;
		nc_string_append(token->text, c);
		c = nc_getc(ctx);
		if (c == '+' || c == '-') {
			nc_string_append(token->text, c);
			c = nc_getc(ctx);
		}
		if ('0' <= c && c <= '9') {
			do {
				nc_string_append(token->text, c);
				c = nc_getc(ctx);
			} while ('0' <= c && c <= '9');
		} else {
			E_FATAL("exponent requires decimal number.");
		}
	}
	int countUnsigned = 0, countLong = 0, size = 4, countFloat = 0, sign = 1;
	for (;;) {
		if (c == 'l' || c == 'L') countLong++;
		else if (c == 'u' || c == 'U') countUnsigned++;
		else if (c == 'f' || c == 'F') countFloat++;
		else break;
		nc_string_append(token->text, c);
		c = nc_getc(ctx);
	}
	if ((isFloating && (countLong + countFloat > 1 || countUnsigned)) ||
			(!isFloating && (countLong > 2 || countFloat || countUnsigned > 1))) {
		E_FATAL("missing in flag.");
		while (c != ' ' && c != '\n' && c != '\0') c = nc_getc(ctx);
	}
	nc_ungetc(ctx, c);
	nc_variable *var = nc_variable_create();
	nc_type *type = nc_type_create();
	var->type = type;
	type->is_const = true;
	char *s = token->text->body;
	if (isOct && isFloating) s++; // oct with floating point or exponent is changed into dec.
	if (isFloating) {
		type->kind = TYPEKIND_FLOAT;
		type->align_size = type->size
			= (countLong ? 16 : (countFloat ? 4 : 8));
		var->value = malloc(type->size);
		if (type->size == 4) *(float *) var->value = strtof(s, NULL);
		else if (type->size == 8) *(double *) var->value = strtod(s, NULL);
		else if (type->size == 16) *(long double *) var->value = strtold(s, NULL);
		else assert(0);
	} else {
		type->kind = TYPEKIND_INTEGER;
		type->align_size = type->size = 4;
		if (countLong >= 2) {
			type->align_size = type->size = 8;
		}
		if (countUnsigned) {
			type->is_unsigned = true;
		}
		var->value = malloc(type->size);
		if (countUnsigned && type->size == 4)
			*(unsigned *) var->value = strtoul(s, NULL, 0);
		else if (countUnsigned && type->size == 8)
			*(unsigned long long *) var->value = strtoull(s, NULL, 0);
		else if (type->size == 4) *(int *) var->value = strtol(s, NULL, 0);
		else if (type->size == 8) *(long long *) var->value = strtoll(s, NULL, 0);
		else assert(0);
	}
	token->value = var;
}

/*
 * Digraph
 * <:	[
 * :>	]
 * <%	{
 * %>	}
 * %:	#
*/
const char * const token_symbol_table[] = {
	/* 3 chars */
	"&&=", "||=", "<<=", ">>=",
	/* 2 chars */
	"&&", "||", "++", "--", "<<", ">>",  "+=", "-=", "*=", "/=", 
	"|=", "&=", "^=", "==", "!=", "<=", ">=", "->",
	"<:", ":>", "<%", "%>", "%:",
	0
};
#define MAX_SYMBOL_LEN	8

static void nc_next_token_symbol(nc_context *ctx, char c, nc_token *token) {
	token->type = TOKEN_SYMBOL;
	char s[MAX_SYMBOL_LEN];
	int i, j, k;
	for (i = 0; token_symbol_table[i] != 0; i++) {
		for (j = 0, k = 0; c && token_symbol_table[i][j] == c; j++) {
			s[k++] = c; c = nc_getc(ctx);
		}
		if (token_symbol_table[i][j] == '\0') {
			s[k] = '\0'; break;
		} else {
			s[k++] = c;
			while (k > 1) nc_ungetc(ctx, s[--k]);
			if (k == 1) c = *s;
			k = 0;
		}
	}
	if (!k) {
		s[0] = c; s[1] = '\0';
		c = nc_getc(ctx);
	}
	nc_string_append_string(token->text, s);
	nc_ungetc(ctx, c);
}

GLOBAL nc_token *nc_next_token(nc_context *ctx) {
	char c = nc_getc(ctx);
	while (c == ' ' || c == '\t' || c == '\n') c = nc_getc(ctx);
	if (c == '\0') return NULL;
	nc_token *token = nc_token_create(ctx);
	if (c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
		nc_next_token_ident(ctx, c, token);
	else if ('0' <= c && c <= '9')  
		nc_next_token_numdbl(ctx, c, token);
	else if (c == '\'' || c == '"') 
		nc_next_token_literal(ctx, c, token);
	else nc_next_token_symbol(ctx, c, token);
	return token;
}

