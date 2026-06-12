.PHONY: all hw sw clean hw_clean sw_clean

ACCEL ?= timer_bram
JOBS ?= 8

all: hw sw

hw:
	$(MAKE) -C hw ACCEL=$(ACCEL) JOBS=$(JOBS)

sw:
	$(MAKE) -C sw ACCEL=$(ACCEL)

clean: hw_clean sw_clean

hw_clean:
	$(MAKE) -C hw clean

sw_clean:
	$(MAKE) -C sw clean
