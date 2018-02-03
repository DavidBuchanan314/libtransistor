#include<libtransistor/nx.h>
#include<libtransistor/fs/fsnet.h>

#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>

/*

PROTOCOL SPEC:

First Byte is a uint8_t representng the packet type

00 = Heartbeat
01 = open
02 = close
03 = read
04 = write
05 = lseek

HEARTBEAT:
	The server should echo back the payload.
	Used as a sanity check, and to keep the TCP session open
	request:
		uint8_t type = 00
		uint32_t payload
	response:
		uint32_t payload

OPEN:
	request:
		uint8_t type = 01
		uint32_t flags
		uint32_t mode
		uint8_t name_len
		uint8_t name[name_len]
	response:
		int32_t fd

CLOSE:
	request:
		uint8_t type = 02
		int32_t fd
	response:
		int32_t result

READ:
	request:
		uint8_t type = 03
		int32_t fd
		uint64_t length
	response:
		int64_t length
		uint8_t data[length]

WRITE:
	request:
		uint8_t type = 04
		int32_t fd
		uint64_t length
		uint8_t data[length]
	response:
		int64_t result

*/

int fsnet_init(uint32_t remote_ip, uint16_t remote_port) {
	if (fsnet_sock >= 0) {
		return -1;
	}
	
	fsnet_sock = bsd_socket(AF_INET, SOCK_STREAM, 6); // PROTO_TCP
	if (fsnet_sock < 0) {
		return -2;
	}
	
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(remote_port),
		.sin_addr = {
			.s_addr = remote_ip
		}
	};
	
	if (bsd_connect(fsnet_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		return -3;
	}
	
	if (fsnet_heartbeat() != 0) { // sanity check
		return -4;
	}
	
	return 0;
}

int fsnet_finalize(void) {
	if (bsd_close(fsnet_sock) < 0) {
		return -1;
	}
	fsnet_sock = -1;
	return 0;
}

int fsnet_heartbeat(void) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	char pkt_heartbeat[] = "\x00\xde\xad\xbe\xef";
	
	if (bsd_send(fsnet_sock, pkt_heartbeat, 5, 0) < 0) {
		return -2;
	}
	
	char buf[4];
	
	if (bsd_recv(fsnet_sock, buf, 4, 0) < 0) {
		return -3;
	}
	
	if (memcmp(buf, &pkt_heartbeat[1], sizeof(buf)) != 0) {
		return -4;
	}
	
	return 0;
}

int fsnet_open(const char * pathname, int flags, int mode) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	int pktlen = 1+4+4+1+strlen(pathname);
	uint8_t * buf = malloc(pktlen);
	
	buf[0] = PKT_OPEN;
	*(uint32_t *)(buf+1) = flags;
	*(uint32_t *)(buf+1+4) = mode;
	*(uint8_t *)(buf+1+4+4) = strlen(pathname);
	memcpy(buf+1+4+4+1, pathname, strlen(pathname));
	
	if (bsd_send(fsnet_sock, buf, pktlen, 0) < 0) {
		free(buf);
		return -2;
	}
	
	free(buf);
	
	int32_t fd;
	
	if (bsd_recv(fsnet_sock, &fd, sizeof(fd), 0) < 0) {
		return -3;
	}
	
	return fd;
}

int fsnet_close(int fd) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	uint8_t buf[5];
	
	buf[0] = PKT_CLOSE;
	*(uint32_t *)(buf+1) = fd;
	
	if (bsd_send(fsnet_sock, buf, sizeof(buf), 0) < 0) {
		return -2;
	}
	
	int32_t result;
	
	if (bsd_recv(fsnet_sock, &result, sizeof(result), 0) < 0) {
		return -3;
	}
	
	return result;
}

int64_t fsnet_read(int fd, void * destination, size_t len) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	uint8_t buf[1+4+8];
	
	buf[0] = PKT_READ;
	*(uint32_t *)(buf+1) = fd;
	*(uint64_t *)(buf+1+4) = len;
	
	if (bsd_send(fsnet_sock, buf, sizeof(buf), 0) < 0) {
		return -2;
	}
	
	int64_t resp_len;
	int64_t resp_remaining;
	uint8_t * response = (uint8_t *) destination;
	
	if (bsd_recv(fsnet_sock, &resp_len, sizeof(resp_len), 0) < 0) {
		return -3;
	}
	
	resp_remaining = resp_len;
	
	while (resp_remaining > 0) {
		int result = bsd_recv(fsnet_sock, response, resp_remaining, 0);
		if (result < 0) {
			return -5;
		}
		resp_remaining -= result;
		response += result;
	}
	
	return resp_len;
}

int64_t fsnet_write(int fd, void * source, size_t len) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	uint8_t buf[1+4+8];
	
	buf[0] = PKT_WRITE;
	*(int32_t *)(buf+1) = fd;
	*(uint64_t *)(buf+5) = len;
	
	if (bsd_send(fsnet_sock, buf, sizeof(buf), 0) < 0) {
		return -2;
	}
	
	size_t bytes_remaining = len;
	uint8_t * data = (uint8_t *)source;
	
	while (bytes_remaining > 0) {
		int result = bsd_send(fsnet_sock, data, bytes_remaining, 0);
		if (result < 0) {
			return -3;
		}
		bytes_remaining -= result;
		data += result;
	}
	
	int64_t result;
	
	if (bsd_recv(fsnet_sock, &result, sizeof(result), 0) < 0) {
		return -4;
	}
	
	return result;
}

int64_t fsnet_lseek(int fd, int offset, int whence) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	uint8_t buf[1+4+4+4];
	
	buf[0] = PKT_LSEEK;
	*(int32_t *)(buf+1) = fd;
	*(int32_t *)(buf+5) = offset;
	*(int32_t *)(buf+9) = whence;
	
	if (bsd_send(fsnet_sock, buf, sizeof(buf), 0) < 0) {
		return -2;
	}
	
	int64_t result;
	
	if (bsd_recv(fsnet_sock, &result, sizeof(result), 0) < 0) {
		return -3;
	}
	
	return result;
}

DIR *fsnet_opendir(const char *name) {
	if (fsnet_sock < 0) {
		return NULL;
	}
	
	int pktlen = 1+1+strlen(name);
	uint8_t * buf = malloc(pktlen);
	
	buf[0] = PKT_OPENDIR;
	buf[1] = strlen(name);
	memcpy(buf+2, name, strlen(name));
	
	if (bsd_send(fsnet_sock, buf, pktlen, 0) < 0) {
		free(buf);
		return NULL;
	}
	
	free(buf);
	
	DIR * dir = malloc(sizeof(dir));
	dir->ent.d_namlen = 0;//strnlen(name, sizeof(dir->ent.d_name));
	//strncpy(dir->ent.d_name, name, sizeof(dir->ent.d_name)); // XXX
	
	if (bsd_recv(fsnet_sock, &dir->id, sizeof(dir->id), 0) < 0 || dir->id < 0) {
		free(dir);
		return NULL;
	}
	
	return dir;
}

struct dirent *fsnet_readdir(DIR *dirp) {
	if (fsnet_sock < 0) {
		return -1;
	}
	
	uint8_t buf[5];
	
	buf[0] = PKT_READDIR;
	*(uint32_t *)(buf+1) = dirp->id;
	
	if (bsd_send(fsnet_sock, buf, sizeof(buf), 0) < 0) {
		return -2;
	}
	
	struct dirent *ent = malloc(sizeof(struct dirent));
	
	if (bsd_recv(fsnet_sock, &ent->d_namlen, sizeof(ent->d_namlen), 0) < 0) {
		free(ent);
		return NULL;
	}
	
	if (ent->d_namlen == 0 || bsd_recv(fsnet_sock, &ent->d_name, ent->d_namlen, 0) < 0) {
		free(ent);
		return NULL;
	}
	
	ent->d_name[ent->d_namlen] = 0;
	
	return ent;
}
