/*
 * Decode RFC2047-encoded strings.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * Table for decoding base64
 */
static char index_64[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#define CHAR64(c)  (index_64[(unsigned char)(c)])

/*
 * Table for decoding hex digits
 */
static char index_hex[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#define CHARHEX(c)  (index_hex[(unsigned char)(c)])


/*
 * Decode a block of Base64-encoded data and return a pointer to the decoded
 * data, which will have been malloc()ed, or NULL on error.  The content of
 * "size" is updated to contain the size of the output buffer, and on entry
 * should contain the size of the input block.
 */
static char *decode_base64(char *data, long *size)
{
	char *out;
	long inpos, outpos;
	int byte, c, c1 = 0, c2 = 0, c3 = 0, c4 = 0;
	char buf[3];			 /* RATS: ignore (checked OK) */

	out = malloc(64 + *size);
	if (out == NULL)
		return NULL;

	for (inpos = 0, outpos = 0, byte = 0; inpos < *size; inpos++) {

		c = data[inpos];

		switch (byte) {
		case 0:
			if (c != '=' && CHAR64(c) == -1)
				continue;
			c1 = c;
			byte++;
			break;
		case 1:
			if (c != '=' && CHAR64(c) == -1)
				continue;
			c2 = c;
			byte++;
			break;
		case 2:
			if (c != '=' && CHAR64(c) == -1)
				continue;
			c3 = c;
			byte++;
			break;
		case 3:
			if (c != '=' && CHAR64(c) == -1)
				continue;
			c4 = c;
			byte++;
			break;
		default:
			break;
		}

		if (byte < 4)
			continue;

		byte = 0;

		if (c1 == '=' || c2 == '=')
			break;

		c1 = CHAR64(c1);
		c2 = CHAR64(c2);
		buf[0] = ((c1 << 2) | ((c2 & 0x30) >> 4));

		out[outpos++] = buf[0];

		if (c3 == '=')
			break;

		c3 = CHAR64(c3);
		buf[1] = (((c2 & 0x0F) << 4) | ((c3 & 0x3C) >> 2));
		out[outpos++] = buf[1];

		if (c4 == '=')
			break;
		c4 = CHAR64(c4);
		buf[2] = (((c3 & 0x03) << 6) | c4);
		out[outpos++] = buf[2];
	}

	out[outpos] = 0;
	*size = outpos;

	return out;
}


/*
 * Take the given single RFC2047 encoded word and store it, decoded, into
 * the given buffer with the given maximum length.
 *
 * It it assumed that "str" contains a valid RFC2047 encoded word, as found
 * by the findencoded function below.
 */
static void decode_rfc2047_word(char *str, char *buf, long bufsize)
{
	int seenq, encoding;
	char *charset;
	char *rpos;
	char *nextq;

	seenq = 0;
	charset = NULL;
	encoding = 0;

	buf[0] = 0;

	for (rpos = str; (nextq = strchr(rpos, '?')); rpos = nextq + 1) {
		char *ptr;
		int n;

		seenq++;

		switch (seenq) {
		case 2:		    /* CHARSET part */
			n = nextq - rpos;

			/*
			 * Since RFC2231 says CHARSET can be of the form
			 * CHARSET*LANGUAGE, we need to throw away the
			 * LANGUAGE part if an asterisk is present.
			 */
			ptr = memchr(rpos, '*', n);
			if (ptr)
				n = ptr - rpos;

			charset = malloc(n + 1);
			if (charset != NULL) {
				memcpy(charset, rpos, n);
				charset[n] = 0;
			}

			break;

		case 3:		    /* ENC part */
			switch (rpos[0]) {
			case 'Q':
			case 'q':
				encoding = 'Q';
				break;
			case 'B':
			case 'b':
				encoding = 'B';
				break;
			default:
				if (charset)
					free(charset);
				return;
			}
			break;

		case 4:		    /* DATA part */

			if (encoding == 'Q') {

				/* Quoted-Printable decoding */

				while ((rpos < nextq) && (bufsize > 0)) {
					if (rpos[0] == '_') {
						buf[0] = ' ';
						buf++;
						bufsize--;
					} else if (rpos[0] == '=') {
						if (rpos[1] == 0)
							break;
						if (rpos[2] == 0)
							break;
						buf[0] =
						    (CHARHEX(rpos[1]) << 4)
						    | CHARHEX(rpos[2]);
						buf++;
						bufsize--;
						rpos += 2;
					} else {
						buf[0] = rpos[0];
						buf++;
						bufsize--;
					}
					rpos++;
				}

				buf[0] = 0;

			} else if (encoding == 'B') {

				/* Base64 decoding */

				char *decbuf;
				long size;

				size = nextq - rpos;
				decbuf = decode_base64(rpos, &size);

				if (decbuf == NULL) {
					if (charset)
						free(charset);
					return;
				}

				if (size > bufsize)
					size = bufsize;

				memcpy(buf, decbuf, size);
				free(decbuf);

				buf += size;
				bufsize -= size;
				buf[0] = 0;

			}
			break;
		}
	}

	if (charset) {
		/*
		 * We don't do anything with the charset information, as
		 * converting between character sets is a bit more
		 * complicated than we need to make this.
		 */
		free(charset);
	}
}


/*
 * Find the next RFC2047 encoded word in the given string, assuming that the
 * encoding must be B or Q (case insensitive, as per the RFC).
 *
 * An RFC2047 encoded word looks like this: =?CHARSET?ENC?DATA?=
 *
 * CHARSET is a character set specifier, ENC is the encoding (Q for
 * quoted-printable, B for base64), and DATA is the encoded data.
 *
 * Returns a pointer to the start of the encoded word and fills in *endptr
 * with a pointer to the end of the encoded word, or returns NULL if nothing
 * was found.
 */
static char *decode_rfc2047_findencoded(char *str, char **endptr)
{
	char *start;
	char *end;

	for (end = str; (start = strstr(end, "=?"));) {

		/*
		 * Look for the next ? at the end of the CHARSET specifier;
		 * CHARSET cannot contain "forbidden" characters.
		 */
		for (end = start + 2; (end[0] > 32)
		     && (end[0] < 127)
		     && (strchr("()<>@,;:\"/[].=?", end[0]) == NULL);
		     end++) {
		}

		/*
		 * Check we've found the ?ENC? part, where ENC is B or Q
		 * (not case sensitive).
		 */
		if (end[0] != '?')
			continue;
		if (strchr("BQbq", end[1]) == NULL)
			continue;
		if (end[2] != '?')
			continue;

		/*
		 * Skip the DATA part.
		 */
		for (end = end + 3; (end[0] > 32)
		     && (end[0] < 127)
		     && (end[0] != '?'); end++) {
		}

		/*
		 * Check that the encoded word ends with ?= as it should.
		 */
		if ((end[0] != '?') || (end[1] != '=')) {
			end--;
			continue;
		}

		end += 2;
		*endptr = end;
		return start;
	}

	return NULL;
}


/*
 * Return a malloc()ed string containing the input string with any RFC2047
 * encoded content decoded.  The content of "len" is updated to contain the
 * size of the output string, and on entry should contain the size of the
 * input string.
 *
 * Returns NULL on error.
 */
char *decode_rfc2047(char *str, long *len)
{
	char *out;
	char *inptr;
	char *outptr;
	long bytesleft;
	int enccount;

	if (str == NULL)
		return NULL;
	if (len == NULL)
		return NULL;
	if (*len < 1)
		return NULL;
	if (str[0] == 0)
		return NULL;

	bytesleft = *len;

	out = malloc(bytesleft + 1);
	if (out == NULL)
		return NULL;

	inptr = str;
	outptr = out;
	enccount = 0;

	while ((inptr[0] != 0) && (bytesleft > 0)) {
		char *start;
		char *end;
		int n;

		/*
		 * Find the next RFC2047 encoded word.
		 */
		start = decode_rfc2047_findencoded(inptr, &end);

		/*
		 * No encoded word found - copy the remainder of the string
		 * to the output and exit the loop.
		 */
		if (start == NULL) {
			strncpy(outptr, inptr, bytesleft);
			outptr += bytesleft;
			break;
		}

		/*
		 * Copy across parts of the string before the encoded word
		 * to the output. However, we ignore whitespace between
		 * encoded words if they are all that's there (i.e. we treat
		 * "ENCWORD ENCWORD" as "ENCWORDENCWORD", but treat "ENCWORD
		 * foo ENCWORD" as "ENCWORD foo ENCWORD".
		 */
		if (start != inptr) {
			n = start - inptr;
			if ((enccount == 0)
			    || (strspn(inptr, " \t\r\n") != n)
			    ) {
				if (n > bytesleft)
					n = bytesleft;
				memcpy(outptr, inptr, n);
				outptr += n;
				bytesleft -= n;
			}
		}

		decode_rfc2047_word(start, outptr, bytesleft);

		enccount++;
		bytesleft -= (1 + end - start);
		inptr = end;
		n = strlen(outptr);
		outptr += n;
	}

	outptr[0] = 0;
	*len = strlen(out);
	return out;
}


/*
 * Scan through the message on stdin, and output the decoded version of the
 * first Subject: header line we find.
 *
 * Returns nonzero on error.
 */
int jmba_decode_subject(opts_t opts)
{
	char buf[4096];			 /* RATS: ignore (checked OK) */
	int skip;

	skip = 0;

	while (!feof(stdin) && !ferror(stdin)) {
		char *ptr;
		char *str;
		long len;

		buf[0] = 0;
		fgets(buf, sizeof(buf) - 2, stdin);

		if (skip)
			continue;

		if (strncasecmp(buf, "Subject:", 8) != 0) {
			/*
			 * Not a Subject: header, but check for
			 * blank lines, which mark the end of
			 * headers.
			 */
			if (buf[0] == '\n') {
				skip = 1;
			}
			continue;
		}

		/*
		 * Decode subject line if it's encoded.
		 */
		len = strlen(buf);
		if (len > 0) {
			str = decode_rfc2047(buf, &len);
			if (str) {
				if (len > sizeof(buf) - 2)
					len = sizeof(buf) - 2;
				strncpy(buf, str, len);
				buf[len] = 0;
				free(str);

				/*
				 * Terminate the line at the first
				 * \r or \n, in case the encoded
				 * data contained a newline.
				 */
				ptr = strchr(buf, '\n');
				if (ptr)
					ptr[1] = 0;
				ptr = strchr(buf, '\r');
				if (ptr)
					ptr[1] = 0;
			}
		}

		/*
		 * Skip the field name and leading whitespace.
		 */
		ptr = buf;
		while ((ptr[0] != 0) && (ptr[0] != ':'))
			ptr++;
		if (ptr[0] == ':')
			ptr++;
		while ((ptr[0] != 0)
		       && ((ptr[0] == ' ') || (ptr[0] == '\t')))
			ptr++;

		printf("%s", ptr);
		skip = 1;
	}

	if (ferror(stdin)) {
		log_error(opts, "%s: %s\n",
			  _("error reading message"), strerror(errno));
		return 1;
	}

	return 0;
}


#ifdef TEST
/*
 * Test program - read lines from standard input, decode any RFC2047 encoded
 * strings, and output the result on standard output.
 */
int main(int argc, char **argv)
{
	char buf[1024];			 /* RATS: ignore (OK) */

	buf[0] = 0;

	while (fgets(buf, sizeof(buf) - 2, stdin)) {
		char *str;
		long len;

		buf[sizeof(buf) - 1] = 0;

		len = strlen(buf);
		if (len < 1)
			break;

		str = decode_rfc2047(buf, &len);

		if (str == NULL) {
			fprintf(stderr, "%s: %s\n", "error",
				strerror(errno));
		} else {
			fwrite(str, 1, len, stdout);
			free(str);
		}
	}

	return 0;
}

#endif

/* EOF */
