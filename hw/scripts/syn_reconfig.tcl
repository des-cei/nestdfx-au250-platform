set dcp_file_name reconfig
set top reconfig_base_inst
set bd_name reconfig_base_inst_bd

# Add and read the IP file
open_project ../dfx_prj/reconfig_prj/reconfig.xpr

set_param general.maxThreads  8
set_property XPM_LIBRARIES {XPM_CDC XPM_MEMORY XPM_FIFO} [current_project]

set logFileId [open ./runLog_$dcp_file_name.log "w"]
set start_time [clock seconds]

puts "\[DFX\] generating reconfigurable module $top"

set bd_file [get_files *${bd_name}.bd]

# Generate output products
generate_target all $bd_file

# Export IP user files
export_ip_user_files -of_objects $bd_file -no_script -sync -force -quiet

# Create specific IP run
create_ip_run $bd_file

# Launch all generated OOC IP synthesis runs for this BD dynamically
set ip_runs [get_runs -quiet -filter "NAME =~ ${bd_name}_*_synth_1"]

if {[llength $ip_runs] > 0} {
    launch_runs $ip_runs -jobs 8

    foreach r $ip_runs {
        wait_on_run $r
    }
}

synth_design -top $top -part xcu250-figd2104-2L-e -mode out_of_context

write_checkpoint -force ${dcp_file_name}.dcp

set end_time [clock seconds]
set total_seconds [expr $end_time - $start_time]
puts $logFileId "syn: $total_seconds seconds"

report_utilization -hierarchical > ${top}_utilization.rpt




