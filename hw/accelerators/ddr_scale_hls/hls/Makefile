CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -I./src

TB_BIN := ddr_scale_tb

.PHONY: all tb clean

all: tb

tb: $(TB_BIN)

$(TB_BIN): src/ddr_scale_accel.cpp tb/ddr_scale_accel_tb.cpp src/ddr_scale_accel.h
	$(CXX) $(CXXFLAGS) src/ddr_scale_accel.cpp tb/ddr_scale_accel_tb.cpp -o $(TB_BIN)

clean:
	rm -f $(TB_BIN)
