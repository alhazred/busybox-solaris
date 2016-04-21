/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 *
 * Based in part on code from sash, Copyright (c) 1999 by David I. Bell
 * Permission has been granted to redistribute this code under the GPL.
 *
 */
#ifndef	__LIBBUSYBOX_H__
#define	__LIBBUSYBOX_H__    1

#include "bb_config.h"
#include "platform.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifdef CONFIG_SELINUX
#include <selinux/selinux.h>
#endif

#include "pwd_.h"
#include "grp_.h"
#ifdef CONFIG_FEATURE_SHADOWPASSWDS
#include "shadow_.h"
#endif
#ifdef CONFIG_FEATURE_SHA1_PASSWORDS
# include "sha1.h"
#endif

/* Try to pull in PATH_MAX */
#include <limits.h>
#include <sys/param.h>
#ifndef PATH_MAX
#define  PATH_MAX         256
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/* Some useful definitions */
#undef FALSE
#define FALSE   ((int) 0)
#undef TRUE
#define TRUE    ((int) 1)
#undef SKIP
#define SKIP	((int) 2)

/* for mtab.c */
#define MTAB_GETMOUNTPT '1'
#define MTAB_GETDEVICE  '2'

#define BUF_SIZE        8192
#define EXPAND_ALLOC    1024

/* Macros for min/max.  */
#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/* buffer allocation schemes */
#ifdef CONFIG_FEATURE_BUFFERS_GO_ON_STACK
#define RESERVE_CONFIG_BUFFER(buffer,len)           char buffer[len]
#define RESERVE_CONFIG_UBUFFER(buffer,len) unsigned char buffer[len]
#define RELEASE_CONFIG_BUFFER(buffer)      ((void)0)
#else
#ifdef CONFIG_FEATURE_BUFFERS_GO_IN_BSS
#define RESERVE_CONFIG_BUFFER(buffer,len)  static          char buffer[len]
#define RESERVE_CONFIG_UBUFFER(buffer,len) static unsigned char buffer[len]
#define RELEASE_CONFIG_BUFFER(buffer)      ((void)0)
#else
#define RESERVE_CONFIG_BUFFER(buffer,len)           char *buffer=xmalloc(len)
#define RESERVE_CONFIG_UBUFFER(buffer,len) unsigned char *buffer=xmalloc(len)
#define RELEASE_CONFIG_BUFFER(buffer)      free (buffer)
#endif
#endif


typedef struct llist_s {
	char *data;
	struct llist_s *link;
} llist_t;
extern void llist_add_to(llist_t **old_head, void *data);
extern void llist_add_to_end(llist_t **list_head, void *data);
extern void *llist_pop(llist_t **elm);
extern void llist_free(llist_t *elm, void (*freeit)(void *data));


extern void bb_show_usage(void) ATTRIBUTE_NORETURN ATTRIBUTE_EXTERNALLY_VISIBLE;
extern void bb_error_msg(const char *s, ...) __attribute__ ((format (printf, 1, 2)));
extern void bb_error_msg_and_die(const char *s, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
extern void bb_perror_msg(const char *s, ...) __attribute__ ((format (printf, 1, 2)));
extern void bb_perror_msg_and_die(const char *s, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
extern void bb_vherror_msg(const char *s, va_list p);
extern void bb_herror_msg(const char *s, ...) __attribute__ ((format (printf, 1, 2)));
extern void bb_herror_msg_and_die(const char *s, ...) __attribute__ ((noreturn, format (printf, 1, 2)));

extern void bb_perror_nomsg_and_die(void) ATTRIBUTE_NORETURN;
extern void bb_perror_nomsg(void);

/* These two are used internally -- you shouldn't need to use them */
extern void bb_verror_msg(const char *s, va_list p) __attribute__ ((format (printf, 1, 0)));
extern void bb_vperror_msg(const char *s, va_list p)  __attribute__ ((format (printf, 1, 0)));

extern int bb_echo(int argc, char** argv);
extern int bb_test(int argc, char** argv);

extern const char *bb_mode_string(int mode);
extern int is_directory(const char *name, int followLinks, struct stat *statBuf);
extern DIR *bb_opendir(const char *path);
extern DIR *bb_xopendir(const char *path);

extern int remove_file(const char *path, int flags);
extern int copy_file(const char *source, const char *dest, int flags);
extern ssize_t safe_read(int fd, void *buf, size_t count);
extern ssize_t bb_full_read(int fd, void *buf, size_t len);
extern ssize_t safe_write(int fd, const void *buf, size_t count);
extern ssize_t bb_full_write(int fd, const void *buf, size_t len);
extern int recursive_action(const char *fileName, int recurse,
	  int followLinks, int depthFirst,
	  int (*fileAction) (const char *fileName, struct stat* statbuf, void* userData),
	  int (*dirAction) (const char *fileName, struct stat* statbuf, void* userData),
	  void* userData);

extern int bb_parse_mode( const char* s, mode_t* theMode);
extern long bb_xgetlarg(const char *arg, int base, long lower, long upper);

extern unsigned int tty_baud_to_value(speed_t speed);
extern speed_t tty_value_to_baud(unsigned int value);

extern int get_linux_version_code(void);

extern int get_console_fd(void);
extern struct mntent *find_mount_point(const char *name, const char *table);
extern void erase_mtab(const char * name);
extern long *find_pid_by_name( const char* pidName);
extern long *pidlist_reverse(long *pidList);
extern char *find_block_device(char *path);
extern char *bb_get_line_from_file(FILE *file);
extern char *bb_get_chomped_line_from_file(FILE *file);
extern char *bb_get_chunk_from_file(FILE *file, int *end);
extern int bb_copyfd_size(int fd1, int fd2, const off_t size);
extern int bb_copyfd_eof(int fd1, int fd2);
extern void  bb_xprint_and_close_file(FILE *file);
extern int   bb_xprint_file_by_name(const char *filename);
extern char  bb_process_escape_sequence(const char **ptr);
extern char *bb_get_last_path_component(char *path);
extern FILE *bb_wfopen(const char *path, const char *mode);
extern FILE *bb_wfopen_input(const char *filename);
extern FILE *bb_xfopen(const char *path, const char *mode);

extern int   bb_fclose_nonstdin(FILE *f);
extern void  bb_fflush_stdout_and_exit(int retval) ATTRIBUTE_NORETURN;

extern void xstat(const char *filename, struct stat *buf);
extern int  bb_xsocket(int domain, int type, int protocol);
extern pid_t bb_spawn(char **argv);
extern pid_t bb_xspawn(char **argv);
extern int wait4pid(int pid);
extern void bb_xdaemon(int nochdir, int noclose);
extern void bb_xbind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
extern void bb_xlisten(int s, int backlog);
extern void bb_xchdir(const char *path);

#define BB_GETOPT_ERROR 0x80000000UL
extern const char *bb_opt_complementally;
extern const struct option *bb_applet_long_options;
extern unsigned long bb_getopt_ulflags(int argc, char **argv, const char *applet_opts, ...);

extern int bb_vfprintf(FILE * __restrict stream, const char * __restrict format,
					   va_list arg) __attribute__ ((format (printf, 2, 0)));
extern int bb_vprintf(const char * __restrict format, va_list arg)
	__attribute__ ((format (printf, 1, 0)));
extern int bb_fprintf(FILE * __restrict stream, const char * __restrict format, ...)
	__attribute__ ((format (printf, 2, 3)));
extern int bb_printf(const char * __restrict format, ...)
	__attribute__ ((format (printf, 1, 2)));

//#warning rename to xferror_filename?
extern void bb_xferror(FILE *fp, const char *fn);
extern void bb_xferror_stdout(void);
extern void bb_xfflush_stdout(void);

extern void bb_warn_ignoring_args(int n);

extern void chomp(char *s);
extern void trim(char *s);
extern char *skip_whitespace(const char *);

extern struct BB_applet *find_applet_by_name(const char *name);
void run_applet_by_name(const char *name, int argc, char **argv);

/* dmalloc will redefine these to it's own implementation. It is safe
 * to have the prototypes here unconditionally.  */
extern void *xmalloc(size_t size);
extern void *xrealloc(void *old, size_t size);
extern void *xzalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);

extern char *bb_xstrdup (const char *s);
extern char *bb_xstrndup (const char *s, int n);
extern char *safe_strncpy(char *dst, const char *src, size_t size);
extern int safe_strtoi(char *arg, int* value);
extern int safe_strtod(char *arg, double* value);
extern int safe_strtol(char *arg, long* value);
extern int safe_strtoul(char *arg, unsigned long* value);

struct suffix_mult {
	const char *suffix;
	unsigned int mult;
};

extern unsigned long bb_xgetularg_bnd_sfx(const char *arg, int base,
										  unsigned long lower,
										  unsigned long upper,
										  const struct suffix_mult *suffixes);
extern unsigned long bb_xgetularg_bnd(const char *arg, int base,
									  unsigned long lower,
									  unsigned long upper);
extern unsigned long bb_xgetularg10_bnd(const char *arg,
										unsigned long lower,
										unsigned long upper);
extern unsigned long bb_xgetularg10(const char *arg);

extern long bb_xgetlarg_bnd_sfx(const char *arg, int base,
								long lower,
								long upper,
								const struct suffix_mult *suffixes);
extern long bb_xgetlarg10_sfx(const char *arg, const struct suffix_mult *suffixes);


//#warning pitchable now?
extern unsigned long bb_xparse_number(const char *numstr,
		const struct suffix_mult *suffixes);


/* These parse entries in /etc/passwd and /etc/group.  This is desirable
 * for BusyBox since we want to avoid using the glibc NSS stuff, which
 * increases target size and is often not needed on embedded systems.  */
extern long bb_xgetpwnam(const char *name);
extern long bb_xgetgrnam(const char *name);
extern char * bb_getug(char *buffer, char *idname, long id, int bufsize, char prefix);
extern char * bb_getpwuid(char *name, long uid, int bufsize);
extern char * bb_getgrgid(char *group, long gid, int bufsize);
extern char *bb_askpass(int timeout, const char * prompt);

extern int device_open(const char *device, int mode);

extern char *query_loop(const char *device);
extern int del_loop(const char *device);
extern int set_loop(char **device, const char *file, int offset);

#if (__GLIBC__ < 2)
extern int vdprintf(int d, const char *format, va_list ap);
#endif

int nfsmount(const char *spec, const char *node, int *flags,
	     char **mount_opts, int running_bg);

/* Include our own copy of struct sysinfo to avoid binary compatibility
 * problems with Linux 2.4, which changed things.  Grumble, grumble. */
struct sysinfo {
	long uptime;			/* Seconds since boot */
	unsigned long loads[3];		/* 1, 5, and 15 minute load averages */
	unsigned long totalram;		/* Total usable main memory size */
	unsigned long freeram;		/* Available memory size */
	unsigned long sharedram;	/* Amount of shared memory */
	unsigned long bufferram;	/* Memory used by buffers */
	unsigned long totalswap;	/* Total swap space size */
	unsigned long freeswap;		/* swap space still available */
	unsigned short procs;		/* Number of current processes */
	unsigned short pad;			/* Padding needed for m68k */
	unsigned long totalhigh;	/* Total high memory size */
	unsigned long freehigh;		/* Available high memory size */
	unsigned int mem_unit;		/* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];	/* Padding: libc5 uses this.. */
};
extern int sysinfo (struct sysinfo* info);

enum {
	KILOBYTE = 1024,
	MEGABYTE = (KILOBYTE*1024),
	GIGABYTE = (MEGABYTE*1024)
};
const char *make_human_readable_str(unsigned long long size,
		unsigned long block_size, unsigned long display_unit);

int bb_ask_confirmation(void);
int klogctl(int type, char * b, int len);

char *xgetcwd(char *cwd);
char *xreadlink(const char *path);
char *concat_path_file(const char *path, const char *filename);
char *concat_subpath_file(const char *path, const char *filename);
char *last_char_is(const char *s, int c);

int read_package_field(const char *package_buffer, char **field_name, char **field_value);
//#warning yuk!
char *fgets_str(FILE *file, const char *terminating_string);

extern int uncompress(int fd_in, int fd_out);
extern int inflate(int in, int out);

extern struct hostent *xgethostbyname(const char *name);
extern struct hostent *xgethostbyname2(const char *name, int af);
extern int create_icmp_socket(void);
extern int create_icmp6_socket(void);
extern int xconnect(struct sockaddr_in s_in, const char *host, const char *port);
extern unsigned short bb_lookup_port(const char *port, const char *protocol, unsigned short default_port);
extern void bb_lookup_host(struct sockaddr_in s_in, const char *host);

//#warning wrap this?
char *dirname (char *path);

int bb_make_directory (char *path, long mode, int flags);

const char *u_signal_names(const char *str_sig, int *signo, int startnum);
char *bb_simplify_path(const char *path);

enum {	/* DO NOT CHANGE THESE VALUES!  cp.c depends on them. */
	FILEUTILS_PRESERVE_STATUS = 1,
	FILEUTILS_DEREFERENCE = 2,
	FILEUTILS_RECUR = 4,
	FILEUTILS_FORCE = 8,
	FILEUTILS_INTERACTIVE = 16
};

extern const char *bb_applet_name;

extern const char * const bb_msg_full_version;
extern const char * const bb_msg_memory_exhausted;
extern const char * const bb_msg_invalid_date;
extern const char * const bb_msg_io_error;
extern const char * const bb_msg_read_error;
extern const char * const bb_msg_write_error;
extern const char * const bb_msg_name_longer_than_foo;
extern const char * const bb_msg_unknown;
extern const char * const bb_msg_can_not_create_raw_socket;
extern const char * const bb_msg_perm_denied_are_you_root;
extern const char * const bb_msg_requires_arg;
extern const char * const bb_msg_invalid_arg;
extern const char * const bb_msg_standard_input;
extern const char * const bb_msg_standard_output;

extern const char * const bb_path_nologin_file;
extern const char * const bb_path_passwd_file;
extern const char * const bb_path_shadow_file;
extern const char * const bb_path_gshadow_file;
extern const char * const bb_path_group_file;
extern const char * const bb_path_securetty_file;
extern const char * const bb_path_motd_file;
extern const char * const bb_path_wtmp_file;
extern const char * const bb_dev_null;

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif
extern char bb_common_bufsiz1[BUFSIZ+1];

/*
 * You can change LIBBB_DEFAULT_LOGIN_SHELL, but don`t use,
 * use bb_default_login_shell and next defines,
 * if you LIBBB_DEFAULT_LOGIN_SHELL change,
 * don`t lose change increment constant!
 */
#define LIBBB_DEFAULT_LOGIN_SHELL      "-/bin/sh"

extern const char * const bb_default_login_shell;
/* "/bin/sh" */
#define DEFAULT_SHELL     (bb_default_login_shell+1)
/* "sh" */
#define DEFAULT_SHELL_SHORT_NAME     (bb_default_login_shell+6)


extern const char bb_path_mtab_file[];

extern int bb_default_error_retval;

#ifdef CONFIG_FEATURE_DEVFS
# define CURRENT_VC "/dev/vc/0"
# define VC_1 "/dev/vc/1"
# define VC_2 "/dev/vc/2"
# define VC_3 "/dev/vc/3"
# define VC_4 "/dev/vc/4"
# define VC_5 "/dev/vc/5"
#if defined(__sh__) || defined(__H8300H__) || defined(__H8300S__)
/* Yes, this sucks, but both SH (including sh64) and H8 have a SCI(F) for their
   respective serial ports .. as such, we can't use the common device paths for
   these. -- PFM */
#  define SC_0 "/dev/ttsc/0"
#  define SC_1 "/dev/ttsc/1"
#  define SC_FORMAT "/dev/ttsc/%d"
#else
#  define SC_0 "/dev/tts/0"
#  define SC_1 "/dev/tts/1"
#  define SC_FORMAT "/dev/tts/%d"
#endif
# define VC_FORMAT "/dev/vc/%d"
# define LOOP_FORMAT "/dev/loop/%d"
# define FB_0 "/dev/fb/0"
#else
# define CURRENT_VC "/dev/tty0"
# define VC_1 "/dev/tty1"
# define VC_2 "/dev/tty2"
# define VC_3 "/dev/tty3"
# define VC_4 "/dev/tty4"
# define VC_5 "/dev/tty5"
#if defined(__sh__) || defined(__H8300H__) || defined(__H8300S__)
#  define SC_0 "/dev/ttySC0"
#  define SC_1 "/dev/ttySC1"
#  define SC_FORMAT "/dev/ttySC%d"
#else
#  define SC_0 "/dev/ttyS0"
#  define SC_1 "/dev/ttyS1"
#  define SC_FORMAT "/dev/ttyS%d"
#endif
# define VC_FORMAT "/dev/tty%d"
# define LOOP_FORMAT "/dev/loop%d"
# define FB_0 "/dev/fb0"
#endif

//#warning put these in .o files

/* The following devices are the same on devfs and non-devfs systems.  */
#define CURRENT_TTY "/dev/tty"
#define CONSOLE_DEV "/dev/console"

int is_in_ino_dev_hashtable(const struct stat *statbuf, char **name);
void add_to_ino_dev_hashtable(const struct stat *statbuf, const char *name);
void reset_ino_dev_hashtable(void);

char *bb_xasprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#define FAIL_DELAY    3
extern void bb_do_delay(int seconds);
extern void change_identity ( const struct passwd *pw );
extern const char *change_identity_e2str ( const struct passwd *pw );
extern void run_shell ( const char *shell, int loginshell, const char *command, const char **additional_args);
#ifdef CONFIG_SELINUX
extern void renew_current_security_context(void);
extern void set_current_security_context(security_context_t sid);
#endif
extern int run_parts(char **args, const unsigned char test_mode, char **env);
extern int restricted_shell ( const char *shell );
extern void setup_environment ( const char *shell, int loginshell, int changeenv, const struct passwd *pw );
extern int correct_password ( const struct passwd *pw );
extern char *pw_encrypt(const char *clear, const char *salt);
extern struct spwd *pwd_to_spwd(const struct passwd *pw);
extern int obscure(const char *old, const char *newval, const struct passwd *pwdp);

extern int bb_xopen(const char *pathname, int flags);
extern int bb_xopen3(const char *pathname, int flags, int mode);
extern ssize_t bb_xread(int fd, void *buf, size_t count);
extern void bb_xread_all(int fd, void *buf, size_t count);
extern unsigned char bb_xread_char(int fd);

#ifndef COMM_LEN
#ifdef TASK_COMM_LEN
#define COMM_LEN TASK_COMM_LEN
#else
/* synchronize with sizeof(task_struct.comm) in /usr/include/linux/sched.h */
#define COMM_LEN 16
#endif
#endif
typedef struct {
	int pid;
	char user[9];
	char state[4];
	unsigned long rss;
	int ppid;
#ifdef CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE
	unsigned pcpu;
	unsigned pscpu;
	unsigned long stime, utime;
#endif
	char *cmd;

	/* basename of executable file in call to exec(2),
		size from kernel headers */
	char short_cmd[COMM_LEN];
} procps_status_t;

extern procps_status_t * procps_scan(int save_user_arg0);
extern int compare_string_array(const char * const string_array[], const char *key);

extern int my_query_module(const char *name, int which, void **buf, size_t *bufsize, size_t *ret);

extern void print_login_issue(const char *issue_file, const char *tty);
extern void print_login_prompt(void);

#ifdef BB_NOMMU
extern void vfork_daemon(int nochdir, int noclose);
extern void vfork_daemon_rexec(int nochdir, int noclose,
		int argc, char **argv, char *foreground_opt);
#endif

extern int get_terminal_width_height(int fd, int *width, int *height);
extern unsigned long get_ug_id(const char *s, long (*__bb_getxxnam)(const char *));

typedef struct _sha1_ctx_t_ {
	uint32_t count[2];
	uint32_t hash[5];
	uint32_t wbuf[16];
} sha1_ctx_t;

void sha1_begin(sha1_ctx_t *ctx);
void sha1_hash(const void *data, size_t length, sha1_ctx_t *ctx);
void *sha1_end(void *resbuf, sha1_ctx_t *ctx);

typedef struct _md5_ctx_t_ {
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
	uint64_t total;
	uint32_t buflen;
	char buffer[128];
} md5_ctx_t;

void md5_begin(md5_ctx_t *ctx);
void md5_hash(const void *data, size_t length, md5_ctx_t *ctx);
void *md5_end(void *resbuf, md5_ctx_t *ctx);

extern uint32_t *bb_crc32_filltable (int endian);

#ifndef RB_POWER_OFF
/* Stop system and switch power off if possible.  */
#define RB_POWER_OFF   0x4321fedc
#endif

extern const char BB_BANNER[];

#endif /* __LIBBUSYBOX_H__ */
