/*
 * Global program option structure and the parsing function prototype.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifndef _OPTIONS_H
#define _OPTIONS_H 1

struct opts_s;
typedef struct opts_s *opts_t;

struct opts_s {           /* structure describing run-time options */
	char *program_name;            /* name the program is running as */
	char *homedir;                 /* current user's home directory */
	char *username;                /* current user's username */
	char *queuedir;                /* path of queue directory */
	char *discarddir;              /* path of discard directory */
	char *discardcmd;              /* command to run on discarded email */
	char *logfile;                 /* path of log file */
	int expirytime;                /* expiry time, in days */
	char *messagefile;             /* path of message template */
	char *subject;                 /* subject for autoreplies */
	int flood_num;                 /* flood threshold message count */
	int flood_kb;                  /* flood log file chunk size, kb */
	char notoself;                 /* flag, if discarding "from"="to" */
	char verbose;                  /* flag, set if we're logging */
	char action;                   /* action to perform */
	int argc;                      /* number of non-option arguments */
	char **argv;                   /* array of non-option arguments */
};

extern opts_t opts_parse(int, char **);
extern void opts_free(opts_t);
extern void log_error(opts_t, char *, ...);
extern void log_info(opts_t, char *, ...);

#endif /* _OPTIONS_H */

/* EOF */
