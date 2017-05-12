/*
 * Store a message in the queue, and output a reply.
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
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

char *decode_rfc2047(char *, long *);
void jmba_queue_discard(opts_t, char *, char *);

static char jmba__tmpname[4096] = { 0, };   /* RATS: ignore (checked OK) */
static char jmba__leafname[128] = { 0, };   /* RATS: ignore (checked OK) */
static char jmba__fullname[4096] = { 0, };  /* RATS: ignore (checked OK) */
static char jmba__foundstr[128] = { 0, };   /* RATS: ignore (checked OK) */
static char jmba__origmsg[4096] = { 0, };   /* RATS: ignore (checked OK) */
static char jmba__addrfrom[256] = { 0, };   /* RATS: ignore (checked OK) */
static char jmba__addrto[256] = { 0, };	    /* RATS: ignore (checked OK) */
static int jmba__isreply = 0;


/*
 * Replace each occurrence of "key" with "val" in the given buffer of the
 * given size.
 */
static void jmba__keyreplace(char *buf, size_t size, char *key, char *val)
{
	int keylen, valuelen, offs;
	char *start;
	char *ptr;

	if (buf == NULL)
		return;
	if (key == NULL)
		return;
	if (val == NULL)
		return;

	keylen = strlen(key);
	valuelen = strlen(val);

	start = buf;
	while (start) {
		ptr = memmem(start, strlen(start), key, keylen);
		if (ptr == NULL)
			return;

		start = ptr;
		offs = start - buf;

		/*
		 * Check there's enough room to insert the value.
		 */
		if ((offs + valuelen + strlen(ptr) - keylen) >= (size - 1))
			return;

		memmove(ptr + valuelen, ptr + keylen,
			1 + strlen(ptr) - keylen);
		memcpy(ptr, val, valuelen);

		start += valuelen;
	}
}


/*
 * Fill in "buf" (max size "bufsz") with the email address contained within
 * "line", if there is one ("buf" is not modified if there isn't or if the
 * address wouldn't fit in "buf").
 *
 * Note that we don't follow the RFC here, we just look for the last @ and
 * work backwards and forwards from there, since that will catch the vast
 * majority of cases and is simplest.
 */
void jmba_addrheader(char *buf, int bufsz, char *line)
{
	long i, a, b, size;

	if (line == NULL)
		return;
	if (buf == NULL)
		return;

	size = strlen(line);

	for (i = size - 1; i > 0 && line[i] != '@'; i--) {
	}

	if (line[i] != '@')
		return;

	for (a = i; a > 0 && line[a] != '<' && line[a] > 32; a--) {
	}
	if ((line[a] == '<') || (line[a] <= 32))
		a++;

	for (b = i; b < size && line[b] != '>' && line[b] > 32; b++) {
	}
	if ((line[b] == '>') || (line[b] <= 32))
		b--;

	if (b < (a + 2))
		return;

	if ((2 + b - a) > bufsz)
		return;

	memset(buf, 0, bufsz);
	strncpy(buf, line + a, 1 + b - a);
}


/*
 * Fill in "addrbuf" (max size "bufsz") with the email address of the sender
 * of the email stored in the given file. Fills in "<sender unknown>" if the
 * sender cannot be determined, the file cannot be opened, etc.
 */
void jmba_queued_sender(opts_t opts, char *file, char *addrbuf, int bufsz)
{
	FILE *fptr;
	char line[4096];		 /* RATS: ignore (checked) */

	strncpy(addrbuf, _("<sender unknown>"), bufsz - 1);
	addrbuf[bufsz - 1] = 0;

	fptr = fopen(file, "r");
	if (!fptr)
		return;

	while (!feof(fptr) && !ferror(fptr)) {
		line[0] = 0;
		fgets(line, sizeof(line) - 2, fptr);
		line[sizeof(line) - 1] = 0;
		if (strncasecmp(line, "From:", 5) == 0) {
			jmba_addrheader(addrbuf, bufsz, line);
			fclose(fptr);
			return;
		}
		if (line[0] == '\n') {
			fclose(fptr);
			return;
		}
	}

	fclose(fptr);
}


/*
 * Safely store the message on standard input to a file in the queue
 * directory. Returns nonzero, and cleans up, on error.
 */
static int jmba__storetoqueue(opts_t opts)
{
	char *hostname;
	struct utsname uts;
	char buf[4096];			 /* RATS: ignore (checked OK) */
	struct stat sb;
	ssize_t got;
	int fd, i;

	hostname = "unknown";
	if (uname(&uts) == 0)
		hostname = uts.nodename;

	fd = -1;

	for (i = 0; i < 4; i++) {
		unsigned char digest[32];	/* RATS: ignore (checked OK) */
		MD5_CTX ctx;
		int j;

		if (i > 0)
			sleep(2);

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf) - 2, "%ld-%d-%s", time(NULL),
			 getpid(), hostname);

		MD5Init(&ctx);
		MD5Update(&ctx, buf, strlen(buf));
		MD5Final(digest, &ctx);

		jmba__leafname[0] = 0;

		for (j = 0; j < 16; j++) {
			snprintf(buf, sizeof(buf) - 2, "%02x", digest[j]);
			strcat(jmba__leafname, buf);	/* RATS: ignore (OK) */
		}

		memset(jmba__tmpname, 0, sizeof(jmba__tmpname));
		memset(jmba__fullname, 0, sizeof(jmba__fullname));

		snprintf(jmba__tmpname, sizeof(jmba__tmpname) - 2,
			 "%s/.%s", opts->queuedir, jmba__leafname);
		snprintf(jmba__fullname, sizeof(jmba__fullname) - 2,
			 "%s/%s", opts->queuedir, jmba__leafname);

		if (stat(jmba__fullname, &sb) == 0)	/* RATS: ignore (OK) */
			continue;

		fd = open(jmba__tmpname, O_WRONLY | O_CREAT | O_EXCL,
			  S_IRUSR | S_IWUSR);
		if (fd >= 0)
			break;
	}

	if (fd < 0) {
		log_error(opts, "%s: %s\n",
			  _("failed to find a unique name"),
			  strerror(errno));
		return 1;
	}

	do {
		char *ptr;
		ssize_t wrote, todo;

		got = read(STDIN_FILENO,    /* RATS: ignore (checked OK) */
			   buf, sizeof(buf));
		ptr = buf;
		todo = got;
		while (todo > 0) {
			wrote = write(fd, ptr, todo);
			if ((wrote < 0) || (fsync(fd) != 0)) {
				log_error(opts, "%s: %s\n",
					  _("failed to store message"),
					  strerror(errno));
				unlink(jmba__tmpname);
				close(fd);
			}
			todo -= wrote;
		}
	} while (got > 0);

	if (fchmod(fd, S_IRUSR | S_IWUSR) || close(fd)
	    || link(jmba__tmpname, jmba__fullname)) {
		log_error(opts, "%s: %s\n",
			  _("failed to store message"), strerror(errno));
		unlink(jmba__tmpname);
		return 1;
	}

	return 0;
}


/*
 * Scan through the message we have just stored looking for a Subject: line
 * containing our reply subject line and a [MSG:%32[0-9a-f]] string in the
 * body; if we see these, we set jmba__isreply since the message must be a
 * response to our reply. Otherwise, we fill in jmba__origmsg with an
 * excerpt from this message (a few headers and part of the body).
 *
 * Returns nonzero, and cleans up, on error.
 */
static int jmba__scanmessage(opts_t opts)
{
	char buf[4096];			 /* RATS: ignore (checked OK) */
	int hdr, lines, len;
	FILE *fptr;

	fptr = fopen(jmba__fullname, "r");
	if (!fptr) {
		log_error(opts, "%s: %s\n",
			  _("failed to store message"), strerror(errno));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	hdr = 1;
	lines = 0;
	jmba__isreply = 0;
	jmba__origmsg[0] = 0;
	jmba__addrfrom[0] = 0;
	jmba__addrto[0] = 0;
	strcpy(jmba__foundstr, "[key not found]");

	while (!feof(fptr) && !ferror(fptr)) {
		char insertbuf[1024];	 /* RATS: ignore (checked OK) */
		char *ptr;
		char *str;
		long len;

		buf[0] = 0;
		fgets(buf, sizeof(buf) - 2, fptr);

		if (hdr) {

			if ((strncasecmp(buf, "From:", 5) == 0)
			    || (strncasecmp(buf, "To:", 3) == 0)
			    || (strncasecmp(buf, "Date:", 5) == 0)
			    || (strncasecmp(buf, "Subject:", 8) == 0)
			    ) {
				snprintf(insertbuf, sizeof(insertbuf) - 2,
					 "> %.80s", buf);
				if (strchr(insertbuf, '\n') == NULL)
					strcat(insertbuf, "\n");	/* RATS: ignore */
				if (strlen(jmba__origmsg) < 640)
					strcat(jmba__origmsg, insertbuf);	/* RATS: ignore */
			}

			/*
			 * Record the "from" and "to" email addresses.
			 */
			if (strncasecmp(buf, "From:", 5) == 0) {
				jmba_addrheader(jmba__addrfrom,
						sizeof(jmba__addrfrom),
						buf);
			} else if (strncasecmp(buf, "To:", 3) == 0) {
				jmba_addrheader(jmba__addrto,
						sizeof(jmba__addrto), buf);
			}

			/*
			 * In headers - look for our subject line.
			 */
			if (strncasecmp(buf, "Subject:", 8) != 0) {
				/*
				 * Not a Subject: header, but check for
				 * blank lines, which mark the end of
				 * headers.
				 */
				if (buf[0] == '\n') {
					strcat(jmba__origmsg, ">\n");
					hdr = 0;
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

			ptr =
			    memmem(buf, strlen(buf), opts->subject,
				   strlen(opts->subject));
			if (ptr) {
				jmba__isreply = 1;

				/*
				 * Check for message key in the subject line.
				 */
				ptr = memmem(buf, strlen(buf), "[MSG:", 5);
				if (ptr)
					sscanf(ptr, "[MSG:%32[0-9a-f]]",
					       jmba__foundstr);
			}

		} else if (jmba__isreply) {

			/*
			 * Found our subject line - look for message key.
			 */
			ptr = memmem(buf, strlen(buf), "[MSG:", 5);
			if (ptr)
				sscanf(ptr, "[MSG:%32[0-9a-f]]",
				       jmba__foundstr);
		} else if (lines < 4) {
			snprintf(insertbuf, sizeof(insertbuf) - 2,
				 "> %.80s", buf);
			if (strchr(insertbuf, '\n') == NULL)
				strcat(insertbuf, "\n");
			if (strlen(insertbuf) < 100)
				strcat(jmba__origmsg, insertbuf);	/* RATS: ignore */
			lines++;
		}
	}

	if (ferror(fptr)) {
		log_error(opts, "%s: %s\n",
			  _("error reading message"), strerror(errno));
		fclose(fptr);
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	fclose(fptr);

	/*
	 * Strip final \n from jmba__origmsg, so "{ORIGINAL}" on a line on
	 * its own won't produce an extra blank line.
	 */
	len = strlen(jmba__origmsg);
	if (len > 0) {
		if (jmba__origmsg[len - 1] == '\n')
			jmba__origmsg[len - 1] = 0;
	}

	return 0;
}


/*
 * Process a reply to one of our emails by marking the message it refers to
 * as OK, if the code is valid; if it is not, we complain. Cleans up on
 * exit.
 */
static int jmba__processreply(opts_t opts)
{
	char buf[4096];			 /* RATS: ignore (checked OK) */
	struct stat sb;

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 2, "%s/%s", opts->queuedir,
		 jmba__foundstr);

	if (stat(buf, &sb)		    /* RATS: ignore (race OK) */
	    ||!S_ISREG(sb.st_mode)
	    || chmod(buf, S_IRUSR)) {
		log_error(opts, "%s: %s: %s\n",
			  jmba__foundstr,
			  _("reply has invalid message ID"),
			  strerror(errno));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	if (opts->verbose) {
		char origsender[256];	 /* RATS: ignore (checked) */

		jmba_queued_sender(opts, buf, origsender,
				   sizeof(origsender));
		log_info(opts, "%s: %s: %s\n", jmba__foundstr,
			 _("unlocked"), origsender);
	}

	unlink(jmba__fullname);
	unlink(jmba__tmpname);
	return 0;
}


/*
 * Check the last opts->flood_kb of logs to see if there have been
 * opts->flood_num or more messages queued (and not yet dequeued) for the
 * same sender.
 *
 * Returns zero if no match, no log, or opts->flood_num < 1, and nonzero if
 * there is a match (i.e. if flooding is occurring).
 */
static int jmba__floodcheck(opts_t opts)
{
	FILE *fptr;
	long readamount, logsize;
	long count;

	if (opts->flood_num < 1) {
		return 0;
	}
	if (!opts->logfile) {
		return 0;
	}

	fptr = fopen(opts->logfile, "r");
	if (!fptr) {
		return 0;
	}

	readamount = opts->flood_kb * 1024;

	fseek(fptr, 0, SEEK_END);
	logsize = ftell(fptr);
	if (logsize < readamount) {
		fseek(fptr, 0, SEEK_SET);
	} else {
		fseek(fptr, 0 - readamount, SEEK_END);
	}

	count = 0;

	while (!feof(fptr)) {
		char line[1024];	 /* RATS: ignore (checked) */
		char message[64];	 /* RATS: ignore (bounded) */
		char address[1024];	 /* RATS: ignore (bounded) */

		line[0] = 0;
		fgets(line, sizeof(line) - 2, fptr);
		line[sizeof(line) - 1] = 0;

		message[0] = 0;
		address[0] = 0;

		if (sscanf
		    (line,
		     "[%*[^]]] %*[0-9a-f]: %60[^:]: %999[^\n]",
		     message, address) < 2)
			continue;
		message[sizeof(message) - 1] = 0;
		address[sizeof(address) - 1] = 0;

		if (strcasecmp(address, jmba__addrfrom) != 0)
			continue;

		if (strcmp(message, _("queued")) == 0) {
			count++;
		} else if (strcmp(message, _("unlocked")) == 0) {
			count--;
		} else if (strcmp(message, _("discarded (bounced)")) == 0) {
			/*
			 * If the message came from a bouncing address, we
			 * count it towards a flood, rather than treating it
			 * as a normal message. So we don't decrement the
			 * count here.
			 */
			/* count--; */
		}
	}

	fclose(fptr);

	if (count >= opts->flood_num)
		return 1;

	return 0;
}


/*
 * Run "formail" with the message as stdin in a child process, to generate
 * the header of the reply email on stdout. Returns nonzero, and cleans up,
 * on error.
 */
static int jmba__replyheader(opts_t opts)
{
	int fd, status;
	pid_t child;

	fd = open(jmba__fullname, O_RDONLY);
	if (fd < 0) {
		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("failed to store message"), strerror(errno));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	child = fork();
	if (child < 0) {
		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("failed to store message"), strerror(errno));
		close(fd);
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	/*
	 * Child process - run formail and exit.
	 */
	if (child == 0) {
		char subject[4096];	 /* RATS: ignore (checked OK) */

		memset(subject, 0, sizeof(subject));
		snprintf(subject, sizeof(subject) - 2,
			 "Subject: %s [MSG:%s]", opts->subject,
			 jmba__leafname);

		if (dup2(fd, STDIN_FILENO))
			exit(1);
		close(fd);

		execlp(FORMAIL, FORMAIL, "-r", "-I", subject, NULL);

		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("failed to run formail"), strerror(errno));

		exit(1);
	}

	/*
	 * Parent process - wait for child to exit, and check exit code.
	 */
	close(fd);
	status = 0;
	if (waitpid(child, &status, 0) != child) {
		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("failed to run formail"), strerror(errno));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}
	if ((!WIFEXITED(status)) || (WEXITSTATUS(status) != 0)) {
		log_error(opts, "%s: %s\n", jmba__leafname,
			  _("failed to run formail - nonzero exit code"));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	return 0;
}


/*
 * Output a reply message body on stdout, based on the reply template.
 * Returns nonzero, and cleans up, on error.
 */
static int jmba__replybody(opts_t opts)
{
	char *username;
	char buf[8192];			 /* RATS: ignore (checked OK) */
	char insertbuf[4096];		 /* RATS: ignore (checked OK) */
	struct passwd *pw;
	FILE *fptr;

	fptr = fopen(opts->messagefile, "r");
	if (!fptr) {
		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("failed to read message template"),
			  strerror(errno));
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	username = "postmaster";
	pw = getpwuid(geteuid());
	if (pw)
		username = pw->pw_name;

	while (!feof(fptr) && !ferror(fptr)) {
		buf[0] = 0;
		fgets(buf, (sizeof(buf) >> 1) - 2, fptr);
		buf[sizeof(buf) - 1] = 0;

		snprintf(insertbuf, sizeof(insertbuf) - 2, "[MSG:%.999s]",
			 jmba__leafname);
		jmba__keyreplace(buf, sizeof(buf), "{MESSAGEKEY}",
				 insertbuf);

		snprintf(insertbuf, sizeof(insertbuf) - 2, "%.999s",
			 jmba__leafname);
		jmba__keyreplace(buf, sizeof(buf), "{MESSAGECODE}",
				 insertbuf);

		snprintf(insertbuf, sizeof(insertbuf) - 2, "%.999s",
			 username);
		jmba__keyreplace(buf, sizeof(buf), "{USER}", insertbuf);

		snprintf(insertbuf, sizeof(insertbuf) - 2, "%.999s",
			 jmba__addrfrom);
		if (jmba__addrfrom[0] == 0) {
			snprintf(insertbuf, sizeof(insertbuf) - 2,
				 "%.999s", _("<unknown>"));
		}
		jmba__keyreplace(buf, sizeof(buf), "{SENDER}", insertbuf);

		snprintf(insertbuf, sizeof(insertbuf) - 2, "%.999s",
			 jmba__addrto);
		if (jmba__addrto[0] == 0) {
			snprintf(insertbuf, sizeof(insertbuf) - 2,
				 "%.999s", _("<unknown>"));
		}
		jmba__keyreplace(buf, sizeof(buf), "{RECIPIENT}",
				 insertbuf);

		jmba__keyreplace(buf, sizeof(buf), "{ORIGINAL}",
				 jmba__origmsg);

		printf("%s", buf);
	}

	if (ferror(fptr)) {
		log_error(opts, "%s: %s: %s\n", jmba__leafname,
			  _("error reading message template"),
			  strerror(errno));
		fclose(fptr);
		unlink(jmba__fullname);
		unlink(jmba__tmpname);
		return 1;
	}

	fclose(fptr);

	return 0;
}


/*
 * Store the message on standard input in the message queue, and output a
 * reply message on standard output.
 *
 * Returns nonzero on error.
 */
int jmba_store_message(opts_t opts)
{
	if (jmba__storetoqueue(opts))
		return 1;

	/*
	 * At this point, we have (reasonably safely) delivered the message
	 * on stdin to the file "jmba__fullname".  We can now process it. 
	 * However, we'll keep the linked temporary file (jmba__tmpname) around
	 * until we're finished processing, in case there are any errors, to
	 * minimise the risk of duplicate filenames.
	 */

	if (jmba__scanmessage(opts))
		return 1;

	if (jmba__isreply) {
		if (jmba__processreply(opts))
			return 1;
		return 0;
	}

	/*
	 * Once we get to this point, we know that we are dealing with a new
	 * message, not a reply, so we create a reply email on stdout.
	 */

	/*
	 * First, check whether the "from" and "to" addresses are the same,
	 * and if we have been told to discard email in this case, discard
	 * it.
	 */
	if ((opts->notoself) && (jmba__addrfrom[0] != 0)
	    && (strcasecmp(jmba__addrfrom, jmba__addrto) == 0)) {
		log_info(opts, "%s: %s: %s\n", jmba__leafname,
			 _("discarded (sender = recipient)"),
			 jmba__addrfrom);
		jmba_queue_discard(opts, jmba__leafname, jmba__fullname);
		unlink(jmba__tmpname);
		return 0;
	}

	/*
	 * Next, check to see if the sender address has been flooding us
	 * with messages, by checking for lots of other queued messages
	 * caused by the same sender; if this is the case, discard the
	 * message.
	 */
	if (jmba__floodcheck(opts)) {
		log_info(opts, "%s: %s: %s\n", jmba__leafname,
			 _("discarded (flood detected)"), jmba__addrfrom);
		jmba_queue_discard(opts, jmba__leafname, jmba__fullname);
		unlink(jmba__tmpname);
		return 0;
	}

	if (jmba__replyheader(opts))
		return 1;

	if (jmba__replybody(opts))
		return 1;

	log_info(opts, "%s: %s: %s\n", jmba__leafname,
		 _("queued"), jmba__addrfrom);

	/*
	 * We're done - now delete the temporary version of the file.
	 */

	unlink(jmba__tmpname);

	return 0;
}

/* EOF */
