
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>
#include <time.h>
#include <lock.h>

#define MAX_FILES			512
#define MAX_MOUNTPOINTS			32

// error codes
#define EACCES				-1
#define EIO				-2
#define ELOOP				-3
#define ENAMETOOLONG			-4
#define ENOENT				-5
#define ENOTDIR				-6
#define EOVERFLOW			-7
#define ENXIO				-8
#define ENOBUFS				-9
#define EBADF				-10
#define EINVAL				-11
#define EPERM				-12
#define ENODEV				-13
#define ENOTBLK				-14
#define EBUSY				-15

// open() flags
#define O_RDONLY			0x0001
#define O_WRONLY			0x0002
#define O_RDWR				(O_RDONLY | O_WRONLY)
#define O_ACCMODE			(~O_RDWR)
#define O_APPEND			0x0004
#define O_NONBLOCK			0x0000		// we don't use this
#define O_NDELAY			O_NONBLOCK
#define O_FSYNC				0x0000		// not using this either
#define O_SYNC				O_FSYNC
#define O_NOATIME			0x0008
#define O_CREAT				0x0010
#define O_EXCL				0x0020
#define O_TMPFILE			0x0040		// we'll fail when we use this
#define O_NOCTTY			0x0000		// not using this
#define O_EXLOCK			0x0080
#define O_SHLOCK			0x0100

// Values of mode_t
#define S_IFBLK				0x0001		// block special
#define S_IFCHR				0x0002		// char special
#define S_IFIFO				0x0004		// fifo special
#define S_IFREG				0x0008		// regular file
#define S_IFDIR				0x0010		// directory
#define S_IFLNK				0x0020		// symbolic link
#define S_IFSOCK			0x0040		// Unix socket
#define S_IFMT				0x007F		// for BSD compatibility

#define S_IRUSR				0x0080		// read, owner
#define S_IWUSR				0x0100		// write, owner
#define S_IXUSR				0x0200		// execute, owner
#define S_IRWXU				(S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRGRP				0x0400		// read, group
#define S_IWGRP				0x0800		// write, group
#define S_IXGRP				0x1000		// execute, group
#define S_IRWXG				(S_IRGRP | S_IWGRP | S_IXGRP)

#define S_IROTH				0x2000		// read, others
#define S_IWOTH				0x4000		// write, others
#define S_IXOTH				0x8000		// execute, others
#define S_IRWXO				(S_IROTH | S_IWOTH | S_IXOTH)

// Values of `whence' for lseek()
#define SEEK_SET			1
#define SEEK_CUR			2
#define SEEK_END			3

// Standard file descriptor numbers
#define STDIN				0
#define STDOUT				1
#define STDERR				2

// Mountpoint Flags
#define MS_MGC_MASK			MS_MGC_VAL
#define MS_MGC_VAL			0x0001
#define MS_REMOUNT			0x0002
#define MS_RDONLY			0x0004
#define MS_NOSUID			0x0008
#define MS_NOEXEC			0x0010
#define MS_NODEV			0x0020
#define MS_SYNCHRONOUS			0x0040
#define MS_MANDLOCK			0x0080
#define MS_NOATIME			0x0100
#define MS_NODIRATIME			0x0200

typedef uint64_t ino_t;
typedef uint32_t mode_t;
typedef uint32_t gid_t;
typedef uint32_t uid_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef size_t off_t;
typedef uint16_t blksize_t;
typedef size_t blkcnt_t;

typedef struct file_handle_t
{
	char present;
	char path[1024];
	off_t position;
	int flags;
	pid_t pid;
} file_handle_t;

file_handle_t *files;

typedef struct directory_t
{
	char path[1024];
	size_t index;
	size_t size;
} directory_t;

typedef struct directory_entry_t
{
	char filename[1024];
} directory_entry_t;

typedef struct mountpoint_t
{
	char present;
	char fstype[16];
	char path[1024];
	char device[64];		// '/dev/hdxpx'
	unsigned long int flags;
	uid_t uid;
	gid_t gid;
} mountpoint_t;

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	//dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

lock_t vfs_mutex;
file_handle_t *files;
mountpoint_t *mountpoints;
char full_path[1024];

void vfs_init();
size_t vfs_resolve_path(char *, const char *);
int vfs_determine_mountpoint(char *);

// Public functions
int open(const char *, int, ...);
int close(int);
ssize_t read(int, char *, size_t);
ssize_t write(int, char *, size_t);
int link(char *, char *);
int unlink(char *);
int lseek(int, off_t, int);
int chmod(const char *, mode_t);
int fchmod(int, mode_t);
int stat(const char *, struct stat *);
int fstat(int, struct stat *);
int mkdir(const char *, mode_t);
int mount(const char *, const char *, const char *, unsigned long int, void *);
int umount(const char *);
int umount2(const char *, int);

// Non-standard functions
directory_t *dir_open(char *);
void dir_close(directory_t *);
int dir_query(directory_t *, directory_entry_t *);



