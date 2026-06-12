#Define first and second order Reconfigurable Partitions
set firstRP1 "A1_level1"

#Create folders for storing full, first-order and second-order bitstreams
file mkdir ./$firstRP1


#Generate a full bitstream for power-on configuration, and second order partial bistreams for shifters
puts "#HD: Generating partial bitstreams for reconfig"
open_checkpoint ../implementation_A1/A1/top_A1_locked.dcp

write_bitstream -force -cell floorplan_static_i/reconfig_base_inst_0/U0 ./$firstRP1/top_A1_reconfig.bit
write_cfgmem -force -disablebitswap -interface SMAPx32 -format BIN -loadbit "up 0x0 ./$firstRP1/top_A1_reconfig.bit" -file "../bin/top_A1_reconfig.bin"
puts "	#HD: Completed"
close_project

