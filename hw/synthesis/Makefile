.PHONY: all clean

dcp_obj := reconfig.dcp

all: $(dcp_obj)

$(dcp_obj): ../scripts/syn_reconfig.tcl
	./run.sh $<

clean:
	rm -rf $(dcp_obj) *.log *.rpt *.jou .Xil
