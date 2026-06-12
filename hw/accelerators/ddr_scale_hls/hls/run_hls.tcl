# Vitis HLS script for the DDR pointer-model accelerator example.
# Usage:
#   vitis_hls -f run_hls.tcl

set project_name ddr_scale_accel_hls
set solution_name sol1
set top_name ddr_scale_accel
set part_name xcu250-figd2104-2L-e
set clk_period_ns 3.333

open_project -reset $project_name
set_top $top_name

add_files src/ddr_scale_accel.cpp
add_files src/ddr_scale_accel.h
add_files -tb tb/ddr_scale_accel_tb.cpp

open_solution -reset $solution_name
set_part $part_name
create_clock -period $clk_period_ns -name default

csim_design
csynth_design
export_design -format ip_catalog

exit
