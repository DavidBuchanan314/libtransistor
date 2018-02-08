#include <dlfcn.h>
#include <stdio.h>

void (*dynamic_hello)(void);

int main() {
	printf("Calling dlopen...\n");
	void * handle = dlopen("./test_dlfcn_hello.nro", RTLD_NOW);
	if (handle == NULL) {
		printf("dlopen failed\n");
		return -1;
	}
	
	printf("Calling dlsym...\n");
	dynamic_hello = dlsym(handle, "hello");
	if (dynamic_hello == NULL) {
		printf("dlsym failed\n");
		return -1;
	}
	
	printf("Calling dynamic_hello...\n");
	dynamic_hello();
	
	
	printf("Calling dlclose...\n");
	if (dlclose(handle) != 0) {
		printf("dlclose failed\n");
	}
	
	printf("dlfcn tests passed, probably.\n");
	return 0;
}
