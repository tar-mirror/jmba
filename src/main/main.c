/*
 * Main program entry point - read the command line options, then perform
 * the appropriate actions.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int jmba_init(opts_t);
int jmba_store_message(opts_t);
int jmba_process_bounce(opts_t);
int jmba_run_queue(opts_t);
int jmba_decode_subject(opts_t);
int jmba_example_procmailrc(opts_t);


/*
 * Process command-line arguments and set option flags, then call functions
 * to initialise, and finally run the function appropriate to the action.
 */
int main(int argc, char **argv)
{
	opts_t opts;
	int retcode = 0;

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	opts = opts_parse(argc, argv);
	if (!opts)
		return 1;

	if (jmba_init(opts)) {
		retcode = 1;
		opts->action = 0;
	}

	switch (opts->action) {
	case 's':
		retcode = jmba_store_message(opts);
		break;
	case 'b':
		retcode = jmba_process_bounce(opts);
		break;
	case 'q':
		retcode = jmba_run_queue(opts);
		break;
	case 'S':
		retcode = jmba_decode_subject(opts);
		break;
	case 'p':
		retcode = jmba_example_procmailrc(opts);
		break;
	default:
		break;
	}

	opts_free(opts);

	return retcode;
}

/* EOF */
