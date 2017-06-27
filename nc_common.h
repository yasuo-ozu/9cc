// nc_common.h

#pragma once

#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MAX_FNAME_LEN	1024
#define MAX_UNGET		32

#define GLOBAL
#define GLODEF

#include "nc_def.h"
// #include "nc_function.h"
#include "nc_function.inc"
#include "nc_error.h"

#ifdef DEBUG
extern char *__unused_tmpval;
#define UNUSED(a,...)	((void)(0 && printf(__unused_tmpval,a,__VA_ARGS__+0)))
#else
#define UNUSED(a,...)
#endif
