/*
 * Replacement memmem function's header file. Include this AFTER config.h.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifndef _LIBRARY_MEMMEM_H
#define _LIBRARY_MEMMEM_H 1

#ifdef HAVE_MEMMEM
#define _GNU_SOURCE
#define __USE_GNU
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_MEMMEM
char *minimemmem(char *, long, char *, long);
#define memmem minimemmem
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LIBRARY_MEMMEM_H */

/* EOF */
