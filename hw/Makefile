.PHONY: all generate_bit level2_implementation out_of_context_syn vivado_dfx_proj check_accel clean

JOBS ?= 8
ACCEL ?= timer_bram

ACCEL_DIR := accelerators/$(ACCEL)

all: generate_bit

check_accel:
	@test -d $(ACCEL_DIR) || (echo "ERROR: unknown accelerator '$(ACCEL)'"; exit 1)
	@test -f $(ACCEL_DIR)/project_gen_reconfig.tcl || \
		(echo "ERROR: missing $(ACCEL_DIR)/project_gen_reconfig.tcl"; exit 1)
	@echo "Selected accelerator: $(ACCEL)"

generate_bit: implementation_A1
	$(MAKE) -C bitstreams all -j$(JOBS)

implementation_A1: out_of_context_syn
	$(MAKE) -C implementation_A1 all -j$(JOBS)

out_of_context_syn: vivado_dfx_proj
	$(MAKE) -C synthesis all -j$(JOBS)

vivado_dfx_proj: check_accel
	$(MAKE) -C dfx_prj all -j$(JOBS)

clean:
	$(MAKE) -C bitstreams clean
	$(MAKE) -C implementation_A1 clean
	$(MAKE) -C synthesis clean
	$(MAKE) -C dfx_prj clean
