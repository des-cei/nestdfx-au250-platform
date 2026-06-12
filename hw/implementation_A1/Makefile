.PHONY: all clean

dcp_obj := ./A1/top_A1_locked.dcp

all: $(dcp_obj)

$(dcp_obj): ../scripts/implement_A1_level1.tcl
	./run.sh $<
	@test -f $@ || { echo "Error: $@ not created"; exit 1; }

clean:
	rm -rf *.txt *.log *.rpt *.jou .Xil A1 hd_visual cong .cache .tmpCRC