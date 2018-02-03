/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <reent.h>

#include<libtransistor/loader_config.h>
#include<libtransistor/svc.h>
#include<libtransistor/tls.h>
#include<libtransistor/fd.h>
#include<libtransistor/ipc/time.h>
#include<libtransistor/fs/fsnet.h>

void _exit(); // implemented in libtransistor crt0

struct _reent *__getreent() {
	struct tls *tls = get_tls();
	if (tls == NULL || tls->ctx == NULL)
		return NULL;
	else
		return &tls->ctx->reent;
}

int _close_r(struct _reent *reent, int file) {
	return fsnet_close(file);
}

char *_environ[] = {NULL};

int _execve_r(struct _reent *reent, const char *name, char *const *argv, char *const *env) {
	reent->_errno = ENOSYS;
	return -1;
}

int _fork_r(struct _reent *reent) {
	reent->_errno = ENOSYS;
	return -1;
}

int _fstat_r(struct _reent *reent, int file, struct stat *st) {
	reent->_errno = ENOSYS;
	return -1;
}

int _getpid_r(struct _reent *reent) {
	reent->_errno = ENOSYS;
	return -1;
}

int _isatty_r(struct _reent *reent, int file) {
	reent->_errno = ENOSYS;
	return -1;
}

int _kill_r(struct _reent *reent, int pid, int sig) {
	reent->_errno = ENOSYS;
	return -1;
}

int _link_r(struct _reent *reent, const char *old, const char *new) {
	reent->_errno = ENOSYS;
	return -1;
}

off_t _lseek_r(struct _reent *reent, int file, off_t pos, int whence) {
	return fsnet_lseek(file, pos, whence);
}

int _open_r(struct _reent *reent, const char *name, int flags, int mode) {
	reent->_errno = ENOENT; // XXX pre-emptive
	return fsnet_open(name, flags, mode);
}

ssize_t _read_r(struct _reent *reent, int file, void *ptr, size_t len) {
	reent->_errno = EBADF; // XXX pre-emptive
	return fsnet_read(file, ptr, len);
}

static size_t data_size = 0;
static size_t actual_data_size = 0; // used if not overriding heap
static void *heap_addr = NULL;
#define HEAP_SIZE_INCREMENT 0x2000000

void *_sbrk_r(struct _reent *reent, ptrdiff_t incr) {
	void *addr;
	if(loader_config.heap_overridden) {
		if(data_size + incr > loader_config.heap_size) {
			reent->_errno = ENOMEM;
			return (void*) -1;
		}
		
		addr = loader_config.heap_base + data_size;
		data_size+= incr;
		return addr;
	} else {
		if(data_size + incr > actual_data_size || heap_addr == NULL) {
			ptrdiff_t corrected_incr = data_size + incr - actual_data_size;
			ptrdiff_t rounded_incr = (corrected_incr + HEAP_SIZE_INCREMENT - 1) & ~(HEAP_SIZE_INCREMENT-1);
			result_t r = svcSetHeapSize(&heap_addr, actual_data_size + rounded_incr);
			if(r != RESULT_OK) {
				reent->_errno = ENOMEM;
				return (void*) -1;
			}
			actual_data_size+= rounded_incr;
		}
		
		addr = heap_addr + data_size;
		data_size+= incr;
		return addr;
	}
}

int _stat_r(struct _reent *reent, const char *file, struct stat *st) {
	reent->_errno = ENOSYS;
	return -1;
}

clock_t _times_r(struct _reent *reent, struct tms *buf) {
	reent->_errno = ENOSYS;
	return (clock_t) -1;
}

int _unlink_r(struct _reent *reent, const char *name) {
	reent->_errno = ENOSYS;
	return -1;
}

int _wait_r(struct _reent *reent, int *status) {
	reent->_errno = ENOSYS;
	return -1;
}

ssize_t _write_r(struct _reent *reent, int file, const void *ptr, size_t len) {
	if (file > 2 && fsnet_sock >= 0) {
		reent->_errno = EBADF; // XXX pre-emptive
		return fsnet_write(file, ptr, len);
	}
	ssize_t res = 0;

	struct file *f = fd_file_get(file);
	if (f == NULL) {
		reent->_errno = EBADF;
		return -1;
	}

	if (f->ops->write == NULL) {
		res = -ENOSYS;
		goto finalize;
	}
	res = f->ops->write(f->data, (char*)ptr, len);
finalize:
	fd_file_put(f);
	if (res < 0) {
		reent->_errno = -res;
		return -1;
	}
	return res;
}

int _gettimeofday_r(struct _reent *reent, struct timeval *__restrict p, void *__restrict z) {
	uint64_t time;
	result_t res;
	
	if (z != NULL) {
		reent->_errno = ENOSYS;
		return -1;
	}
	if (p == NULL) {
		reent->_errno = EINVAL;
		return -1;
	}
	
	static bool time_initialized = false;
	if(!time_initialized) {
		time_init();
		time_initialized = true;
		atexit(time_finalize);
	}
	
	if ((res = time_system_clock_get_current_time(time_system_clock_user, &time)) != RESULT_OK) {
		reent->_errno = -EINVAL;
		return -1;
	}
	p->tv_sec = time;
	// No usec support on here :(.
	p->tv_usec = 0;
	return 0;
}

long sysconf(int name) {
	switch(name) {
	case _SC_PAGESIZE:
		return 0x1000;
	}
	errno = ENOSYS;
	return 01;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
	svcSleepThread(rqtp->tv_nsec + (rqtp->tv_sec * 1000000000));
	return 0;
}

int posix_memalign (void **memptr, size_t alignment, size_t size) {
	void *mem;
	
	if (alignment % sizeof(void *) != 0 || (alignment & (alignment - 1)) != 0) {
		return EINVAL;
	}

	mem = memalign(alignment, size);
	
	if (mem != NULL) {
		*memptr = mem;
		return 0;
	}
	
	return ENOMEM;
}

int _rename_r(struct _reent *reent, const char *old, const char *new) {
	// TODO: implement this
	reent->_errno = EROFS;
	return -1;
}

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize) {
	errno = ENOSYS;
	return -1;
}

char *realpath(const char *path, char *resolved_path) {
	errno = ENOSYS;
	return -1;
}

DIR *opendir(const char *name) {
	return fsnet_opendir(name);
}

struct dirent *readdir(DIR *dirp) {
	return fsnet_readdir(dirp);;
}

int closedir(DIR *dirp) {
	free(dirp); // TODO: should be closed on remote machine properly
	return 0;
}

int mkdir(const char *path, mode_t mode) {
	errno = EROFS;
	return -1;
}
