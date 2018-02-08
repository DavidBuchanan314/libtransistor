#include <stdio.h>
#include <dlfcn.h>

int dlclose(void * handle) {
	return -1; // TODO
}

char *dlerror(void) {
	return NULL; // TODO
}

void *dlopen(const char * file, int mode) {
	return NULL; // TODO
}

void *dlsym(void *restrict handle, const char *restrict name) {
	return NULL; // TODO
}
