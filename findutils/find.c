/* vi: set sw=4 ts=4: */
/*
 * Mini find implementation for busybox
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Reworked by David Douthitt <n9ubh@callsign.net> and
 *  Matt Kraai <kraai@alumni.carnegiemellon.edu>.
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 */

#include "busybox.h"
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <time.h>
#include <ctype.h>

static char *pattern;
#ifdef CONFIG_FEATURE_FIND_PRINT0
static char printsep = '\n';
#endif

#ifdef CONFIG_FEATURE_FIND_TYPE
static int type_mask = 0;
#endif

#ifdef CONFIG_FEATURE_FIND_PERM
static char perm_char = 0;
static int perm_mask = 0;
#endif

#ifdef CONFIG_FEATURE_FIND_MTIME
static char mtime_char;
static int mtime_days;
#endif

#ifdef CONFIG_FEATURE_FIND_MMIN
static char mmin_char;
static int mmin_mins;
#endif

#ifdef CONFIG_FEATURE_FIND_XDEV
static dev_t *xdev_dev;
static int xdev_count = 0;
#endif

#ifdef CONFIG_FEATURE_FIND_NEWER
static time_t newer_mtime;
#endif

#ifdef CONFIG_FEATURE_FIND_INUM
static ino_t inode_num;
#endif

#ifdef CONFIG_FEATURE_FIND_EXEC
static char **exec_str;
static int num_matches;
static int exec_opt;
#endif

static int fileAction(const char *fileName, struct stat *statbuf, void* junk)
{
#ifdef CONFIG_FEATURE_FIND_XDEV
	if (S_ISDIR(statbuf->st_mode) && xdev_count) {
		int i;
		for (i=0; i<xdev_count; i++) {
			if (xdev_dev[i] != statbuf->st_dev)
				return SKIP;
		}
	}
#endif
	if (pattern != NULL) {
		const char *tmp = strrchr(fileName, '/');

		if (tmp == NULL)
			tmp = fileName;
		else
			tmp++;
		if (!(fnmatch(pattern, tmp, FNM_PERIOD) == 0))
			goto no_match;
	}
#ifdef CONFIG_FEATURE_FIND_TYPE
	if (type_mask != 0) {
		if (!((statbuf->st_mode & S_IFMT) == type_mask))
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_PERM
	if (perm_mask != 0) {
		if (!((isdigit(perm_char) && (statbuf->st_mode & 07777) == perm_mask) ||
			 (perm_char == '-' && (statbuf->st_mode & perm_mask) == perm_mask) ||
			 (perm_char == '+' && (statbuf->st_mode & perm_mask) != 0)))
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_MTIME
	if (mtime_char != 0) {
		time_t file_age = time(NULL) - statbuf->st_mtime;
		time_t mtime_secs = mtime_days * 24 * 60 * 60;
		if (!((isdigit(mtime_char) && file_age >= mtime_secs &&
						file_age < mtime_secs + 24 * 60 * 60) ||
				(mtime_char == '+' && file_age >= mtime_secs + 24 * 60 * 60) ||
				(mtime_char == '-' && file_age < mtime_secs)))
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_MMIN
	if (mmin_char != 0) {
		time_t file_age = time(NULL) - statbuf->st_mtime;
		time_t mmin_secs = mmin_mins * 60;
		if (!((isdigit(mmin_char) && file_age >= mmin_secs &&
						file_age < mmin_secs + 60) ||
				(mmin_char == '+' && file_age >= mmin_secs + 60) ||
				(mmin_char == '-' && file_age < mmin_secs)))
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_NEWER
	if (newer_mtime != 0) {
		time_t file_age = newer_mtime - statbuf->st_mtime;
		if (file_age >= 0)
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_INUM
	if (inode_num != 0) {
		if (!(statbuf->st_ino == inode_num))
			goto no_match;
	}
#endif
#ifdef CONFIG_FEATURE_FIND_EXEC
	if (exec_opt) {
		int i;
		char *cmd_string = "";
		for (i = 0; i < num_matches; i++)
			cmd_string = bb_xasprintf("%s%s%s", cmd_string, exec_str[i], fileName);
		cmd_string = bb_xasprintf("%s%s", cmd_string, exec_str[num_matches]);
		system(cmd_string);
		goto no_match;
	}
#endif

#ifdef CONFIG_FEATURE_FIND_PRINT0
	printf("%s%c", fileName, printsep);
#else
	puts(fileName);
#endif
no_match:
	return (TRUE);
}

#ifdef CONFIG_FEATURE_FIND_TYPE
static int find_type(char *type)
{
	int mask = 0;

	switch (type[0]) {
		case 'b':
			mask = S_IFBLK;
			break;
		case 'c':
			mask = S_IFCHR;
			break;
		case 'd':
			mask = S_IFDIR;
			break;
		case 'p':
			mask = S_IFIFO;
			break;
		case 'f':
			mask = S_IFREG;
			break;
		case 'l':
			mask = S_IFLNK;
			break;
		case 's':
			mask = S_IFSOCK;
			break;
	}

	if (mask == 0 || type[1] != '\0')
		bb_error_msg_and_die(bb_msg_invalid_arg, type, "-type");

	return mask;
}
#endif

int find_main(int argc, char **argv)
{
	int dereference = FALSE;
	int i, firstopt, status = EXIT_SUCCESS;

	for (firstopt = 1; firstopt < argc; firstopt++) {
		if (argv[firstopt][0] == '-')
			break;
	}

	/* Parse any options */
	for (i = firstopt; i < argc; i++) {
		if (strcmp(argv[i], "-follow") == 0)
			dereference = TRUE;
		else if (strcmp(argv[i], "-print") == 0) {
			;
			}
#ifdef CONFIG_FEATURE_FIND_PRINT0
		else if (strcmp(argv[i], "-print0") == 0)
			printsep = '\0';
#endif
		else if (strcmp(argv[i], "-name") == 0) {
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-name");
			pattern = argv[i];
#ifdef CONFIG_FEATURE_FIND_TYPE
		} else if (strcmp(argv[i], "-type") == 0) {
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-type");
			type_mask = find_type(argv[i]);
#endif
#ifdef CONFIG_FEATURE_FIND_PERM
		} else if (strcmp(argv[i], "-perm") == 0) {
			char *end;
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-perm");
			perm_mask = strtol(argv[i], &end, 8);
			if ((end[0] != '\0') || (perm_mask > 07777))
				bb_error_msg_and_die(bb_msg_invalid_arg, argv[i], "-perm");
			if ((perm_char = argv[i][0]) == '-')
				perm_mask = -perm_mask;
#endif
#ifdef CONFIG_FEATURE_FIND_MTIME
		} else if (strcmp(argv[i], "-mtime") == 0) {
			char *end;
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-mtime");
			mtime_days = strtol(argv[i], &end, 10);
			if (end[0] != '\0')
				bb_error_msg_and_die(bb_msg_invalid_arg, argv[i], "-mtime");
			if ((mtime_char = argv[i][0]) == '-')
				mtime_days = -mtime_days;
#endif
#ifdef CONFIG_FEATURE_FIND_MMIN
		} else if (strcmp(argv[i], "-mmin") == 0) {
			char *end;
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-mmin");
			mmin_mins = strtol(argv[i], &end, 10);
			if (end[0] != '\0')
				bb_error_msg_and_die(bb_msg_invalid_arg, argv[i], "-mmin");
			if ((mmin_char = argv[i][0]) == '-')
				mmin_mins = -mmin_mins;
#endif
#ifdef CONFIG_FEATURE_FIND_XDEV
		} else if (strcmp(argv[i], "-xdev") == 0) {
			struct stat stbuf;

			xdev_count = ( firstopt - 1 ) ? ( firstopt - 1 ) : 1;
			xdev_dev = xmalloc ( xdev_count * sizeof( dev_t ));

			if ( firstopt == 1 ) {
				xstat ( ".", &stbuf );
				xdev_dev [0] = stbuf. st_dev;
			}
			else {

				for (i = 1; i < firstopt; i++) {
					xstat ( argv [i], &stbuf );
					xdev_dev [i-1] = stbuf. st_dev;
				}
			}
#endif
#ifdef CONFIG_FEATURE_FIND_NEWER
		} else if (strcmp(argv[i], "-newer") == 0) {
			struct stat stat_newer;
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-newer");
			xstat (argv[i], &stat_newer);
			newer_mtime = stat_newer.st_mtime;
#endif
#ifdef CONFIG_FEATURE_FIND_INUM
		} else if (strcmp(argv[i], "-inum") == 0) {
			char *end;
			if (++i == argc)
				bb_error_msg_and_die(bb_msg_requires_arg, "-inum");
			inode_num = strtol(argv[i], &end, 10);
			if (end[0] != '\0')
				bb_error_msg_and_die(bb_msg_invalid_arg, argv[i], "-inum");
#endif
#ifdef CONFIG_FEATURE_FIND_EXEC
		} else if (strcmp(argv[i], "-exec") == 0) {
			int b_pos;
			char *cmd_string = "";

			while (i++) {
				if (i == argc)
					bb_error_msg_and_die(bb_msg_requires_arg, "-exec");
				if (*argv[i] == ';')
					break;
				cmd_string = bb_xasprintf("%s %s", cmd_string, argv[i]);
			}

			if (*cmd_string == 0)
				bb_error_msg_and_die(bb_msg_requires_arg, "-exec");
			cmd_string++;
			exec_str = xmalloc(sizeof(char *));

			while ((b_pos = strstr(cmd_string, "{}") - cmd_string), (b_pos >= 0)) {
				num_matches++;
				exec_str = xrealloc(exec_str, (num_matches + 1) * sizeof(char *));
				exec_str[num_matches - 1] = bb_xstrndup(cmd_string, b_pos);
				cmd_string += b_pos + 2;
			}
			exec_str[num_matches] = bb_xstrdup(cmd_string);
			exec_opt = 1;
#endif
		} else
			bb_show_usage();
	}

	if (firstopt == 1) {
		if (! recursive_action(".", TRUE, dereference, FALSE, fileAction,
					fileAction, NULL))
			status = EXIT_FAILURE;
	} else {
		for (i = 1; i < firstopt; i++) {
			if (! recursive_action(argv[i], TRUE, dereference, FALSE, fileAction,
						fileAction, NULL))
				status = EXIT_FAILURE;
		}
	}

	return status;
}
