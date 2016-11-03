TARGET          := imk
CC              ?= cc
BUILD_HOST      := build_host.h
SRC             != (ls *.c || true)
OS              != (uname -s || true)

.if $(OS) == Linux
    GRP    := root
    SRC    += compat/poll_linux.c
.elif $(OS) == OpenBSD || $(OS) == FreeBSD || $(OS) == Darwin
    GRP    := wheel
    SRC    += compat/poll_bsd.c
.endif

INSTALL         := install
INSTALL_ARGS    := -o root -g $(GRP) -m 755
INSTALL_DIR     := /usr/local/bin/

OBJ             := $(SRC:.c=.o)

INCLUDES        :=
LIBS            :=

CFLAGS          := -Wall $(INCLUDES)
LFLAGS          := $(LIBS)

.if $(CC) == cc || $(CC) == clang || $(CC) == gcc
    CFLAGS := -std=c99 -pedantic
.endif

.if make(release)
    CFLAGS += -O3
.else  # debug
    CFLAGS += -g -ggdb -DDEBUG
    LFLAGS += -g
.endif

all: debug
debug: build
release: clean build
	strip $(TARGET)

build: $(BUILD_HOST) $(TARGET)

$(BUILD_HOST):
	@echo "#define BUILD_HOST \"`hostname`\""      > $(BUILD_HOST)
	@echo "#define BUILD_OS \"`uname`\""          >> $(BUILD_HOST)
	@echo "#define BUILD_PLATFORM \"`uname -m`\"" >> $(BUILD_HOST)
	@echo "#define BUILD_KERNEL \"`uname -r`\""   >> $(BUILD_HOST)

$(TARGET): $(BUILD_HOST) $(OBJ)
	$(CC) $(LFLAGS) -o $@ $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $?

install: release
	$(INSTALL) $(INSTALL_ARGS) $(TARGET) $(INSTALL_DIR)
	@echo "DONE"

clean:
	-rm -f *.core
	-rm -f $(BUILD_HOST)
	-rm -f $(TARGET)
	-rm -f *.o compat/*.o

.PHONY : all debug release build run clean
