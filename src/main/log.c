/*
 * Logging functions.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/*
 * Perform logging with the given data, if logging is enabled.
 */
static void log__write(opts_t opts, char *prefix, char *format, va_list ap)
{
	char timebuf[256];		 /* RATS: ignore (checked OK) */
	struct timeval tv;
	struct timezone tz;
	time_t t;
	struct tm *tm;
	FILE *fptr;
	struct flock lock;

	if (!opts)
		return;
	if (!opts->verbose)
		return;
	if (!opts->logfile)
		return;

	fptr = fopen(opts->logfile, "a");
	if (!fptr)
		return;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(fileno(fptr), F_SETLKW, &lock);

	gettimeofday(&tv, &tz);

	t = tv.tv_sec;
	tm = localtime(&t);
	timebuf[0] = 0;

	strftime(timebuf, sizeof(timebuf) - 1, "%Y-%m-%d %H:%M:%S", tm);

	fprintf(fptr, "[%s.%06ld] %s", timebuf, tv.tv_usec, prefix);
	vfprintf(fptr, format, ap);	    /* RATS: ignore */

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(fileno(fptr), F_SETLK, &lock);

	fclose(fptr);
}


/*
 * Report an error to stderr, and also log it to the log file if logging is
 * enabled.
 */
void log_error(opts_t opts, char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if (opts)
		fprintf(stderr, "%s: ", opts->program_name);
	vfprintf(stderr, format, ap);	    /* RATS: ignore */
	log__write(opts, _("ERROR: "), format, ap);
	va_end(ap);
}


/*
 * Output logging information to the log file, if logging is active.
 */
void log_info(opts_t opts, char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	log__write(opts, "", format, ap);
	va_end(ap);
}

/* EOF */
