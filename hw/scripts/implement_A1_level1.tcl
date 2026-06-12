set loc "./A1"
set part xcu250-figd2104-2L-e



file delete -force $loc
file mkdir $loc
file mkdir $loc/reports

# Open checkpoint
open_checkpoint ../implementation_A0/top_static/top_A0_locked.dcp

read_checkpoint -cell [get_cells floorplan_static_i/reconfig_base_inst_0/U0] ../synthesis/reconfig.dcp

# Run implementation
opt_design
place_design
route_design

# Save checkpoint
write_checkpoint -force $loc/top_A1_locked.dcp
# Close Vivado project
close_project


