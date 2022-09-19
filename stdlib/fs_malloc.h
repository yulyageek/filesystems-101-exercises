#pragma once

#include <stdlib.h>

/* A version of malloc() that panics if an allocation fails. */
void* fs_xmalloc(size_t size) __attribute__((malloc));

/* A version of malloc() that returns zero-initialised memory
   and panics if an allocation fails. */
void* fs_xzalloc(size_t size) __attribute__((malloc));

/* A version of realloc() that panics if an allocation fails. */
void* fs_xrealloc(void *x, size_t size);

/* A version of free() to be used to deallocate blocks returned by
   fs_x*alloc(). */
void fs_xfree(void *x);
