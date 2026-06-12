.PHONY: all clean

stamp := build_bitstream.stamp

all: $(stamp)

$(stamp): ../scripts/generate_all_bitstreams.tcl run.sh
	./run.sh $<
	touch $@

clean:
	rm -rf *.log *.mcs *.prm *.rpt *.jou .Xil A1_level1 ./hd_visual ../bin/* $(stamp)
