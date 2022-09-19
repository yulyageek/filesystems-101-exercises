#pragma once

/* Print a message into a heap-allocated string, and panic if memory allocation fails. */
char* fs_xasprintf(const char *fmt, ...) __attribute__((malloc, format(printf, 1, 2)));

/* Duplicate a string, and panic if memory allocation fails. */
char* fs_xstrdup(const char *x) __attribute__((malloc));
