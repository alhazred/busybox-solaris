/* Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 *
 *  FIXME:
 *    In privileged mode if uname and gname map to a uid and gid then use the
 *    mapped value instead of the uid/gid values in tar header
 *
 *  References:
 *    GNU tar and star man pages,
 *    Opengroup's ustar interchange format,
 *	http://www.opengroup.org/onlinepubs/007904975/utilities/pax.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysmacros.h>	/* For makedev */
#include "unarchive.h"
#include "libbb.h"

#ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
static char *longname = NULL;
static char *linkname = NULL;
#endif

char get_header_tar(archive_handle_t *archive_handle)
{
	file_header_t *file_header = archive_handle->file_header;
	union {
		/* ustar header, Posix 1003.1 */
		unsigned char raw[512];
		struct {
			char name[100];	/*   0-99 */
			char mode[8];	/* 100-107 */
			char uid[8];	/* 108-115 */
			char gid[8];	/* 116-123 */
			char size[12];	/* 124-135 */
			char mtime[12];	/* 136-147 */
			char chksum[8];	/* 148-155 */
			char typeflag;	/* 156-156 */
			char linkname[100];	/* 157-256 */
			char magic[6];	/* 257-262 */
			char version[2];	/* 263-264 */
			char uname[32];	/* 265-296 */
			char gname[32];	/* 297-328 */
			char devmajor[8];	/* 329-336 */
			char devminor[8];	/* 337-344 */
			char prefix[155];	/* 345-499 */
			char padding[12];	/* 500-512 */
		} formated;
	} tar;
	long sum = 0;
	long i;
	static int end = 0;

	/* Align header */
	data_align(archive_handle, 512);

	if (bb_full_read(archive_handle->src_fd, tar.raw, 512) != 512) {
		/* Assume end of file */
		bb_error_msg_and_die("Short header");
		//return(EXIT_FAILURE);
	}
	archive_handle->offset += 512;

	/* If there is no filename its an empty header */
	if (tar.formated.name[0] == 0) {
		if (end) {
			/* This is the second consecutive empty header! End of archive!
			 * Read until the end to empty the pipe from gz or bz2
			 */
			while (bb_full_read(archive_handle->src_fd, tar.raw, 512) == 512);
			return(EXIT_FAILURE);
		}
		end = 1;
		return(EXIT_SUCCESS);
	}
	end = 0;

	/* Check header has valid magic, "ustar" is for the proper tar
	 * 0's are for the old tar format
	 */
	if (strncmp(tar.formated.magic, "ustar", 5) != 0) {
#ifdef CONFIG_FEATURE_TAR_OLDGNU_COMPATIBILITY
		if (strncmp(tar.formated.magic, "\0\0\0\0\0", 5) != 0)
#endif
			bb_error_msg_and_die("Invalid tar magic");
	}
	/* Do checksum on headers */
	for (i =  0; i < 148 ; i++) {
		sum += tar.raw[i];
	}
	sum += ' ' * 8;
	for (i =  156; i < 512 ; i++) {
		sum += tar.raw[i];
	}
	if (sum != strtol(tar.formated.chksum, NULL, 8)) {
		bb_error_msg("Invalid tar header checksum");
		return(EXIT_FAILURE);
	}

#ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
	if (longname) {
		file_header->name = longname;
		longname = NULL;
	}
	else if (linkname) {
		file_header->name = linkname;
		linkname = NULL;
	} else
#endif
	{
		file_header->name = bb_xstrndup(tar.formated.name,100);

		if (tar.formated.prefix[0]) {
			char *temp = file_header->name;
			file_header->name = concat_path_file(tar.formated.prefix, temp);
			free(temp);
		}
	}

	file_header->uid = strtol(tar.formated.uid, NULL, 8);
	file_header->gid = strtol(tar.formated.gid, NULL, 8);
	file_header->size = strtol(tar.formated.size, NULL, 8);
	file_header->mtime = strtol(tar.formated.mtime, NULL, 8);
	file_header->link_name = (tar.formated.linkname[0] != '\0') ?
	    bb_xstrdup(tar.formated.linkname) : NULL;
	file_header->device = makedev(strtol(tar.formated.devmajor, NULL, 8),
		strtol(tar.formated.devminor, NULL, 8));

	/* Set bits 0-11 of the files mode */
	file_header->mode = 07777 & strtol(tar.formated.mode, NULL, 8);

	/* Set bits 12-15 of the files mode */
	switch (tar.formated.typeflag) {
	/* busybox identifies hard links as being regular files with 0 size and a link name */
	case '1':
		file_header->mode |= S_IFREG;
		break;
	case 'x':
	case 'g':
		bb_error_msg_and_die("pax is not tar");
		break;
	case '7':
		/* Reserved for high performance files, treat as normal file */
	case 0:
	case '0':
#ifdef CONFIG_FEATURE_TAR_OLDGNU_COMPATIBILITY
		if (last_char_is(file_header->name, '/')) {
			file_header->mode |= S_IFDIR;
		} else
#endif
			file_header->mode |= S_IFREG;
		break;
	case '2':
		file_header->mode |= S_IFLNK;
		break;
	case '3':
		file_header->mode |= S_IFCHR;
		break;
	case '4':
		file_header->mode |= S_IFBLK;
		break;
	case '5':
		file_header->mode |= S_IFDIR;
		break;
	case '6':
		file_header->mode |= S_IFIFO;
		break;
#ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
	case 'L': {
			longname = xzalloc(file_header->size + 1);
			archive_xread_all(archive_handle, longname, file_header->size);
			archive_handle->offset += file_header->size;

			return(get_header_tar(archive_handle));
		}
	case 'K': {
			linkname = xzalloc(file_header->size + 1);
			archive_xread_all(archive_handle, linkname, file_header->size);
			archive_handle->offset += file_header->size;

			file_header->name = linkname;
			return(get_header_tar(archive_handle));
		}
	case 'D':	/* GNU dump dir */
	case 'M':	/* Continuation of multi volume archive*/
	case 'N':	/* Old GNU for names > 100 characters */
	case 'S':	/* Sparse file */
	case 'V':	/* Volume header */
		bb_error_msg("Ignoring GNU extension type %c", tar.formated.typeflag);
#endif
	default:
		bb_error_msg("Unknown typeflag: 0x%x", tar.formated.typeflag);
	}
	{	/* Strip trailing '/' in directories */
		/* Must be done after mode is set as '/' is used to check if its a directory */
		char *tmp = last_char_is(file_header->name, '/');
		if (tmp) {
			*tmp = '\0';
		}
	}

	if (archive_handle->filter(archive_handle) == EXIT_SUCCESS) {
		archive_handle->action_header(archive_handle->file_header);
		archive_handle->flags |= ARCHIVE_EXTRACT_QUIET;
		archive_handle->action_data(archive_handle);
		llist_add_to(&(archive_handle->passed), file_header->name);
	} else {
		data_skip(archive_handle);
	}
	archive_handle->offset += file_header->size;

	free(file_header->link_name);

	return(EXIT_SUCCESS);
}
