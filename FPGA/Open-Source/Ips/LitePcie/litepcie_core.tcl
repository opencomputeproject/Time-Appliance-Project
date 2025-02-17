
# Create Project

create_project -force -name litepcie_core -part xc7a
set_msg_config -id {Common 17-55} -new_severity {Warning}

# Add project commands


# Add Sources

read_verilog {/home/thomas/Litex/litepcie/litepcie/frontend/ptm/sniffer_tap.v}
read_verilog {/media/sf_litepcie/litepcie/build/gateware/litepcie_core.v}

# Add EDIFs


# Add IPs


# Add constraints

read_xdc litepcie_core.xdc
set_property PROCESSING_ORDER EARLY [get_files litepcie_core.xdc]

# Add pre-synthesis commands

create_ip -vendor xilinx.com -name pcie_7x -module_name pcie_s7
set obj [get_ips pcie_s7]
set_property -dict [list \
CONFIG.Bar0_Scale {Megabytes} \
CONFIG.Bar0_Size {1} \
CONFIG.Buf_Opt_BMA {True} \
CONFIG.Component_Name {pcie} \
CONFIG.Device_ID {7021} \
CONFIG.Interface_Width {64_bit} \
CONFIG.Link_Speed {5.0_GT/s} \
CONFIG.Max_Payload_Size {512_bytes} \
CONFIG.Maximum_Link_Width {X1} \
CONFIG.PCIe_Blk_Locn {X0Y0} \
CONFIG.Ref_Clk_Freq {100_MHz} \
CONFIG.Trans_Buf_Pipeline {None} \
CONFIG.Trgt_Link_Speed {4'h2} \
CONFIG.User_Clk_Freq {125} \
CONFIG.Legacy_Interrupt {None} \
CONFIG.IntX_Generation {False} \
CONFIG.MSI_64b {False} \
CONFIG.Multiple_Message_Capable {1_vector} \
] $obj
synth_ip $obj

# Synthesis

synth_design -directive default -top litepcie_core -part xc7a

# Synthesis report

report_timing_summary -file litepcie_core_timing_synth.rpt
report_utilization -hierarchical -file litepcie_core_utilization_hierarchical_synth.rpt
report_utilization -file litepcie_core_utilization_synth.rpt
write_checkpoint -force litepcie_core_synth.dcp

# Add pre-optimize commands

set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_ctl_in[0]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_ctl_in[0]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_k_wire_filter[0] -objects pcie_ptm_sniffer_tap/rx_ctl_in[0]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_ctl_in[1]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_ctl_in[1]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_k_wire_filter[1] -objects pcie_ptm_sniffer_tap/rx_ctl_in[1]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[0]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[0]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[0] -objects pcie_ptm_sniffer_tap/rx_data_in[0]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[1]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[1]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[1] -objects pcie_ptm_sniffer_tap/rx_data_in[1]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[2]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[2]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[2] -objects pcie_ptm_sniffer_tap/rx_data_in[2]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[3]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[3]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[3] -objects pcie_ptm_sniffer_tap/rx_data_in[3]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[4]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[4]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[4] -objects pcie_ptm_sniffer_tap/rx_data_in[4]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[5]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[5]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[5] -objects pcie_ptm_sniffer_tap/rx_data_in[5]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[6]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[6]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[6] -objects pcie_ptm_sniffer_tap/rx_data_in[6]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[7]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[7]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[7] -objects pcie_ptm_sniffer_tap/rx_data_in[7]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[8]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[8]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[8] -objects pcie_ptm_sniffer_tap/rx_data_in[8]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[9]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[9]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[9] -objects pcie_ptm_sniffer_tap/rx_data_in[9]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[10]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[10]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[10] -objects pcie_ptm_sniffer_tap/rx_data_in[10]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[11]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[11]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[11] -objects pcie_ptm_sniffer_tap/rx_data_in[11]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[12]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[12]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[12] -objects pcie_ptm_sniffer_tap/rx_data_in[12]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[13]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[13]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[13] -objects pcie_ptm_sniffer_tap/rx_data_in[13]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[14]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[14]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[14] -objects pcie_ptm_sniffer_tap/rx_data_in[14]
set pin_driver [get_nets -of [get_pins pcie_ptm_sniffer_tap/rx_data_in[15]]]
disconnect_net -net $pin_driver -objects pcie_ptm_sniffer_tap/rx_data_in[15]
connect_net -hier -net pcie_s7/inst/inst/gt_top_i/gt_rx_data_wire_filter[15] -objects pcie_ptm_sniffer_tap/rx_data_in[15]

# Optimize design

opt_design -directive default

# Add pre-placement commands

reset_property LOC [get_cells -hierarchical -filter {NAME=~pcie_s7/*gtp_common.gtpe2_common_i}]
reset_property LOC [get_cells -hierarchical -filter {NAME=~pcie_s7/*genblk*.bram36_tdp_bl.bram36_tdp_bl}]

# Placement

place_design -directive default

# Placement report

report_utilization -hierarchical -file litepcie_core_utilization_hierarchical_place.rpt
report_utilization -file litepcie_core_utilization_place.rpt
report_io -file litepcie_core_io.rpt
report_control_sets -verbose -file litepcie_core_control_sets.rpt
report_clock_utilization -file litepcie_core_clock_utilization.rpt
write_checkpoint -force litepcie_core_place.dcp

# Add pre-routing commands


# Routing

route_design -directive default
phys_opt_design -directive default
write_checkpoint -force litepcie_core_route.dcp

# Routing report

report_timing_summary -no_header -no_detailed_paths
report_route_status -file litepcie_core_route_status.rpt
report_drc -file litepcie_core_drc.rpt
report_timing_summary -datasheet -max_paths 10 -file litepcie_core_timing.rpt
report_power -file litepcie_core_power.rpt

# Bitstream generation

write_bitstream -force litepcie_core.bit 

# End

quit