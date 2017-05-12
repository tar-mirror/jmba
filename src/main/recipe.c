/*
 * Output an example procmail recipe.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include "options.h"
#include <stdio.h>

int jmba_example_procmailrc(opts_t opts)
{
	printf("#\n");
	printf("# %s\n", _("Example procmail recipe"));
	printf("#\n");
	printf("# %s\n", _("Put this AFTER rules for mailing lists,"));
	printf("# %s\n", _("so that we don't trip up on them!"));
	printf("#\n");
	printf("\n");

	printf("# %s\n", _("First, we'll set up some variables."));
	printf("#\n");
	printf("%s%s=\"%s\"\n", UCPACKAGE, _("MAINEMAIL"),
	       _("your@own.mail.address"));
	printf("%s%s=\"%s\"\n", UCPACKAGE, _("SUBJECT"),
	       _("Email queued by " UCPACKAGE));
	printf("%s=$1\n", _("TESTPASS"));
	printf("%s=`%s -S`\n", _("SUBJECT"), PROGRAM_NAME);
	printf("\n");

	printf("# %s\n", _("Step 1:"));
	printf("# %s\n",
	       _("Filter messages only if the pass-through flag"));
	printf("# %s\n", _("is not set and the message is small."));
	printf("#\n");
	printf(":0\n");
	printf(				    /* RATS: ignore (OK) */
		      "* ! %s ?? " UCPACKAGE "PASSTHROUGH\n",
		      _("TESTPASS"));
	printf("* ! > 500000\n");
	printf("{\n");

	printf("   # %s\n", _("Step 2:"));
	printf("   # %s\n", _("Discard any looping mail."));
	printf("   #\n");
	printf("   :0\n");
	printf("   * $ ^X-Loop: $%s%s\n", UCPACKAGE, _("MAINEMAIL"));
	printf("   /dev/null\n");
	printf("\n");

	printf("   # %s\n", _("Step 3:"));
	printf("   # %s\n", _("Deal with any bounces."));
	printf("   #\n");
	printf("   :0\n");
	printf("   * ^FROM_MAILER\n");
	printf("   {\n");
	printf("      :0 Bw\n");
	printf("      * $ %s ?? .*$%s%s\n", _("SUBJECT"), UCPACKAGE,
	       _("SUBJECT"));
	printf("      | " PROGRAM_NAME " -v -b\n"	/* RATS: ignore (OK) */
	    );
	printf("\n");
	printf("      :0 Bw\n");
	printf("      * $ ^X-Loop: $%s%s\n", UCPACKAGE, _("MAINEMAIL"));
	printf("      | " PROGRAM_NAME " -v -b\n"	/* RATS: ignore (OK) */
	    );
	printf("   }\n");
	printf("\n");


	printf("   # %s\n", _("Step 4:"));
	printf("   # %s\n",
	       _("Pass any sender's replies straight to the program."));
	printf("   #\n");
	printf("   :0 w\n");
	printf("   * ! ^FROM_DAEMON\n");
	printf("   * $ %s ?? .*$%s%s\n", _("SUBJECT"), UCPACKAGE,
	       _("SUBJECT"));
	printf("   | " PROGRAM_NAME " -v -s \"$%s%s\"\n",	/* RATS: ignore */
	       UCPACKAGE, _("SUBJECT"));
	printf("\n");

	printf("   # %s\n", _("Step 5:"));
	printf("   # %s\n", _("Check whether message is spam."));
	printf("   #\n");
	printf("   :0 wf\n");
	printf("   | qsf -ra\n");
	printf("\n");

	printf("   # %s\n", _("Step 6:"));
	printf("   # %s\n", _("Drop potential spam into the"));
	printf("   # %s\n", _("queue, and send the reply email."));
	printf("   #\n");
	printf("   :0\n");
	printf("   * ! ^FROM_DAEMON\n");
	printf("   * !$ %s ?? .*$%s%s\n", _("SUBJECT"), UCPACKAGE,
	       _("SUBJECT"));
	printf("   * ! ^X-Spam: NO\n");
	printf("   {\n");
	printf("      # %s\n", _("Generate the reply."));
	printf("      :0 wf\n");
	printf("      | " PROGRAM_NAME " -t -v -s \"$%s%s\"\n",	/* RATS: ignore */
	       UCPACKAGE, _("SUBJECT"));
	printf("\n");
	printf("      :0 Ha\n");
	printf("      {\n");
	printf("         # %s\n", _("Discard unaddressed responses."));
	printf("         :0\n");
	printf("         * ! ^To\n");
	printf("         /dev/null\n");
	printf("\n");
	printf("         # %s\n", _("Add our loop catching header."));
	printf("         :0 wf\n");
	printf("         | formail -A \"X-Loop: $%s%s\"\n", UCPACKAGE,
	       _("MAINEMAIL"));
	printf("\n");
	printf("         # %s\n", _("Add our From: address."));
	printf("         :0 wfa\n");
	printf("         | formail -I \"From: $%s%s\"\n", UCPACKAGE,
	       _("MAINEMAIL"));
	printf("\n");
	printf("         # %s\n", _("Send the reply."));
	printf("         :0 wa\n");
	printf("         | $SENDMAIL $SENDMAILFLAGS -t\n");
	printf("      }\n");
	printf("   }\n");

	printf("}\n");
	printf("\n");

	printf("# %s\n", _("Step 7:"));
	printf("# %s\n", _("Teach the spam filter that passed-through"));
	printf("# %s\n", _("messages are not spam."));
	printf("#\n");
	printf(":0 wc\n");
	printf("* %s ?? " UCPACKAGE "PASSTHROUGH\n",	/* RATS: ignore */
	       _("TESTPASS"));
	printf("| qsf -Ma\n");
	printf("\n");

	printf("#\n");
	printf("# %s\n", _("End of example."));
	printf("# %s %s\n",
	       _("Don't forget to add"),
	       "\"0 * * * * " PROGRAM_NAME " -v -q\"");
	printf("# %s\n", _("to your crontab!"));
	printf("#\n");

	return 0;
}

/* EOF */
