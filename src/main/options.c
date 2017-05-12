/*
 * Parse command-line options.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include "library/getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


void display_help(void);
void display_license(void);
void display_version(void);


/*
 * Free an opts_t object.
 */
void opts_free(opts_t opts)
{
	if (!opts)
		return;
	if (opts->argv)
		free(opts->argv);
	free(opts);
}


/*
 * Parse the given command-line arguments into an opts_t object, handling
 * "help", "license" and "version" options internally.
 *
 * Returns an opts_t, or 0 on error.
 *
 * Note that the contents of *argv[] (i.e. the command line parameters)
 * aren't copied anywhere, just the pointers are copied, so make sure the
 * command line data isn't overwritten or argv[1] free()d or whatever.
 */
opts_t opts_parse(int argc, char **argv)
{
#ifdef HAVE_GETOPT_LONG
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"license", 0, 0, 'l'},
		{"version", 0, 0, 'V'},
		{"queue-dir", 1, 0, 'd'},
		{"queuedir", 1, 0, 'd'},
		{"dir", 1, 0, 'd'},
		{"discard-dir", 1, 0, 'D'},
		{"discarddir", 1, 0, 'D'},
		{"discard-command", 1, 0, 'x'},
		{"discardcommand", 1, 0, 'x'},
		{"discard-cmd", 1, 0, 'x'},
		{"discardcmd", 1, 0, 'x'},
		{"expiry-time", 1, 0, 'e'},
		{"expirytime", 1, 0, 'e'},
		{"expiry", 1, 0, 'e'},
		{"expire", 1, 0, 'e'},
		{"message-file", 1, 0, 'm'},
		{"messagefile", 1, 0, 'm'},
		{"message", 1, 0, 'm'},
		{"log-file", 1, 0, 'L'},
		{"logfile", 1, 0, 'L'},
		{"log", 1, 0, 'L'},
		{"subject", 1, 0, 's'},
		{"notoself", 0, 0, 't'},
		{"no-to-self", 0, 0, 't'},
		{"no-toself", 0, 0, 't'},
		{"noto-self", 0, 0, 't'},
		{"notoself", 0, 0, 't'},
		{"allow-to-self", 0, 0, 'T'},
		{"allow-toself", 0, 0, 'T'},
		{"allowto-self", 0, 0, 'T'},
		{"allowtoself", 0, 0, 'T'},
		{"to-self", 0, 0, 'T'},
		{"toself", 0, 0, 'T'},
		{"verbose", 0, 0, 'v'},
		{"flood-check", 1, 0, 'f'},
		{"floodcheck", 1, 0, 'f'},
		{"flood", 1, 0, 'f'},
		{"no-flood-check", 0, 0, 'F'},
		{"no-floodcheck", 0, 0, 'F'},
		{"no-flood", 0, 0, 'F'},
		{"noflood-check", 0, 0, 'F'},
		{"nofloodcheck", 0, 0, 'F'},
		{"noflood", 0, 0, 'F'},
		{"queue", 0, 0, 'q'},
		{"decode-subject", 0, 0, 'S'},
		{"bounce", 0, 0, 'b'},
		{"procmail", 0, 0, 'p'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
#endif
	char *short_options = "hlVd:D:x:e:m:L:s:tTvf:FqSbp";
	char *ptr;
	int c;
	opts_t opts;

	opts = calloc(1, sizeof(*opts));
	if (!opts) {
		fprintf(stderr, "%s: %s: %s\n", argv[0],
			_("option structure allocation failed"),
			strerror(errno));
		return 0;
	}

	opts->program_name = argv[0];

	opts->argc = 0;
	opts->argv = calloc(argc + 2, sizeof(char *));
	if (!opts->argv) {
		fprintf(stderr, "%s: %s: %s\n", argv[0],
			_("option structure argv allocation failed"),
			strerror(errno));
		opts_free(opts);
		return 0;
	}

	/* Default action: store message */
	opts->action = 's';

	/* Default settings */
	opts->flood_num = 20;		    /* flood: threshold 20 messages */
	opts->flood_kb = 50;		    /* flood: check last 50kb of log */
	opts->notoself = 1;		    /* discard if sender = recipient */

	do {
#ifdef HAVE_GETOPT_LONG
		c = getopt_long(argc, argv, /* RATS: ignore */
				short_options, long_options,
				&option_index);
#else
		c = getopt(argc, argv, short_options);	/* RATS: ignore */
#endif

		if (c < 0)
			continue;

		switch (c) {
		case 'h':
			display_help();
			opts->action = 0;
			return opts;
			break;
		case 'l':
			display_license();
			opts->action = 0;
			return opts;
			break;
		case 'V':
			display_version();
			opts->action = 0;
			return opts;
			break;
			break;
		case 'd':
			opts->queuedir = optarg;
			break;
		case 'D':
			opts->discarddir = optarg;
			break;
		case 'x':
			opts->discardcmd = optarg;
			break;
		case 'e':
			opts->expirytime = atoi(optarg);
			break;
		case 'm':
			opts->messagefile = optarg;
			break;
		case 'L':
			opts->logfile = optarg;
			opts->verbose = 1;
			break;
		case 's':
			opts->subject = optarg;
			break;
		case 't':
			opts->notoself = 1;
			break;
		case 'T':
			opts->notoself = 0;
			break;
		case 'v':
			opts->verbose = 1;
			break;
		case 'f':
			ptr = strchr(optarg, ',');
			if (ptr) {
				ptr[0] = 0;
				ptr++;
				opts->flood_kb = atoi(ptr);
			}
			opts->flood_num = atoi(optarg);
			break;
		case 'F':
			opts->flood_num = 0;
			break;
		case 'q':
			opts->action = 'q';
			break;
		case 'S':
			opts->action = 'S';
			break;
		case 'b':
			opts->action = 'b';
			break;
		case 'p':
			opts->action = 'p';
			break;
		default:
#ifdef HAVE_GETOPT_LONG
			fprintf(stderr,	    /* RATS: ignore (OK) */
				_
				("Try `%s --help' for more information."),
				argv[0]);
#else
			fprintf(stderr,	    /* RATS: ignore (OK) */
				_("Try `%s -h' for more information."),
				argv[0]);
#endif
			fprintf(stderr, "\n");
			opts_free(opts);
			return 0;
			break;
		}

	} while (c != -1);

	while (optind < argc) {
		opts->argv[opts->argc++] = argv[optind++];
	}

	opts->argv[opts->argc] = NULL;

	return opts;
}

/* EOF */
