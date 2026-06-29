CC ?= gcc
AR ?= ar

CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L

LDFLAGS ?=
LDLIBS ?= -pthread

# Software example selected by the user.
# It must match a file in accelerators/<ACCEL>.c
ACCEL ?= timer_bram

ACCELS := $(patsubst accelerators/%.c,%,$(wildcard accelerators/*.c))

LIB := libalveo.a
LIB_OBJ := alveo.o

TARGET := $(ACCEL)
ACCEL_SRC := accelerators/$(ACCEL).c
ACCEL_OBJ := accelerators/$(ACCEL).o

.PHONY: all clean clean_app list_accelerators check_app

all: check_app $(LIB) $(TARGET)

check_app:
	@test -f $(ACCEL_SRC) || \
		(echo "ERROR: unknown software accelerator '$(ACCEL)'"; exit 1)
	@echo "Selected software accelerator: $(ACCEL)"

list_accelerators:
	@printf "%s\n" $(ACCELS) | sort

$(LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^

$(TARGET): $(ACCEL_OBJ) $(LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $(ACCEL_OBJ) -L. -lalveo $(LDLIBS)

%.o: %.c alveo.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean_app:
	rm -f $(ACCEL_OBJ) $(TARGET)

clean:
	rm -f $(LIB_OBJ) accelerators/*.o $(LIB) $(ACCELS)
