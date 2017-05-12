/*
 * Deal with a bounce message by removing the queued message it is referring
 * to.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include "md5.h"
#include "library/memmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *decode_rfc2047(char *, long *);
void jmba_queue_discard(opts_t, char *, char *);
void jmba_queued_sender(opts_t, char *, char *, int);


/*
 * Read the message from standard input and, if it is a failure notice and
 * it refers to a message in the message queue, remove the appropriate
 * message from the message queue.
 *
 * Returns nonzero on error.
 */
int jmba_process_bounce(opts_t opts)
{
	char buf[4096];			 /* RATS: ignore (checked OK) */
	char foundstr[128] = { 0, };	    /* RATS: ignore (checked OK) */
	int i, hdr, is_failure;
	struct stat sb;
	char *failstrings[] = {
		"failure",
		"Failure",
		"FAILURE",
		"Delivery Status Notification",
		"failed",
		"Failed",
		"FAILED",
		"Returned mail",
		"Returned Mail",
		"RETURNED MAIL",
		"Undelivered",
		0
	};

	/*
	 * Now we scan through the message looking for a Subject: line
	 * containing our reply subject line, and a [MSG:%32[0-9a-f]] string
	 * in the body - these would be responses to our replies, so we need
	 * to act on those.
	 */

	hdr = 1;
	is_failure = 0;
	strcpy(foundstr, "[key not found]");

	while (!feof(stdin) && !ferror(stdin)) {
		char *ptr;
		char *str;
		long len;

		buf[0] = 0;
		fgets(buf, sizeof(buf) - 2, stdin);

		if (hdr) {

			/*
			 * In headers - check subject line.
			 */
			if (strncasecmp(buf, "Subject:", 8) != 0) {
				/*
				 * Not a Subject: header, but check for
				 * blank lines, which mark the end of
				 * headers.
				 */
				if (buf[0] == '\n')
					hdr = 0;
				continue;
			}

			/*
			 * Decode subject line if it's encoded.
			 */
			len = strlen(buf);
			if (len > 0) {
				str = decode_rfc2047(buf, &len);
				if (str) {
					if (len > sizeof(buf) - 1)
						len = sizeof(buf) - 1;
					strncpy(buf, str, len);
					buf[len] = 0;
					free(str);

					/*
					 * Terminate the line after the
					 * first \r or \n, in case the
					 * encoded data contained a newline.
					 */
					ptr = strchr(buf, '\n');
					if (ptr)
						ptr[1] = 0;
					ptr = strchr(buf, '\r');
					if (ptr)
						ptr[1] = 0;
				}
			}

			for (i = 0; failstrings[i]; i++) {
				ptr =
				    memmem(buf, strlen(buf),
					   failstrings[i],
					   strlen(failstrings[i]));
				if (ptr)
					is_failure = 1;
			}

		}

		/*
		 * Look for a message key, which can appear anywhere.
		 */
		ptr = memmem(buf, strlen(buf), "[MSG:", 5);
		if (ptr)
			sscanf(ptr, "[MSG:%32[0-9a-f]]", foundstr);
	}

	if (ferror(stdin)) {
		log_error(opts, "%s: %s\n",
			  _("error reading message"), strerror(errno));
		return 1;
	}

	/*
	 * If the message isn't a failure notice (perhaps it's just a
	 * delivery delay warning, for instance) we silently discard it.
	 */
	if (!is_failure)
		return 0;

	/*
	 * Silently discard the message if we didn't find one of our message
	 * IDs inside it (i.e. the found string isn't just [0-9a-f] chars).
	 */
	for (i = 0; i < strlen(foundstr); i++) {
		if ((foundstr[0] >= '0') && (foundstr[0] <= '9'))
			continue;
		if ((foundstr[0] >= 'a') && (foundstr[0] <= 'f'))
			continue;
		return 0;
	}

	/*
	 * Complain and abort if we found a message ID, but it refers to a
	 * nonexistent queue entry.
	 */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 2, "%s/%s", opts->queuedir, foundstr);
	if (stat(buf, &sb)		    /* RATS: ignore (race OK) */
	    ||!S_ISREG(sb.st_mode)) {
		log_error(opts, "%s: %s: %s\n", foundstr,
			  _
			  ("bounce received containing invalid message ID"),
			  strerror(errno));
		return 1;
	}

	/*
	 * Finally, delete the queue entry and discard the message. If we
	 * can, link the message to the discard directory first, so a copy
	 * is retained.
	 */
	/*
	 * If logging is switched on, we read the header of the queued
	 * message we're about to discard, so we can log the original
	 * sender address.
	 */
	if (opts->verbose) {
		char origsender[256];	 /* RATS: ignore (checked) */

		jmba_queued_sender(opts, buf, origsender,
				   sizeof(origsender));
		log_info(opts, "%s: %s: %s\n", foundstr,
			 _("discarded (bounced)"), origsender);
	}
	jmba_queue_discard(opts, foundstr, buf);
	unlink(buf);

	return 0;
}

/* EOF */
