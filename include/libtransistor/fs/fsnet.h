#pragma once

#include<dirent.h>

#define PKT_HEARTBEAT 0x00
#define PKT_OPEN      0x01
#define PKT_CLOSE     0x02
#define PKT_READ      0x03
#define PKT_WRITE     0x04
#define PKT_LSEEK     0x05
#define PKT_OPENDIR   0x06
#define PKT_READDIR   0x07

static int fsnet_sock = -1;

int fsnet_init(uint32_t remote_ip, uint16_t remote_port);
int fsnet_finalize(void);
int fsnet_heartbeat(void);
int fsnet_open(const char * pathname, int flags, int mode);
int fsnet_close(int fd);
int64_t fsnet_read(int fd, void * buf, size_t len);
int64_t fsnet_write(int fd, void * source, size_t len);
int64_t fsnet_lseek(int fd, int offset, int whence);
DIR *fsnet_opendir(const char *name);
struct dirent *fsnet_readdir(DIR *dirp);
