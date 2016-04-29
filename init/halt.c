/* vi: set sw=4 ts=4: */
/*
 * Poweroff reboot and halt, oh my.
 *
 * Copyright 2006 by Rob Landley <rob@landley.net>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "busybox.h"
#include <signal.h>
#include <sys/reboot.h>
#include <unistd.h>

int halt_main(int argc, char *argv[])
{
	static const int magic[] = {
#ifdef RB_HALT_SYSTEM
RB_HALT_SYSTEM,
#elif defined RB_HALT
RB_HALT,
#endif
#ifdef RB_POWER_OFF
RB_POWER_OFF,
#elif defined RB_POWERDOWN
RB_POWERDOWN,
#endif
RB_AUTOBOOT
	};
	static const int signals[] = {SIGUSR1, SIGUSR2, SIGTERM};

	char *delay = "hpr";
	int which, flags, rc = 1;

	/* Figure out which applet we're running */
	for(which=0;delay[which]!=*bb_applet_name;which++);

	/* Parse and handle arguments */
	flags = bb_getopt_ulflags(argc, argv, "d:nf", &delay);
	if (flags&1) sleep(atoi(delay));
	if (!(flags&2)) sync();

	(void) signal(SIGHUP, SIG_IGN);	
	(void) signal(SIGTSTP, SIG_IGN);
	(void) signal(SIGTTIN, SIG_IGN);
	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGPIPE, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	(void) kill(1, SIGHUP);
	sleep(2);		
	rc = reboot(magic[which],NULL);

	if (rc) bb_error_msg("No.");
	return rc;
}
