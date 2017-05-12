/*
 * Deliver any messages in the queue that have been marked OK.
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
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

void jmba_queued_sender(opts_t, char *, char *, int);


/*
 * Discard a message from the queue, linking it into the discard directory
 * in the process if this is applicable.
 */
void jmba_queue_discard(opts_t opts, char *leaf, char *fullpath)
{
	char discardbuf[4096];		 /* RATS: ignore (checked OK) */
	struct stat sb;
	struct tm *t;

	/*
	 * Run the discard-command, if one has been defined.
	 */
	if (opts->discardcmd) {
		pid_t child;

		child = fork();
		if (child < 0) {
			log_error(opts, "%s: %s\n", "fork",
				  strerror(errno));
		} else if (child > 0) {
			waitpid(child, NULL, 0);
		} else {
			int fd;

			fd = open(fullpath, O_RDONLY);
			if (fd < 0) {
				log_error(opts, "%s: %s: %s\n", leaf,
					  "open", strerror(errno));
				exit(1);
			}
			dup2(fd, STDIN_FILENO);
			close(fd);

			execl("/bin/sh", "sh", "-c", opts->discardcmd, 0);
			log_error(opts, "%s: %s: %s\n", leaf, "exec",
				  strerror(errno));
			exit(1);
		}
	}

	if (opts->discarddir == NULL) {
		unlink(fullpath);
		return;
	}

	if (stat(fullpath, &sb)) {	    /* RATS: ignore (only want mtime) */
		unlink(fullpath);
		return;
	}

	t = localtime(&(sb.st_mtime));

	memset(discardbuf, 0, sizeof(discardbuf));
	if (t) {
		snprintf(discardbuf, sizeof(discardbuf) - 2,
			 "%s/%04d%02d%02d-%s", opts->discarddir,
			 t->tm_year + 1900,
			 t->tm_mon + 1, t->tm_mday, leaf);
	} else {
		snprintf(discardbuf, sizeof(discardbuf) - 2,
			 "%s/%s", opts->discarddir, leaf);
	}

	if (link(fullpath, discardbuf)) {
		log_error(opts, "%s: %s: %s\n", leaf,
			  _("failed to copy to discard directory"),
			  strerror(errno));
	}

	unlink(fullpath);
}


/*
 * Go through the message queue and deliver any messages which are marked OK
 * (i.e. have their "write" bit clear). Messages which have been in the
 * queue for too long will be discarded.
 *
 * Returns nonzero on error.
 */
int jmba_run_queue(opts_t opts)
{
	DIR *dptr;
	struct dirent *d;

	dptr = opendir(opts->queuedir);	    /* RATS: ignore (OK) */
	if (!dptr) {
		log_error(opts, "%s: %s\n",
			  opts->queuedir, strerror(errno));
		return 1;
	}

	while ((d = readdir(dptr)) != NULL) {
		char fullpath[4096];	 /* RATS: ignore (checked OK) */
		struct stat sb;
		pid_t child;
		int status;
		int fd;

		/* Skip "hidden" files */
		if (d->d_name[0] == '.')
			continue;

		memset(fullpath, 0, sizeof(fullpath));
		snprintf(fullpath,	    /* RATS: ignore (OK) */
			 sizeof(fullpath) - 2, "%s/%s",
			 opts->queuedir, d->d_name);

		if (stat(fullpath, &sb))    /* RATS: ignore (race OK) */
			continue;
		if (!S_ISREG(sb.st_mode))
			continue;
		if ((sb.st_mode & S_IRUSR) == 0)
			continue;

		/*
		 * If the file is readable and writable, meaning it is still
		 * in the queue waiting for the sender to validate it, we
		 * check its last-modification time; if it is older than the
		 * expiry time, we delete it.
		 */
		if ((sb.st_mode & S_IWUSR) != 0) {
			double seconds_old;
			int days_old;

			seconds_old = difftime(time(NULL), sb.st_mtime);

			days_old = seconds_old / 86400;

			if (days_old >= opts->expirytime) {
				if (opts->verbose) {
					char origsender[256];	/* RATS: ignore (checked) */

					jmba_queued_sender(opts, fullpath,
							   origsender,
							   sizeof
							   (origsender));

					log_info(opts, "%s: %s: %s\n",
						 d->d_name,
						 _("discarded (expired)"),
						 origsender);
				}
				jmba_queue_discard(opts, d->d_name,
						   fullpath);
			}

			continue;
		}

		fd = open(fullpath, O_RDONLY);
		if (fd < 0)
			continue;

		child = fork();		    /* RATS: ignore (OK) */
		if (child < 0) {
			log_error(opts, "%s: %s: %s\n", d->d_name,
				  _("fork failed"), strerror(errno));
			close(fd);
			closedir(dptr);
			return 1;
		}

		/*
		 * Child process - run delivery program and exit.
		 */
		if (child == 0) {
			if (dup2(fd, STDIN_FILENO))
				exit(1);
			close(fd);

			if (opts->argv[0] != NULL) {
				execvp(	    /* RATS: ignore (OK) */
					      opts->argv[0], opts->argv);
				log_error(opts, "%s: %s: %s: %s\n",
					  d->d_name,
					  _("failed to run"),
					  opts->argv[0], strerror(errno));
			} else {
				execlp(	    /* RATS: ignore (OK) */
					      PROCMAIL, PROCMAIL, "-a",
					      "JMBAPASSTHROUGH", "-d",
					      opts->username, NULL);
				log_error(opts, "%s: %s: %s\n",
					  d->d_name,
					  _("failed to run procmail"),
					  strerror(errno));
			}

			exit(1);
		}

		/*
		 * Parent process - wait for child to exit, and check exit code.
		 */
		close(fd);
		status = 0;
		if (waitpid(child, &status, 0) != child) {
			log_error(opts, "%s: %s: %s\n", d->d_name,
				  _("delivery failed"), strerror(errno));
			continue;
		}

		if ((!WIFEXITED(status)) || (WEXITSTATUS(status) != 0)) {
			log_error(opts, "%s: %s\n", d->d_name,
				  _
				  ("delivery failed with nonzero exit code"));
			continue;
		}

		/*
		 * If we reach this point, the message has been successfully
		 * delivered by the delivery program, so we can delete it.
		 */
		if (opts->verbose) {
			char origsender[256];	/* RATS: ignore (checked) */

			jmba_queued_sender(opts, fullpath, origsender,
					   sizeof(origsender));

			log_info(opts, "%s: %s: %s\n", d->d_name,
				 _("delivered"), origsender);
		}

		unlink(fullpath);	    /* RATS: ignore (no TOCTOU) */
	}

	closedir(dptr);

	return 0;
}

/* EOF */
