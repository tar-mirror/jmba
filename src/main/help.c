/*
 * Output command-line help to stdout.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct optdesc_s {
	char *optshort;
	char *optlong;
	char *param;
	char *description;
};


/*
 * Display command-line help.
 */
void display_help(void)
{
	struct optdesc_s optlist[] = {
		{"-d", "--queue-dir", "DIR",
		 _("store queued messages in DIR")},
		{"-e", "--expiry-time", "DAYS",
		 _("expire messages after DAYS days")},
		{"-m", "--message-file", "FILE",
		 _("use FILE as the message template")},
		{"-D", "--discard-dir", "DIR",
		 _("store discarded messages in DIR")},
		{"-x", "--discard-command", "COMMAND",
		 _("run COMMAND on each discarded message")},
		{"-L", "--log-file", "FILE",
		 _("log messages to FILE")},
		{"-s", "--subject", "STRING",
		 _("use STRING as the reply subject")},
		{"-T", "--allow-to-self", 0,
		 _("reply even if \"from\" equals \"to\" address")},
		{"-t", "--no-to-self", 0,
		 _("discard if \"from\" = \"to\" (default)")},
		{"-v", "--verbose", 0,
		 _("write logging information")},
		{"-f", "--flood-check", "NUM[,KB]",
		 _
		 ("discard if NUM queued from same sender in last KB of logs (default 20,50)")},
		{"-F", "--no-flood-check", 0,
		 _("do not perform flood checking")},
		{"", 0, 0, 0},
		{"-b", "--bounce", 0,
		 _("process a bounced email")},
		{"-q", "--queue", 0,
		 _("output verified queued messages")},
		{"-S", "--decode-subject", 0,
		 _("output decoded subject of email")},
		{"-p", "--procmail", 0,
		 _("output an example procmail recipe")},
		{"", 0, 0, 0},
		{"-h", "--help", 0,
		 _("show this help and exit")},
		{"-l", "--license", 0,
		 _("show this program's license")},
		{"-V", "--version", 0,
		 _("show version information and exit")},
		{0, 0, 0, 0}
	};
	int i, col1max = 0, tw = 77;
	char *optbuf;

	printf(_("Usage: %s [OPTION]..."),  /* RATS: ignore */
	       PROGRAM_NAME);
	printf("\n%s\n\n",
	       _
	       ("Read the message on standard input and output a response\n"
		"on standard output."));

	for (i = 0; optlist[i].optshort; i++) {
		int width = 0;

		width = 2 + strlen(optlist[i].optshort);	/* RATS: ignore */
#ifdef HAVE_GETOPT_LONG
		if (optlist[i].optlong)
			width += 2 + strlen(optlist[i].optlong);	/* RATS: ignore */
#endif
		if (optlist[i].param)
			width += 1 + strlen(optlist[i].param);	/* RATS: ignore */

		if (width > col1max)
			col1max = width;
	}

	col1max++;

	optbuf = malloc(col1max + 16);
	if (optbuf == NULL) {
		fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
		exit(1);
	}

	for (i = 0; optlist[i].optshort; i++) {
		char *start;
		char *end;

		if (optlist[i].optshort[0] == 0) {
			printf("\n");
			continue;
		}

		snprintf(optbuf, col1max + 14, "%s%s%s%s%s",
			 optlist[i].optshort,
#ifdef HAVE_GETOPT_LONG
			 optlist[i].optlong ? ", " : "",
			 optlist[i].optlong ? optlist[i].optlong : "",
#else
			 "", "",
#endif
			 optlist[i].param ? " " : "",
			 optlist[i].param ? optlist[i].param : "");

		printf("  %-*s ", col1max - 2, optbuf);

		if (optlist[i].description == NULL) {
			printf("\n");
			continue;
		}

		start = optlist[i].description;

		while (strlen(start) /* RATS: ignore */ >tw - col1max) {
			end = start + tw - col1max;
			while ((end > start) && (end[0] != ' '))
				end--;
			if (end == start) {
				end = start + tw - col1max;
			} else {
				end++;
			}
			printf("%.*s\n%*s ", (int) (end - start), start,
			       col1max, "");
			if (end == start)
				end++;
			start = end;
		}

		printf("%s\n", start);
	}

	printf("\n");
	printf(_("Please report any bugs to %s."),	/* RATS: ignore */
	       BUG_REPORTS_TO);
	printf("\n");
}

/* EOF */
