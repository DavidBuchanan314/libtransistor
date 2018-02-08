# DLFCN

dlfcn_SRCS=	dlfcn.c
dlfcn_OBJECT_FILES := $(addprefix $(LIBTRANSISTOR_HOME)/build/dlfcn/,$(dlfcn_SRCS:.c=.o))

dlfcn_CC_FLAGS := -I$(LIBTRANSISTOR_HOME)/dlfcn

# ARCHIVE RULES

$(LIBTRANSISTOR_HOME)/build/dlfcn/libdl.a: $(dlfcn_OBJECT_FILES)
	mkdir -p $(@D)
	rm -f $@
	$(AR) $(AR_FLAGS) $@ $+

# BUILD RULES

$(LIBTRANSISTOR_HOME)/build/dlfcn/%.o: $(LIBTRANSISTOR_HOME)/dlfcn/%.c
	mkdir -p $(@D)
	$(CC) $(CC_FLAGS) $(WARNINGS) $(dlfcn_CC_FLAGS) -c -o $@ $<

# CLEAN RULES

.PHONY: clean_dlfcn

clean_dlfcn:
	rm -fr build/dlfcn
