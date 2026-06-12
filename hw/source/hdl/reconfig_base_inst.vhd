----------------------------------------------------------------------------------
-- Fixed DFX top wrapper for the reconfigurable region.
--
-- This entity is the shell-facing boundary and must keep the exact same port list
-- expected by the static design.
--
-- Internally it instantiates the generated block design wrapper:
--
--     reconfig_base_inst_bd_wrapper
--
-- The internal BD is normalized so that it does not expose the optional
-- M00_AXI_0_aruser / M00_AXI_0_awuser AXI sideband ports. Those shell-facing
-- ports are tied to zero here, making the DFX boundary stable regardless of
-- whether the internal HW is the tutorial traffic generator, an HLS IP, or
-- another accelerator.
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity reconfig_base_inst is
  port (
    M00_AXI_0_araddr  : out STD_LOGIC_VECTOR (31 downto 0);
    M00_AXI_0_arburst : out STD_LOGIC_VECTOR (1 downto 0);
    M00_AXI_0_arcache : out STD_LOGIC_VECTOR (3 downto 0);
    M00_AXI_0_arlen   : out STD_LOGIC_VECTOR (7 downto 0);
    M00_AXI_0_arlock  : out STD_LOGIC_VECTOR (0 to 0);
    M00_AXI_0_arprot  : out STD_LOGIC_VECTOR (2 downto 0);
    M00_AXI_0_arqos   : out STD_LOGIC_VECTOR (3 downto 0);
    M00_AXI_0_arready : in  STD_LOGIC;
    M00_AXI_0_arsize  : out STD_LOGIC_VECTOR (2 downto 0);
    M00_AXI_0_aruser  : out STD_LOGIC_VECTOR (7 downto 0);
    M00_AXI_0_arvalid : out STD_LOGIC;

    M00_AXI_0_awaddr  : out STD_LOGIC_VECTOR (31 downto 0);
    M00_AXI_0_awburst : out STD_LOGIC_VECTOR (1 downto 0);
    M00_AXI_0_awcache : out STD_LOGIC_VECTOR (3 downto 0);
    M00_AXI_0_awlen   : out STD_LOGIC_VECTOR (7 downto 0);
    M00_AXI_0_awlock  : out STD_LOGIC_VECTOR (0 to 0);
    M00_AXI_0_awprot  : out STD_LOGIC_VECTOR (2 downto 0);
    M00_AXI_0_awqos   : out STD_LOGIC_VECTOR (3 downto 0);
    M00_AXI_0_awready : in  STD_LOGIC;
    M00_AXI_0_awsize  : out STD_LOGIC_VECTOR (2 downto 0);
    M00_AXI_0_awuser  : out STD_LOGIC_VECTOR (7 downto 0);
    M00_AXI_0_awvalid : out STD_LOGIC;

    M00_AXI_0_bready  : out STD_LOGIC;
    M00_AXI_0_bresp   : in  STD_LOGIC_VECTOR (1 downto 0);
    M00_AXI_0_bvalid  : in  STD_LOGIC;

    M00_AXI_0_rdata   : in  STD_LOGIC_VECTOR (31 downto 0);
    M00_AXI_0_rlast   : in  STD_LOGIC;
    M00_AXI_0_rready  : out STD_LOGIC;
    M00_AXI_0_rresp   : in  STD_LOGIC_VECTOR (1 downto 0);
    M00_AXI_0_rvalid  : in  STD_LOGIC;

    M00_AXI_0_wdata   : out STD_LOGIC_VECTOR (31 downto 0);
    M00_AXI_0_wlast   : out STD_LOGIC;
    M00_AXI_0_wready  : in  STD_LOGIC;
    M00_AXI_0_wstrb   : out STD_LOGIC_VECTOR (3 downto 0);
    M00_AXI_0_wvalid  : out STD_LOGIC;

    S00_AXI_0_araddr  : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_0_arprot  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_0_arready : out STD_LOGIC;
    S00_AXI_0_arvalid : in  STD_LOGIC;
    S00_AXI_0_awaddr  : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_0_awprot  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_0_awready : out STD_LOGIC;
    S00_AXI_0_awvalid : in  STD_LOGIC;
    S00_AXI_0_bready  : in  STD_LOGIC;
    S00_AXI_0_bresp   : out STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_0_bvalid  : out STD_LOGIC;
    S00_AXI_0_rdata   : out STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_0_rready  : in  STD_LOGIC;
    S00_AXI_0_rresp   : out STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_0_rvalid  : out STD_LOGIC;
    S00_AXI_0_wdata   : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_0_wready  : out STD_LOGIC;
    S00_AXI_0_wstrb   : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_0_wvalid  : in  STD_LOGIC;

    S00_AXI_1_araddr  : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_1_arburst : in  STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_1_arcache : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_1_arid    : in  STD_LOGIC_VECTOR (11 downto 0);
    S00_AXI_1_arlen   : in  STD_LOGIC_VECTOR (7 downto 0);
    S00_AXI_1_arlock  : in  STD_LOGIC_VECTOR (0 to 0);
    S00_AXI_1_arprot  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_1_arqos   : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_1_arready : out STD_LOGIC;
    S00_AXI_1_arsize  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_1_aruser  : in  STD_LOGIC_VECTOR (0 to 0);
    S00_AXI_1_arvalid : in  STD_LOGIC;

    S00_AXI_1_awaddr  : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_1_awburst : in  STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_1_awcache : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_1_awid    : in  STD_LOGIC_VECTOR (11 downto 0);
    S00_AXI_1_awlen   : in  STD_LOGIC_VECTOR (7 downto 0);
    S00_AXI_1_awlock  : in  STD_LOGIC_VECTOR (0 to 0);
    S00_AXI_1_awprot  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_1_awqos   : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_1_awready : out STD_LOGIC;
    S00_AXI_1_awsize  : in  STD_LOGIC_VECTOR (2 downto 0);
    S00_AXI_1_awuser  : in  STD_LOGIC_VECTOR (0 to 0);
    S00_AXI_1_awvalid : in  STD_LOGIC;

    S00_AXI_1_bid     : out STD_LOGIC_VECTOR (11 downto 0);
    S00_AXI_1_bready  : in  STD_LOGIC;
    S00_AXI_1_bresp   : out STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_1_buser   : out STD_LOGIC_VECTOR (0 to 0);
    S00_AXI_1_bvalid  : out STD_LOGIC;

    S00_AXI_1_rdata   : out STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_1_rid     : out STD_LOGIC_VECTOR (11 downto 0);
    S00_AXI_1_rlast   : out STD_LOGIC;
    S00_AXI_1_rready  : in  STD_LOGIC;
    S00_AXI_1_rresp   : out STD_LOGIC_VECTOR (1 downto 0);
    S00_AXI_1_rvalid  : out STD_LOGIC;

    S00_AXI_1_wdata   : in  STD_LOGIC_VECTOR (31 downto 0);
    S00_AXI_1_wlast   : in  STD_LOGIC;
    S00_AXI_1_wready  : out STD_LOGIC;
    S00_AXI_1_wstrb   : in  STD_LOGIC_VECTOR (3 downto 0);
    S00_AXI_1_wvalid  : in  STD_LOGIC;

    interrupt         : out STD_LOGIC_VECTOR (3 downto 0);
    s_axi_aclk        : in  STD_LOGIC;
    s_axi_aresetn     : in  STD_LOGIC
  );
end reconfig_base_inst;

architecture Behavioral of reconfig_base_inst is

  ATTRIBUTE X_INTERFACE_PARAMETER : STRING;
  ATTRIBUTE X_INTERFACE_PARAMETER of s_axi_aclk : SIGNAL is
    "ASSOCIATED_BUSIF S00_AXI_0:S00_AXI_1:M00_AXI_0, ASSOCIATED_RESET s_axi_aresetn";

  component reconfig_base_inst_bd_wrapper is
    port (
      M00_AXI_0_araddr  : out STD_LOGIC_VECTOR (31 downto 0);
      M00_AXI_0_arburst : out STD_LOGIC_VECTOR (1 downto 0);
      M00_AXI_0_arcache : out STD_LOGIC_VECTOR (3 downto 0);
      M00_AXI_0_arlen   : out STD_LOGIC_VECTOR (7 downto 0);
      M00_AXI_0_arlock  : out STD_LOGIC_VECTOR (0 to 0);
      M00_AXI_0_arprot  : out STD_LOGIC_VECTOR (2 downto 0);
      M00_AXI_0_arqos   : out STD_LOGIC_VECTOR (3 downto 0);
      M00_AXI_0_arready : in  STD_LOGIC;
      M00_AXI_0_arsize  : out STD_LOGIC_VECTOR (2 downto 0);
      M00_AXI_0_arvalid : out STD_LOGIC;

      M00_AXI_0_awaddr  : out STD_LOGIC_VECTOR (31 downto 0);
      M00_AXI_0_awburst : out STD_LOGIC_VECTOR (1 downto 0);
      M00_AXI_0_awcache : out STD_LOGIC_VECTOR (3 downto 0);
      M00_AXI_0_awlen   : out STD_LOGIC_VECTOR (7 downto 0);
      M00_AXI_0_awlock  : out STD_LOGIC_VECTOR (0 to 0);
      M00_AXI_0_awprot  : out STD_LOGIC_VECTOR (2 downto 0);
      M00_AXI_0_awqos   : out STD_LOGIC_VECTOR (3 downto 0);
      M00_AXI_0_awready : in  STD_LOGIC;
      M00_AXI_0_awsize  : out STD_LOGIC_VECTOR (2 downto 0);
      M00_AXI_0_awvalid : out STD_LOGIC;

      M00_AXI_0_bready  : out STD_LOGIC;
      M00_AXI_0_bresp   : in  STD_LOGIC_VECTOR (1 downto 0);
      M00_AXI_0_bvalid  : in  STD_LOGIC;

      M00_AXI_0_rdata   : in  STD_LOGIC_VECTOR (31 downto 0);
      M00_AXI_0_rlast   : in  STD_LOGIC;
      M00_AXI_0_rready  : out STD_LOGIC;
      M00_AXI_0_rresp   : in  STD_LOGIC_VECTOR (1 downto 0);
      M00_AXI_0_rvalid  : in  STD_LOGIC;

      M00_AXI_0_wdata   : out STD_LOGIC_VECTOR (31 downto 0);
      M00_AXI_0_wlast   : out STD_LOGIC;
      M00_AXI_0_wready  : in  STD_LOGIC;
      M00_AXI_0_wstrb   : out STD_LOGIC_VECTOR (3 downto 0);
      M00_AXI_0_wvalid  : out STD_LOGIC;

      S00_AXI_0_araddr  : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_0_arprot  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_0_arready : out STD_LOGIC;
      S00_AXI_0_arvalid : in  STD_LOGIC;
      S00_AXI_0_awaddr  : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_0_awprot  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_0_awready : out STD_LOGIC;
      S00_AXI_0_awvalid : in  STD_LOGIC;
      S00_AXI_0_bready  : in  STD_LOGIC;
      S00_AXI_0_bresp   : out STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_0_bvalid  : out STD_LOGIC;
      S00_AXI_0_rdata   : out STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_0_rready  : in  STD_LOGIC;
      S00_AXI_0_rresp   : out STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_0_rvalid  : out STD_LOGIC;
      S00_AXI_0_wdata   : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_0_wready  : out STD_LOGIC;
      S00_AXI_0_wstrb   : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_0_wvalid  : in  STD_LOGIC;

      S00_AXI_1_araddr  : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_1_arburst : in  STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_1_arcache : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_1_arid    : in  STD_LOGIC_VECTOR (11 downto 0);
      S00_AXI_1_arlen   : in  STD_LOGIC_VECTOR (7 downto 0);
      S00_AXI_1_arlock  : in  STD_LOGIC_VECTOR (0 to 0);
      S00_AXI_1_arprot  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_1_arqos   : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_1_arready : out STD_LOGIC;
      S00_AXI_1_arsize  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_1_aruser  : in  STD_LOGIC_VECTOR (0 to 0);
      S00_AXI_1_arvalid : in  STD_LOGIC;

      S00_AXI_1_awaddr  : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_1_awburst : in  STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_1_awcache : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_1_awid    : in  STD_LOGIC_VECTOR (11 downto 0);
      S00_AXI_1_awlen   : in  STD_LOGIC_VECTOR (7 downto 0);
      S00_AXI_1_awlock  : in  STD_LOGIC_VECTOR (0 to 0);
      S00_AXI_1_awprot  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_1_awqos   : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_1_awready : out STD_LOGIC;
      S00_AXI_1_awsize  : in  STD_LOGIC_VECTOR (2 downto 0);
      S00_AXI_1_awuser  : in  STD_LOGIC_VECTOR (0 to 0);
      S00_AXI_1_awvalid : in  STD_LOGIC;

      S00_AXI_1_bid     : out STD_LOGIC_VECTOR (11 downto 0);
      S00_AXI_1_bready  : in  STD_LOGIC;
      S00_AXI_1_bresp   : out STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_1_buser   : out STD_LOGIC_VECTOR (0 to 0);
      S00_AXI_1_bvalid  : out STD_LOGIC;

      S00_AXI_1_rdata   : out STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_1_rid     : out STD_LOGIC_VECTOR (11 downto 0);
      S00_AXI_1_rlast   : out STD_LOGIC;
      S00_AXI_1_rready  : in  STD_LOGIC;
      S00_AXI_1_rresp   : out STD_LOGIC_VECTOR (1 downto 0);
      S00_AXI_1_rvalid  : out STD_LOGIC;

      S00_AXI_1_wdata   : in  STD_LOGIC_VECTOR (31 downto 0);
      S00_AXI_1_wlast   : in  STD_LOGIC;
      S00_AXI_1_wready  : out STD_LOGIC;
      S00_AXI_1_wstrb   : in  STD_LOGIC_VECTOR (3 downto 0);
      S00_AXI_1_wvalid  : in  STD_LOGIC;

      interrupt         : out STD_LOGIC_VECTOR (3 downto 0);
      s_axi_aclk        : in  STD_LOGIC;
      s_axi_aresetn     : in  STD_LOGIC
    );
  end component;

begin

  -- The static shell expects these ports to exist because they are part of the
  -- original DFX boundary. The internal BD is normalized and does not generate
  -- them, so we provide a default AXI USER value here.
  M00_AXI_0_aruser <= (others => '0');
  M00_AXI_0_awuser <= (others => '0');

  bd_i : reconfig_base_inst_bd_wrapper
    port map (
      M00_AXI_0_araddr  => M00_AXI_0_araddr,
      M00_AXI_0_arburst => M00_AXI_0_arburst,
      M00_AXI_0_arcache => M00_AXI_0_arcache,
      M00_AXI_0_arlen   => M00_AXI_0_arlen,
      M00_AXI_0_arlock  => M00_AXI_0_arlock,
      M00_AXI_0_arprot  => M00_AXI_0_arprot,
      M00_AXI_0_arqos   => M00_AXI_0_arqos,
      M00_AXI_0_arready => M00_AXI_0_arready,
      M00_AXI_0_arsize  => M00_AXI_0_arsize,
      M00_AXI_0_arvalid => M00_AXI_0_arvalid,

      M00_AXI_0_awaddr  => M00_AXI_0_awaddr,
      M00_AXI_0_awburst => M00_AXI_0_awburst,
      M00_AXI_0_awcache => M00_AXI_0_awcache,
      M00_AXI_0_awlen   => M00_AXI_0_awlen,
      M00_AXI_0_awlock  => M00_AXI_0_awlock,
      M00_AXI_0_awprot  => M00_AXI_0_awprot,
      M00_AXI_0_awqos   => M00_AXI_0_awqos,
      M00_AXI_0_awready => M00_AXI_0_awready,
      M00_AXI_0_awsize  => M00_AXI_0_awsize,
      M00_AXI_0_awvalid => M00_AXI_0_awvalid,

      M00_AXI_0_bready  => M00_AXI_0_bready,
      M00_AXI_0_bresp   => M00_AXI_0_bresp,
      M00_AXI_0_bvalid  => M00_AXI_0_bvalid,

      M00_AXI_0_rdata   => M00_AXI_0_rdata,
      M00_AXI_0_rlast   => M00_AXI_0_rlast,
      M00_AXI_0_rready  => M00_AXI_0_rready,
      M00_AXI_0_rresp   => M00_AXI_0_rresp,
      M00_AXI_0_rvalid  => M00_AXI_0_rvalid,

      M00_AXI_0_wdata   => M00_AXI_0_wdata,
      M00_AXI_0_wlast   => M00_AXI_0_wlast,
      M00_AXI_0_wready  => M00_AXI_0_wready,
      M00_AXI_0_wstrb   => M00_AXI_0_wstrb,
      M00_AXI_0_wvalid  => M00_AXI_0_wvalid,

      S00_AXI_0_araddr  => S00_AXI_0_araddr,
      S00_AXI_0_arprot  => S00_AXI_0_arprot,
      S00_AXI_0_arready => S00_AXI_0_arready,
      S00_AXI_0_arvalid => S00_AXI_0_arvalid,
      S00_AXI_0_awaddr  => S00_AXI_0_awaddr,
      S00_AXI_0_awprot  => S00_AXI_0_awprot,
      S00_AXI_0_awready => S00_AXI_0_awready,
      S00_AXI_0_awvalid => S00_AXI_0_awvalid,
      S00_AXI_0_bready  => S00_AXI_0_bready,
      S00_AXI_0_bresp   => S00_AXI_0_bresp,
      S00_AXI_0_bvalid  => S00_AXI_0_bvalid,
      S00_AXI_0_rdata   => S00_AXI_0_rdata,
      S00_AXI_0_rready  => S00_AXI_0_rready,
      S00_AXI_0_rresp   => S00_AXI_0_rresp,
      S00_AXI_0_rvalid  => S00_AXI_0_rvalid,
      S00_AXI_0_wdata   => S00_AXI_0_wdata,
      S00_AXI_0_wready  => S00_AXI_0_wready,
      S00_AXI_0_wstrb   => S00_AXI_0_wstrb,
      S00_AXI_0_wvalid  => S00_AXI_0_wvalid,

      S00_AXI_1_araddr  => S00_AXI_1_araddr,
      S00_AXI_1_arburst => S00_AXI_1_arburst,
      S00_AXI_1_arcache => S00_AXI_1_arcache,
      S00_AXI_1_arid    => S00_AXI_1_arid,
      S00_AXI_1_arlen   => S00_AXI_1_arlen,
      S00_AXI_1_arlock  => S00_AXI_1_arlock,
      S00_AXI_1_arprot  => S00_AXI_1_arprot,
      S00_AXI_1_arqos   => S00_AXI_1_arqos,
      S00_AXI_1_arready => S00_AXI_1_arready,
      S00_AXI_1_arsize  => S00_AXI_1_arsize,
      S00_AXI_1_aruser  => S00_AXI_1_aruser,
      S00_AXI_1_arvalid => S00_AXI_1_arvalid,

      S00_AXI_1_awaddr  => S00_AXI_1_awaddr,
      S00_AXI_1_awburst => S00_AXI_1_awburst,
      S00_AXI_1_awcache => S00_AXI_1_awcache,
      S00_AXI_1_awid    => S00_AXI_1_awid,
      S00_AXI_1_awlen   => S00_AXI_1_awlen,
      S00_AXI_1_awlock  => S00_AXI_1_awlock,
      S00_AXI_1_awprot  => S00_AXI_1_awprot,
      S00_AXI_1_awqos   => S00_AXI_1_awqos,
      S00_AXI_1_awready => S00_AXI_1_awready,
      S00_AXI_1_awsize  => S00_AXI_1_awsize,
      S00_AXI_1_awuser  => S00_AXI_1_awuser,
      S00_AXI_1_awvalid => S00_AXI_1_awvalid,

      S00_AXI_1_bid     => S00_AXI_1_bid,
      S00_AXI_1_bready  => S00_AXI_1_bready,
      S00_AXI_1_bresp   => S00_AXI_1_bresp,
      S00_AXI_1_buser   => S00_AXI_1_buser,
      S00_AXI_1_bvalid  => S00_AXI_1_bvalid,

      S00_AXI_1_rdata   => S00_AXI_1_rdata,
      S00_AXI_1_rid     => S00_AXI_1_rid,
      S00_AXI_1_rlast   => S00_AXI_1_rlast,
      S00_AXI_1_rready  => S00_AXI_1_rready,
      S00_AXI_1_rresp   => S00_AXI_1_rresp,
      S00_AXI_1_rvalid  => S00_AXI_1_rvalid,

      S00_AXI_1_wdata   => S00_AXI_1_wdata,
      S00_AXI_1_wlast   => S00_AXI_1_wlast,
      S00_AXI_1_wready  => S00_AXI_1_wready,
      S00_AXI_1_wstrb   => S00_AXI_1_wstrb,
      S00_AXI_1_wvalid  => S00_AXI_1_wvalid,

      interrupt         => interrupt,
      s_axi_aclk        => s_axi_aclk,
      s_axi_aresetn     => s_axi_aresetn
    );

end Behavioral;