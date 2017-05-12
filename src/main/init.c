/*
 * Initialise the directories and files needed by this package if they are
 * not already set up.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>


static char *default_message =
    "Please just hit REPLY to this message and send a blank reply.\n"
    "\n"
    "You have sent a message which has triggered an automated junk\n"
    "mail filter.  Your message has been stored in a queue, and will\n"
    "be deleted after several days.\n"
    "\n"
    "To allow your original message to be delivered, please reply to\n"
    "this email, keeping the subject line intact and including the\n"
    "following code in the message body:\n"
    "\n"
    "  {MESSAGEKEY}\n"
    "\n"
    "This can be done in most email programs by just replying to this\n"
    "message.\n"
    "\n"
    "Part of your original message is shown below:\n"
    "\n"
    "----------------------------------------------------------------\n"
    "{ORIGINAL}\n"
    "----------------------------------------------------------------\n"
    "\n"
    "If you did not send the above message, please ignore this email.\n"
    "----------------------------------------------------------------\n"
    "(This is an automatically generated message)\n"
    "(reference: {USER}-{MESSAGECODE})\n";


/*
 * If a queue directory has not been specified, set it to the default, and
 * make sure it exists; do the same for the template file. If in either case
 * the item has been specified but doesn't exist, or another error occurs,
 * return nonzero.
 */
int jmba_init(opts_t opts)
{
	struct passwd *pw;
	struct stat sb;
	char buf[4096];			 /* RATS: ignore (checked OK) */
	FILE *fptr;

	/*
	 * Determine the current user's home directory and username.
	 */
	opts->homedir = getenv("HOME");	    /* RATS: ignore (OK) */
	opts->username = getenv("LOGNAME"); /* RATS: ignore (OK) */
	if (!opts->username)
		opts->username = getenv("USER");	/* RATS: ignore (OK) */
	if ((!opts->homedir) || (!opts->username)) {
		pw = getpwuid(geteuid());
		if (pw) {
			if (!opts->homedir)
				opts->homedir = pw->pw_dir;
			if (!opts->username)
				opts->username = pw->pw_name;
		}
	}

	/*
	 * If a queue directory has not been specified, set it to the
	 * default, and create it if it doesn't exist.
	 */
	if (!opts->queuedir) {
		if (!opts->homedir) {
			log_error(opts, "%s\n",
				  _("cannot determine home directory"));
			return 1;
		}

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf) - 2,
			 "%s/." PROGRAM_NAME "/queue", opts->homedir);
		opts->queuedir = calloc(1, strlen(buf) + 1);
		if (!opts->queuedir) {
			log_error(opts, "%s\n", strerror(errno));
			return 1;
		}
		strcpy(opts->queuedir, buf);	/* RATS: ignore (OK) */

		snprintf(buf, sizeof(buf) - 2, "%s/." PROGRAM_NAME,
			 opts->homedir);
		mkdir(buf, 0700);
		mkdir(opts->queuedir, 0700);
	}


	/*
	 * If a message file has not been specified, set it to the default,
	 * and create it if it doesn't exist.
	 */
	if (!opts->messagefile) {
		if (!opts->homedir) {
			log_error(opts, "%s\n",
				  _("cannot determine home directory"));
			return 1;
		}

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf) - 2,
			 "%s/." PROGRAM_NAME "/template", opts->homedir);
		opts->messagefile = calloc(1, strlen(buf) + 1);
		if (!opts->messagefile) {
			log_error(opts, "%s\n", strerror(errno));
			return 1;
		}
		strcpy(opts->messagefile, buf);	/* RATS: ignore (OK) */

		snprintf(buf, sizeof(buf) - 2, "%s/." PROGRAM_NAME,
			 opts->homedir);
		mkdir(buf, 0700);

		if (stat(opts->messagefile, /* RATS: ignore (race OK) */
			 &sb)) {
			fptr = fopen(opts->messagefile, "w");
			if (!fptr) {
				log_error(opts, "%s: %s: %s\n",
					  opts->messagefile,
					  _("cannot create file"),
					  strerror(errno));
				return 1;
			}
			fprintf(fptr, "%s", default_message);
			fclose(fptr);
		}
	}


	/*
	 * If a discard directory has not been specified, set it to the
	 * default.
	 */
	if (!opts->discarddir) {
		if (!opts->homedir) {
			log_error(opts, "%s\n",
				  _("cannot determine home directory"));
			return 1;
		}

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf) - 2,
			 "%s/." PROGRAM_NAME "/discarded", opts->homedir);
		opts->discarddir = calloc(1, strlen(buf) + 1);
		if (!opts->discarddir) {
			log_error(opts, "%s\n", strerror(errno));
			return 1;
		}
		strcpy(opts->discarddir, buf);	/* RATS: ignore (OK) */
	}


	/*
	 * If a log file has not been specified, set it to the default.
	 */
	if (!opts->logfile) {
		if (!opts->homedir) {
			log_error(opts, "%s\n",
				  _("cannot determine home directory"));
			return 1;
		}

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf) - 2,
			 "%s/." PROGRAM_NAME "/log", opts->homedir);
		opts->logfile = calloc(1, strlen(buf) + 1);
		if (!opts->logfile) {
			log_error(opts, "%s\n", strerror(errno));
			return 1;
		}
		strcpy(opts->logfile, buf); /* RATS: ignore (OK) */
	}


	/*
	 * Check queue directory exists, is a directory, and is readable,
	 * writable, and enterable (executable).
	 */
	if (stat(opts->queuedir, /* RATS: ignore (race OK) */ &sb)) {
		log_error(opts, "%s: %s\n",
			  opts->queuedir, strerror(errno));
		return 1;
	}

	if (!S_ISDIR(sb.st_mode)) {
		log_error(opts, "%s: %s\n",
			  opts->queuedir, _("not a directory"));
		return 1;
	}

	if (access(opts->queuedir, R_OK | W_OK | X_OK)	/* RATS: ignore (minimal TOCTOU) */
	    !=0) {
		log_error(opts, "%s: %s: %s\n",
			  opts->queuedir, _("access failed"),
			  strerror(errno));
		return 1;
	}

	/*
	 * Check message file exists, is a regular file, and is readable.
	 */
	if (stat(opts->messagefile, &sb)) {
		log_error(opts, "%s: %s\n",
			  opts->messagefile, strerror(errno));
		return 1;
	}

	if (!S_ISREG(sb.st_mode)) {
		log_error(opts, "%s: %s\n",
			  opts->messagefile, _("not a regular file"));
		return 1;
	}

	if (access(opts->messagefile, R_OK) != 0) {
		log_error(opts, "%s: %s\n",
			  opts->messagefile, strerror(errno));
		return 1;
	}

	/*
	 * Check discard directory exists, is a directory, and is readable,
	 * writable, and enterable (executable); if not, blank it.
	 */
	if (stat(opts->discarddir, /* RATS: ignore (race OK) */ &sb)) {
		opts->discarddir = NULL;
	} else if (!S_ISDIR(sb.st_mode)) {
		opts->discarddir = NULL;
	} else if (access(opts->discarddir, R_OK | W_OK | X_OK) != 0) {
		opts->discarddir = NULL;
	}

	/*
	 * Fill in the default subject if none was given.
	 */
	if (!opts->subject)
		opts->subject = "JMBA: Email queued";

	/*
	 * Set the default expiry time if none was given.
	 */
	if (opts->expirytime < 1)
		opts->expirytime = 28;

	return 0;
}

/* EOF */
