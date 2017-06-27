#include "nc_common.h"

#define STRING_SIZE_INIT	128

GLOBAL nc_string *nc_string_create(char *s) {
	nc_string *str = calloc(1, sizeof(nc_string));
	if (s != NULL) nc_string_append_string(str, s);
	else {
		str->allocated = STRING_SIZE_INIT;
		str->body = malloc(STRING_SIZE_INIT);
		assert(str->body != NULL);
	}
	return str;
}

GLOBAL void nc_string_destroy(nc_string *str) {
	if (str != NULL) free(str->body);
	free(str);
}

static void nc_string_extend(nc_string *str, size_t fact) {
	size_t size = str->allocated;
	if (size < fact) size = fact;
	else if (size < fact * 4) size *= 2;
	else size += fact * 2;
	char *p = malloc(size);
	assert(p != NULL);
	memcpy(p, str->body, str->length);
	free(str->body);
	str->body = p;
	str->allocated = size;
}

GLOBAL char *nc_string_get(nc_string *str) {
	return str->body;
}

GLOBAL void nc_string_append(nc_string *str, char c) {
	if (str->length + 2 > str->allocated) {
		nc_string_extend(str, STRING_SIZE_INIT);
	}
	str->body[str->length++] = c;
	str->body[str->length] = '\0';
}

GLOBAL void nc_string_append_string(nc_string *str, char *s) {
	int len = strlen((char *)s);
	if (str->length + len + 1 > str->allocated) {
		nc_string_extend(str, len + 1);
	}
	strcpy((char *)(str->body + str->length), (char *)s);
	str->length += len;
}

#define VECTOR_SIZE_INIT	64

GLOBAL nc_vector *nc_vector_create() {
	nc_vector *vec = calloc(1, sizeof(nc_vector));
	vec->body = malloc((size_t)VECTOR_SIZE_INIT);
	assert(vec->body != NULL);
	vec->allocated = VECTOR_SIZE_INIT;
	return vec;
}

GLOBAL void nc_vector_destroy(nc_vector *vec) {
	if (vec != NULL) free(vec->body);
	free(vec);
}

GLOBAL void nc_vector_append(nc_vector *vec, void *val) {
	if (vec->length + 1 > vec->allocated) {
		size_t size = vec->length * 2;
		void *p = malloc(size);
		assert(p != NULL);
		memcpy(p, vec->body, vec->length);
		vec->allocated = size;
		free(vec->body);
		vec->body = p;
	}
	vec->body[vec->length++] = val;
}

GLOBAL void *nc_vector_pop(nc_vector *vec) {
	return vec->body[--vec->length];
}

GLOBAL void nc_vector_clear(nc_vector *vec) {
	vec->length = 0;
}

static size_t nc_map_hash(char *s, size_t size) {
	size_t r = 7148987;
	do {
		r ^= *s;
		r *= 1536673;
	} while (*s++);
	return r % size;
}

GLOBAL nc_map *nc_map_create(int size) {
	nc_map *map = calloc(1, sizeof(nc_map));
	map->size = size;
	map->key = calloc(1, sizeof(char *) * size);
	map->value = malloc(sizeof(void *) * size);
	return map;
}

GLOBAL void nc_map_destroy(nc_map *map) {
	size_t i;
	if (map == NULL) return;
	for (i = 0; i < map->size; i++) free(map->key[i]);
	free(map->key);
	free(map->value);
	free(map);
}

GLOBAL void nc_map_free_destroy(nc_map *map) {
	size_t i;
	if (map == NULL) return;
	for (i = 0; i < map->size; i++) {
		if (map->key[i] != NULL) free(map->value[i]);
	}
	nc_map_destroy(map);
}

GLOBAL void nc_map_add(nc_map *map, char *key, void *value) {
	size_t i = nc_map_hash(key, map->size);
	assert(map->count < map->size);
	while (map->key[i] != NULL) i = (i + 1) % map->size;
	map->key[i] = (char *) strdup((char *)key);
	map->value[i] = value;
}

GLOBAL void *nc_map_search(nc_map *map, char *key) {
	int i = nc_map_hash(key, map->size);
	assert(map->count < map->size);
	while (map->key[i] != NULL) {
		if (strcmp((char *)map->key[i], (char *)key) == 0) {
			return map->value[i];
		}
		i = (i + 1) % map->size;
	}
	return NULL;
}
