.PHONY: all clean

ACCEL ?= timer_bram

ACCEL_TCL := ../accelerators/$(ACCEL)/project_gen_reconfig.tcl

stamp := .project_gen_$(ACCEL).stamp

all: $(stamp)

$(stamp): $(ACCEL_TCL) run.sh
	@echo "Generating Vivado DFX project for accelerator: $(ACCEL)"
	@echo "Using project TCL: $(ACCEL_TCL)"
	rm -rf reconfig_prj
	./run.sh $(ACCEL_TCL)
	touch $@

clean:
	rm -rf .project_gen_*.stamp *.log *.rpt *.jou .Xil reconfig_prj hd_visual