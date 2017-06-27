#include "nc_common.h"

GLOBAL nc_variable *nc_variable_create() {
	nc_variable *val = calloc(1, sizeof(nc_variable));
	return val;
}

GLOBAL void nc_variable_destroy(nc_variable *val) {
	free(val);
}

GLOBAL void nc_variable_dump(nc_context *ctx, nc_variable *var) {
	if (var->type->kind == TYPEKIND_INTEGER) {
		if (var->type->size == 1 && var->type->is_unsigned == false) {
			printf("%c", *(char *) var->value);
		} else {
			long long val = 0;
			memcpy(&val, var->value, var->type->size);
			printf("%lld", val);
		}
	} else if (var->type->kind == TYPEKIND_FLOAT) {
		if (var->type->size == 4)
			printf("%f", *(float *)var->value);
		else if (var->type->size == 8)
			printf("%lf", *(double *)var->value);
		else if (var->type->size == 16)
			printf("%Lf", *(long double *)var->value);
	} else if (var->type->kind == TYPEKIND_POINTER) {
		if (var->type->pointer_to->kind == TYPEKIND_INTEGER &&
				var->type->pointer_to->size == 1 &&
				var->type->pointer_to->is_unsigned == false) {
			printf("\"%s\"", ((nc_string *)ctx->buf->body[*(int *)var->value])->body);
		}
	} else printf("...");
}

GLOBAL nc_type *nc_type_create() {
	nc_type *type = calloc(1, sizeof(nc_type));
	return type;
}

GLOBAL void nc_type_destroy(nc_type *type) {
	if (type == NULL) return;
	free(type->pointer_to);
	free(type);
}

GLOBAL void nc_type_dump(nc_context *ctx, nc_type *type) {
	if (type->kind == TYPEKIND_INTEGER) {
		if (type->is_const) printf("const ");
		if (type->is_unsigned) printf("unsigned ");
		if (type->size == 1) printf("char ");
		else if (type->size == 2) printf("short ");
		else if (type->size == 4) printf("int ");
		else if (type->size == 8) printf("long long ");
		else printf("<size=%d> ", (int) type->size);
	} else if (type->kind == TYPEKIND_FLOAT) {
		if (type->is_const) printf("const ");
		if (type->is_unsigned) printf("unsigned ");
		if (type->size == 4) printf("float ");
		else if (type->size == 8) printf("double ");
		else if (type->size == 16) printf("long double ");
	} else if (type->kind == TYPEKIND_ARRAY) {
		nc_type_dump(ctx, type->pointer_to);
		printf("[%d] ", (int)type->array_size);
	} else if (type->kind == TYPEKIND_POINTER) {
		nc_type_dump(ctx, type->pointer_to);
		printf("* ");
	} else printf("some ");
}
