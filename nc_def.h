/*typedef struct nc_string nc_string;
typedef struct nc_vector nc_vector;
typedef struct nc_map nc_map;
typedef struct nc_file nc_file;
typedef struct nc_variable nc_variable;
typedef struct nc_type nc_type;
typedef struct nc_token nc_token;
typedef struct nc_context nc_context;
typedef enum nc_typekind nc_typekind;
*/

#include "nc_def.inc"

// nc_util.c
GLODEF struct nc_string {
	size_t allocated;
	size_t length;
	char *body;
};

GLODEF struct nc_vector {
	size_t allocated;
	size_t length;
	void **body;
};

GLODEF struct nc_map {
	size_t size;
	size_t count;
	char **key;
	void **value;

};

// nc_file.c
GLODEF struct nc_file {
	nc_file *parent, *next;
	const char *fname;
	FILE *fp;
	int line, col;
	int unget_buf[MAX_UNGET];
	int unget_buf_top;
};

// nc_variable.c
GLODEF struct nc_variable {
	nc_type *type;
	void *value;
};

GLODEF enum nc_typekind {
	TYPEKIND_INTEGER = 0, TYPEKIND_FLOAT, TYPEKIND_ARRAY, TYPEKIND_POINTER, TYPEKIND_STRUCT, TYPEKIND_ENUM, TYPEKIND_FUNCTION
};
GLODEF struct nc_type {
	nc_typekind kind;
	size_t size;
	size_t align_size;
	size_t array_size;	// used when array
	bool is_const;
	bool is_unsigned;	// used when object
	nc_type *pointer_to;	// used when array or pointer
	nc_vector *children;	// used when struct, function
};

// nc_token.c
GLODEF enum nc_token_type {
	TOKEN_NULL = 0,
	TOKEN_SYMBOL, TOKEN_IDENT, TOKEN_LITERAL, TOKEN_KEYWORD
};
GLODEF struct nc_token {
	enum nc_token_type type;
	nc_string *text;
	nc_variable *value;
};

// nc_main.c
GLODEF struct nc_context {
	struct nc_options {
		const char *outfile;
		int verbose;
	} options;
	nc_file *file;
	nc_vector *buf;
};
