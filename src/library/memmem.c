/*
 * Small reimplementation of memmem().
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>


#ifndef HAVE_MEMMEM

/*
 * Find the start of the first occurrence of the substring "needle" of
 * length "needlelen" in the memory area "haystack" of length "haystacklen".
 * Returns a pointer to the start of the beginning of the substring, or NULL
 * if not found.
 */
char *minimemmem(char *haystack, long haystacklen, char *needle,
		 long needlelen)
{
	char *found;

	if (haystack == NULL)
		return NULL;
	if (haystacklen < 1)
		return NULL;
	if (needle == NULL)
		return haystack;
	if (needlelen < 1)
		return haystack;

	while (haystacklen > needlelen) {
		found = memchr(haystack, needle[0], haystacklen);
		if (found == NULL)
			return NULL;
		haystacklen -= (found - haystack);
		if (haystacklen < needlelen)
			return NULL;
		haystack = found;
		if (memcmp(haystack, needle, needlelen) == 0)
			return haystack;
		haystack++;
		haystacklen--;
	}

	return NULL;
}

#endif				/* HAVE_MEMMEM */

/* EOF */
