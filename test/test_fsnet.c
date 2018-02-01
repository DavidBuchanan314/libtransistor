#include<libtransistor/nx.h>
#include<libtransistor/fs/fsnet.h>

#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include <fcntl.h>

int main() {
	svcSleepThread(100000000);
  
	result_t r;
	if((r = sm_init()) != RESULT_OK) {
		printf("failed to init sm: 0x%x\n", r);
		return 1;
	}

	if((r = bsd_init()) != RESULT_OK) {
		printf("failed to init bsd: 0x%x, %d\n", r, bsd_errno);
		goto err_only_sm;
	}

	char server_ip_addr[4] = {192, 168, 0, 36};

	if((r = fsnet_init(*(uint32_t*) server_ip_addr, 1337)) != RESULT_OK) {
		printf("failed to init fsnet: 0x%x\n", r);
		goto err_fsnet;
	}

	//FILE * fd = fopen("test", "r");
	//char buf[32];
	//printf("%u bytes read\n", fread(buf, 1, sizeof(buf), fd));
	//printf("Result: \"%s\"\n", buf);
	//fclose(fd);

	printf("%08x\n", O_RDWR);
	printf("%08x\n", O_CREAT);
	printf("%08x\n", O_TRUNC);
	printf("%08x\n", O_APPEND);

	printf("fsnet is alive!\n");
	
	svcSleepThread(100000000);
	
	FILE * f = fopen("test", "r");
	char buf[32];
	printf("%u bytes read\n", fread(buf, 1, sizeof(buf), f));
	printf("Result: \"%s\"\n", buf);
	fclose(f);
	
	f = fopen("write_test", "w+");
	if (f != NULL) {
		char * hello = "Hello from the switch!\n";
		printf("%u bytes written\n", fwrite(hello, 1, strlen(hello), f));
		fclose(f);
	} else {
		printf("fopen failed\n");
	}
	
	svcSleepThread(100000000);

	fsnet_finalize();
	bsd_finalize();
	sm_finalize();
	printf("fsnet tests passed.\n");
	return 0;

err:
	fsnet_finalize();
err_fsnet:
	bsd_finalize();
err_only_sm:
	sm_finalize();
	return 1;  
}
