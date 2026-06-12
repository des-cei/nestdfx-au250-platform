#----------------------------------------------------------------------------
##
## Project    : The Xilinx PCI Express DMA
## File       : xcu250.xdc
## Version    : 1.0
##-----------------------------------------------------------------------------
#
# User Configuration
# Link Width   - x16
# Link Speed   - Gen3
# Family       - virtexuplus
# Part         - xcu250
# Package      - figd2104
# Speed grade  - -2L
#
# PCIe Block INT - 1
# PCIe Block STR - X0Y1
#

# Xilinx Reference Board is AU250

#----------------------------------------------------------------------------
# XDC file for the PCIe Gen3 x16 design
#-------------------------------------------------------------------------- -
set_property CONFIG_VOLTAGE 1.8                        [current_design]
set_property BITSTREAM.CONFIG.CONFIGFALLBACK Enable    [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE           [current_design]
set_property CONFIG_MODE SPIx4                         [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4           [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 72.9          [current_design]
set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN disable [current_design]
set_property BITSTREAM.CONFIG.SPI_FALL_EDGE YES        [current_design]
set_property BITSTREAM.CONFIG.UNUSEDPIN Pullup         [current_design]
set_property BITSTREAM.CONFIG.SPI_32BIT_ADDR Yes       [current_design]


create_pblock Stage1_IO
add_cells_to_pblock [get_pblocks Stage1_IO] [get_cells pcie_perstn_IBUF_inst/INBUF_INST]

#Configure LEDS

set_property PACKAGE_PIN BA20 [get_ports user_lnk_up_0]
set_property IOSTANDARD LVCMOS12 [get_ports user_lnk_up_0]
set_property DRIVE 8 [get_ports user_lnk_up_0]

#CMS(Card Management Solution)

    # Satellite UART
    set_property PACKAGE_PIN BA19 [get_ports satellite_uart_0_rxd]
    set_property -dict {IOSTANDARD LVCMOS12} [get_ports satellite_uart_0_rxd]
    set_property PACKAGE_PIN BB19 [get_ports satellite_uart_0_txd]
    set_property -dict {IOSTANDARD LVCMOS12 DRIVE 4} [get_ports satellite_uart_0_txd]

    # Satellite GPIO
    set_property PACKAGE_PIN AR20 [get_ports satellite_gpio_0[0]]
    set_property -dict {IOSTANDARD LVCMOS12} [get_ports satellite_gpio_0[0]]
    set_property PACKAGE_PIN AM20 [get_ports satellite_gpio_0[1]]
    set_property -dict {IOSTANDARD LVCMOS12} [get_ports satellite_gpio_0[1]]
    set_property PACKAGE_PIN AM21 [get_ports satellite_gpio_0[2]]
    set_property -dict {IOSTANDARD LVCMOS12} [get_ports satellite_gpio_0[2]]
    set_property PACKAGE_PIN AN21 [get_ports satellite_gpio_0[3]]
    set_property -dict {IOSTANDARD LVCMOS12} [get_ports satellite_gpio_0[3]]
    # QSFP/I2C Control
    set_property PACKAGE_PIN BE16 [get_ports qsfp0_modsel_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp0_modsel_l_0[0]]
    set_property PACKAGE_PIN BE17 [get_ports qsfp0_reset_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp0_reset_l_0[0]]
    set_property PACKAGE_PIN BD18 [get_ports qsfp0_lpmode_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp0_lpmode_0[0]]
    set_property PACKAGE_PIN BE20 [get_ports qsfp0_modprs_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp0_modprs_l_0[0]]
    set_property PACKAGE_PIN BE21 [get_ports qsfp0_int_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp0_int_l_0[0]]


    set_property PACKAGE_PIN AY20 [get_ports qsfp1_modsel_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp1_modsel_l_0[0]]
    set_property PACKAGE_PIN BC18 [get_ports qsfp1_reset_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp1_reset_l_0[0]]
    set_property PACKAGE_PIN AV22 [get_ports qsfp1_lpmode_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp1_lpmode_0[0]]
    set_property PACKAGE_PIN BC19 [get_ports qsfp1_modprs_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp1_modprs_l_0[0]]
    set_property PACKAGE_PIN AV21 [get_ports qsfp1_int_l_0[0]]
    set_property IOSTANDARD LVCMOS12 [get_ports qsfp1_int_l_0[0]]

#-------------------------------------------------
# pblock_inst_RP
#            for pr instance inst_RP
#-------------------------------------------------

create_pblock pblock_inst_RP
add_cells_to_pblock [get_pblocks pblock_inst_RP] [get_cells -quiet [list floorplan_static_i/reconfig_base_inst_0/U0]]
resize_pblock [get_pblocks pblock_inst_RP] -add {CLOCKREGION_X0Y8:CLOCKREGION_X7Y15 CLOCKREGION_X0Y0:CLOCKREGION_X3Y7}
set_property SNAPPING_MODE ON [get_pblocks pblock_inst_RP]
set_property IS_SOFT FALSE [get_pblocks pblock_inst_RP]


set_property DONT_TOUCH true [get_cells floorplan_static_i/reconfig_base_inst_0/U0]
set_property HD.RECONFIGURABLE true [get_cells floorplan_static_i/reconfig_base_inst_0/U0]


