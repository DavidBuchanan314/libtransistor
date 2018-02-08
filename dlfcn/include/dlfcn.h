#ifndef	_DLFCN_H
#define	_DLFCN_H 1

#define RTLD_LAZY 0
#define RTLD_NOW 1
#define RTLD_GLOBAL 2
#define RTLD_LOCAL 3

int dlclose(void * handle);
char *dlerror(void);
void *dlopen(const char * file, int mode);
void *dlsym(void *restrict handle, const char *restrict name);

#endif
