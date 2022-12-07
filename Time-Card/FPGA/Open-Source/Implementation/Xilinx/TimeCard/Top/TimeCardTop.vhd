--*****************************************************************************************
-- Project: Time Card
--
-- Author: Sven Meier, NetTimeLogic GmbH
--
-- License: Copyright (c) 2022, NetTimeLogic GmbH, Switzerland, <contact@nettimelogic.com>
-- All rights reserved.
--
-- THIS PROGRAM IS FREE SOFTWARE: YOU CAN REDISTRIBUTE IT AND/OR MODIFY
-- IT UNDER THE TERMS OF THE GNU LESSER GENERAL PUBLIC LICENSE AS
-- PUBLISHED BY THE FREE SOFTWARE FOUNDATION, VERSION 3.
--
-- THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
-- WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED WARRANTY OF
-- MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. SEE THE GNU
-- LESSER GENERAL LESSER PUBLIC LICENSE FOR MORE DETAILS.
--
-- YOU SHOULD HAVE RECEIVED A COPY OF THE GNU LESSER GENERAL PUBLIC LICENSE
-- ALONG WITH THIS PROGRAM. IF NOT, SEE <http://www.gnu.org/licenses/>.
--
--*****************************************************************************************


--*****************************************************************************************
-- General Libraries
--*****************************************************************************************
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library TimeCardLib;
use TimeCardlib.TimeCard_Package.all;

library xil_defaultlib;
use xil_defaultlib.all;

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
entity TimeCardTop is
    generic (
        GoldenImage_Gen             :       boolean := false
    );
    port (
        -- System 
        Mhz10ClkDcxo1_ClkIn         : in    std_logic;
            
        RstN_RstIn                  : in    std_logic;
        
        -- MAC RF Out Clock
        Mhz10Clk0_ClkIn             : in    std_logic;
        Mhz10Clk1_ClkIn             : in    std_logic;
        
        --GTP Clock
        Mhz125ClkP_ClkIn            : in    std_logic;
        Mhz125ClkN_ClkIn            : in    std_logic;
        
        --200MHz Clock
        Mhz200ClkP_ClkIn            : in    std_logic;
        Mhz200ClkN_ClkIn            : in    std_logic; 
        
        Led_DatOut                  : out   std_logic_vector(3 downto 0);
        Key_DatIn                   : in    std_logic_vector(1 downto 0);
   
        EepromWp_DatOut             : out   std_logic;
   
        -----------------------------------------------------------------
        -- QSPI Flash Inputs/Outputs
        -----------------------------------------------------------------
        SpiFlashDq0_DatInOut        : inout std_logic;
        SpiFlashDq1_DatInOut        : inout std_logic;
        SpiFlashDq2_DatInOut        : inout std_logic;
        SpiFlashDq3_DatInOut        : inout std_logic;
        SpiFlashCsN_EnaOut          : out   std_logic;
        
        -----------------------------------------------------------------
        -- SMA Inputs/Outputs
        -----------------------------------------------------------------
        SmaIn1_DatIn                : in    std_logic;      -- ANT1
        SmaIn2_DatIn                : in    std_logic;      -- ANT2
        SmaIn3_DatIn                : in    std_logic;      -- ANT3
        SmaIn4_DatIn                : in    std_logic;      -- ANT4
        
        SmaOut1_DatOut              : out   std_logic;      -- ANT1
        SmaOut2_DatOut              : out   std_logic;      -- ANT2
        SmaOut3_DatOut              : out   std_logic;      -- ANT3
        SmaOut4_DatOut              : out   std_logic;      -- ANT4
        
        Sma1InBufEnableN_EnOut      : out   std_logic;
        Sma2InBufEnableN_EnOut      : out   std_logic;
        Sma3InBufEnableN_EnOut      : out   std_logic;
        Sma4InBufEnableN_EnOut      : out   std_logic;
        
        Sma1OutBufEnableN_EnOut     : out   std_logic;
        Sma2OutBufEnableN_EnOut     : out   std_logic;
        Sma3OutBufEnableN_EnOut     : out   std_logic;
        Sma4OutBufEnableN_EnOut     : out   std_logic;
        
        -----------------------------------------------------------------
        -- I2C
        -----------------------------------------------------------------
        I2cScl_ClkInOut             : inout std_logic;
        I2cSda_DatInOut             : inout std_logic;
        
        -----------------------------------------------------------------
        -- UART1
        -----------------------------------------------------------------
        Uart1TxDat_DatOut           : out   std_logic;
        Uart1RxDat_DatIn            : in    std_logic;
        
        -----------------------------------------------------------------
        -- GNSS
        -----------------------------------------------------------------
        UartGnss1TxDat_DatOut       : out   std_logic;
        UartGnss1RxDat_DatIn        : in    std_logic;
        Gnss1Tp_DatIn               : in    std_logic_vector(1 downto 0);
        Gnss1RstN_RstOut            : out   std_logic;
        
        UartGnss2TxDat_DatOut       : out   std_logic;
        UartGnss2RxDat_DatIn        : in    std_logic;
        Gnss2Tp_DatIn               : in    std_logic_vector(1 downto 0);
        Gnss2RstN_RstOut            : out   std_logic;
        
        -----------------------------------------------------------------
        --MAC
        -----------------------------------------------------------------
        MacTxDat_DatInOut           : inout std_logic;
        MacRxDat_DatInOut           : inout std_logic;
        
        MacFreqControl_DatOut       : out   std_logic;
        MacAlarm_DatIn              : in    std_logic;
        MacBite_DatIn               : in    std_logic;
        
        MacUsbPower_EnOut           : out   std_logic;
        MacUsbP_DatOut              : out   std_logic;
        MacUsbN_DatOut              : out   std_logic;
        
        MacPps_EvtIn                : in    std_logic;
        MacPps0_EvtOut              : out   std_logic;
        MacPps1_EvtOut              : out   std_logic;

        -----------------------------------------------------------------
        --PCIe
        -----------------------------------------------------------------
        PciePerst_RstIn             : in    std_logic;
        PcieRefClkP_ClkIn           : in    std_logic;
        PcieRefClkN_ClkIn           : in    std_logic;
        pcie_7x_mgt_0_rxn           : in    std_logic_vector(0 downto 0);
        pcie_7x_mgt_0_rxp           : in    std_logic_vector(0 downto 0);
        pcie_7x_mgt_0_txn           : out   std_logic_vector(0 downto 0);
        pcie_7x_mgt_0_txp           : out   std_logic_vector(0 downto 0)
    );
end TimeCardTop;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture TimeCardTop_Arch of TimeCardTop is
    --*************************************************************************************
    -- Component Definitions
    --*************************************************************************************
    component Bufio
        port (
            i                           : in    std_logic; 
            o                           : out   std_logic
        );
    end component Bufio;    
    
    
    component Bufg
        port (
            i                           : in    std_logic; 
            o                           : out   std_logic
        );
    end component Bufg;    
        
    component Bufr is
        port (
            o                           : out std_logic;
            ce                          : in std_logic;
            clr                         : in std_logic;
            i                           : in std_logic
        );
    end component Bufr;
    
    component Ibufds is
        port (
            i                           : in    std_logic;
            ib                          : in    std_logic;
            o                           : out   std_logic
        );
    end component Ibufds;
    
    component Obufds is
        port (
            o                           : out   std_logic;
            ob                          : out   std_logic;
            i                           : in    std_logic
        );
    end component Obufds;
    
    component TimeCard_wrapper is
        port (
            Clk_RxSdaT_EnaOut : out STD_LOGIC;
            Clk_RxSda_DatIn : in STD_LOGIC;
            Clk_RxSda_DatOut : out STD_LOGIC;
            Clk_TxSclT_EnaOut : out STD_LOGIC;
            Clk_TxScl_DatIn : in STD_LOGIC;
            Clk_TxScl_DatOut : out STD_LOGIC;
            Ext_DatIn_tri_i : in STD_LOGIC_VECTOR ( 1 downto 0 );
            Ext_DatOut : out STD_LOGIC_VECTOR ( 6 downto 0 );
            GoldenImageN_EnaIn : in STD_LOGIC;
            GpioGnss_DatOut_tri_o : out STD_LOGIC_VECTOR ( 1 downto 0 );
            GpioMac_DatIn_tri_i : in STD_LOGIC_VECTOR ( 1 downto 0 );
            I2c_scl_io : inout STD_LOGIC;
            I2c_sda_io : inout STD_LOGIC;
            InHoldover_DatOut : out STD_LOGIC;
            InSync_DatOut : out STD_LOGIC;
            MacPps0_EvtOut : out STD_LOGIC;
            MacPps1_EvtOut : out STD_LOGIC;
            MacPps_EvtIn : in STD_LOGIC;
            Mhz10ClkDcxo1_ClkIn : in STD_LOGIC;
            Mhz10ClkDcxo2_ClkIn : in STD_LOGIC;
            Mhz10ClkMac_ClkIn : in STD_LOGIC;
            Mhz10ClkSma_ClkIn : in STD_LOGIC;
            Mhz200Clk_ClkIn_clk_n : in STD_LOGIC;
            Mhz200Clk_ClkIn_clk_p : in STD_LOGIC;
            Mhz50Clk_ClkOut : out STD_LOGIC;
            Mhz50Clk_ClkOut_0 : out STD_LOGIC;
            Mhz62_5Clk_ClkOut : out STD_LOGIC;
            PciePerstN_RstIn : in STD_LOGIC;
            PcieRefClockN : in STD_LOGIC_VECTOR ( 0 to 0 );
            PcieRefClockP : in STD_LOGIC_VECTOR ( 0 to 0 );
            PpsGnss1_EvtIn : in STD_LOGIC;
            PpsGnss2_EvtIn : in STD_LOGIC;
            Pps_EvtOut : out STD_LOGIC;
            Reset50MhzN_RstOut : out STD_LOGIC_VECTOR ( 0 to 0 );
            Reset50MhzN_RstOut_0 : out STD_LOGIC_VECTOR ( 0 to 0 );
            Reset62_5MhzN_RstOut : out STD_LOGIC_VECTOR ( 0 to 0 );
            ResetN_RstIn : in STD_LOGIC;
            SmaIn1_DatIn : in STD_LOGIC;
            SmaIn1_EnOut : out STD_LOGIC;
            SmaIn2_DatIn : in STD_LOGIC;
            SmaIn2_EnOut : out STD_LOGIC;
            SmaIn3_DatIn : in STD_LOGIC;
            SmaIn3_EnOut : out STD_LOGIC;
            SmaIn4_DatIn : in STD_LOGIC;
            SmaIn4_EnOut : out STD_LOGIC;
            SmaOut1_DatOut : out STD_LOGIC;
            SmaOut1_EnOut : out STD_LOGIC;
            SmaOut2_DatOut : out STD_LOGIC;
            SmaOut2_EnOut : out STD_LOGIC;
            SmaOut3_DatOut : out STD_LOGIC;
            SmaOut3_EnOut : out STD_LOGIC;
            SmaOut4_DatOut : out STD_LOGIC;
            SmaOut4_EnOut : out STD_LOGIC;
            SpiFlash_io0_io : inout STD_LOGIC;
            SpiFlash_io1_io : inout STD_LOGIC;
            SpiFlash_io2_io : inout STD_LOGIC;
            SpiFlash_io3_io : inout STD_LOGIC;
            SpiFlash_ss_io : inout STD_LOGIC_VECTOR ( 0 to 0 );
            StartUpIo_cfgclk : out STD_LOGIC;
            StartUpIo_cfgmclk : out STD_LOGIC;
            StartUpIo_preq : out STD_LOGIC;
            UartGnss1Rx_DatIn : in STD_LOGIC;
            UartGnss1Tx_DatOut : out STD_LOGIC;
            UartGnss2Rx_DatIn : in STD_LOGIC;
            UartGnss2Tx_DatOut : out STD_LOGIC;
            pcie_7x_mgt_0_rxn : in STD_LOGIC_VECTOR ( 0 to 0 );
            pcie_7x_mgt_0_rxp : in STD_LOGIC_VECTOR ( 0 to 0 );
            pcie_7x_mgt_0_txn : out STD_LOGIC_VECTOR ( 0 to 0 );
            pcie_7x_mgt_0_txp : out STD_LOGIC_VECTOR ( 0 to 0 )
        );
    end component TimeCard_wrapper;
    
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant ClkPeriodNanosecond_Con    :       natural := 20;   
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    
    -- Rst & Clk
    signal PciePerstN_Rst               : std_logic;
    
    signal Mhz10Clk0_Clk                : std_logic;
 
    signal Mhz10ClkDcxo1_Clk            : std_logic;
    signal Mhz10ClkDcxo2_Clk            : std_logic;
    
    signal Mhz50Clk_Clk                 : std_logic;
    signal Mhz50RstN_Rst                : std_logic;
    
    signal Mhz50Clk_Clk_0               : std_logic;
    signal Mhz50RstN_Rst_0              : std_logic;
    
    signal Mhz62_5Clk_Clk               : std_logic;
    signal Mhz62_5RstN_Rst              : std_logic;
    
    signal RstCount_CntReg              : natural := 0;
    
    -- Led
    signal BlinkingLed_DatReg           : std_logic;
    signal BlinkingLedCount_CntReg      : natural;
    
    signal BlinkingLed2_DatReg          : std_logic;
    signal BlinkingLed2Count_CntReg     : natural;
    
    signal GnssDataOe_EnaReg            : std_logic;
    
    signal Ext_DatIn                    : std_logic_vector(1 downto 0);
    signal Ext_DatOut                   : std_logic_vector(6 downto 0);
    
    signal Pps_EvtOut                   : std_logic;

    signal MacPps0_Evt                  : std_logic;
    signal MacPps1_Evt                  : std_logic;
    signal GpioGnss_DatOut              : std_logic_vector(1 downto 0);
    
    signal UartGnss1TxDat_Dat           : std_logic;
    signal UartGnss2TxDat_Dat           : std_logic;
        
    signal StartUpIo_cfgclk             : std_logic;
    signal StartUpIo_cfgmclk            : std_logic;
    signal StartUpIo_preq               : std_logic;
    
    -- SMA Connector / Buffers
    signal SmaIn1_En                    : std_logic;
    signal SmaIn2_En                    : std_logic;
    signal SmaIn3_En                    : std_logic;
    signal SmaIn4_En                    : std_logic;
        
    signal SmaOut1_En                   : std_logic;
    signal SmaOut2_En                   : std_logic;
    signal SmaOut3_En                   : std_logic;
    signal SmaOut4_En                   : std_logic;
    
    signal SpiFlashCsN_Ena              : std_logic;
    
    signal GoldenImageN_Ena             : std_logic;
    
    signal Clk_RxSda_i                  : std_logic;
    signal Clk_RxSda_o                  : std_logic;
    signal Clk_RxSda_t                  : std_logic;
    signal Clk_TxScl_i                  : std_logic;
    signal Clk_TxScl_o                  : std_logic;
    signal Clk_TxScl_t                  : std_logic;
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin
    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    
    -- CLK UART and CLK I2C share the same pins (it is configurable which interface is active)
    Clk_RxSda_i <= MacRxDat_DatInOut;
    MacRxDat_DatInOut <= Clk_RxSda_o when (Clk_RxSda_t = '0') else 'Z';
    
    Clk_TxScl_i <= MacTxDat_DatInOut;
    MacTxDat_DatInOut <= Clk_TxScl_o when (Clk_TxScl_t = '0') else 'Z';
    
    -- SMA
    Sma1InBufEnableN_EnOut <= not SmaIn1_En;
    Sma2InBufEnableN_EnOut <= not SmaIn2_En;
    Sma3InBufEnableN_EnOut <= not SmaIn3_En;
    Sma4InBufEnableN_EnOut <= not SmaIn4_En;
    
    Sma1OutBufEnableN_EnOut <= not SmaOut1_En;
    Sma2OutBufEnableN_EnOut <= not SmaOut2_En;
    Sma3OutBufEnableN_EnOut <= not SmaOut3_En;
    Sma4OutBufEnableN_EnOut <= not SmaOut4_En;
    
    
    GoldenImageN_Ena <= '0' when GoldenImage_Gen = true else '1';
    
    PciePerstN_Rst <= PciePerst_RstIn;
    
    Ext_DatIn <= Key_DatIn;
    
    Led_DatOut(3) <= MacPps_EvtIn;          -- Ext_DatOut(3);
    Led_DatOut(2) <= Pps_EvtOut;            -- Ext_DatOut(2);
    Led_DatOut(1) <= BlinkingLed2_DatReg;   -- Ext_DatOut(1);
    Led_DatOut(0) <= BlinkingLed_DatReg;    -- Ext_DatOut(0);
    
    EepromWp_DatOut <= Ext_DatOut(4);
    
    -- GNSS Outputs
    Gnss1RstN_RstOut <= not GpioGnss_DatOut(0);
    Gnss2RstN_RstOut <= not GpioGnss_DatOut(1);
    
    -- Wait 1s until enable Gnss Uart Tx Output
    UartGnss1TxDat_DatOut <= 'Z' when GnssDataOe_EnaReg = '0' else UartGnss1TxDat_Dat;
    UartGnss2TxDat_DatOut <= 'Z' when GnssDataOe_EnaReg = '0' else UartGnss2TxDat_Dat;
    
    -- SPI Flash
    SpiFlashCsN_EnaOut <= SpiFlashCsN_Ena;
    
    -- MAC
    MacFreqControl_DatOut <= '0';
    MacUsbPower_EnOut <= '0';
    
    --! unused uart
    Uart1TxDat_DatOut <= '0'; --! unused

    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    BlinkingLed_Prc : process(Mhz50RstN_Rst, Mhz50Clk_Clk) is
    begin
        if (Mhz50RstN_Rst = '0') then
            BlinkingLed_DatReg <= '0';
            BlinkingLedCount_CntReg <= 0;
        elsif ((Mhz50Clk_Clk'event) and (Mhz50Clk_Clk = '1')) then
            if (BlinkingLedCount_CntReg < 250000000) then
                BlinkingLedCount_CntReg <= BlinkingLedCount_CntReg + ClkPeriodNanosecond_Con;
            else
                BlinkingLed_DatReg <= (not BlinkingLed_DatReg);
                BlinkingLedCount_CntReg <= 0;
            end if;
        end if;
    end process BlinkingLed_Prc;
    
    BlinkingLed2_Prc : process(Mhz62_5RstN_Rst, Mhz62_5Clk_Clk) is
    begin
        if (Mhz62_5RstN_Rst = '0') then
            BlinkingLed2_DatReg <= '0';
            BlinkingLed2Count_CntReg <= 0;
        elsif ((Mhz62_5Clk_Clk'event) and (Mhz62_5Clk_Clk = '1')) then
            if (BlinkingLed2Count_CntReg < 200000000) then
                BlinkingLed2Count_CntReg <= BlinkingLed2Count_CntReg + ClkPeriodNanosecond_Con;
            else
                BlinkingLed2_DatReg <= (not BlinkingLed2_DatReg);
                BlinkingLed2Count_CntReg <= 0;
            end if;
        end if;
    end process BlinkingLed2_Prc;
    
    Rst_Prc : process(Mhz50RstN_Rst, Mhz50Clk_Clk) is
    begin
        if (Mhz50RstN_Rst = '0') then
            GnssDataOe_EnaReg <= '0';
            RstCount_CntReg <= 0;
        elsif ((Mhz50Clk_Clk'event) and (Mhz50Clk_Clk = '1')) then
            if (RstCount_CntReg < 2000000000) then
                RstCount_CntReg <= RstCount_CntReg + ClkPeriodNanosecond_Con;
                if (RstCount_CntReg < 1000000000) then -- 1000ms
                    GnssDataOe_EnaReg <= '0';
                else
                    GnssDataOe_EnaReg <= '1';
                end if;
            else
                RstCount_CntReg <= RstCount_CntReg;
                GnssDataOe_EnaReg <= '1';
            end if;
        end if;
    end process Rst_Prc;
    
    --*************************************************************************************
    -- Instantiation and Port mapping
    --*************************************************************************************
    
    MacPps0_EvtOut <= MacPps0_Evt;
    MacPps1_EvtOut <= MacPps1_Evt;
    
    -- MacUsb_Inst: component Obufds 
    -- port map(
        -- o                           => MacUsbP_DatOut,
        -- ob                          => MacUsbN_DatOut,
        -- i                           => '0'
    -- );
    MacUsbP_DatOut <= '0';
    MacUsbN_DatOut <= '0';
       
    
    BufrClk0_Inst : Bufr   
    port map (  
        ce                              => '1',
        clr                             => '0',
        i                               => Mhz10Clk0_ClkIn,
        o                               => Mhz10Clk0_Clk
    );  
    
    BufrDcxo1_Inst : Bufr   
    port map (
        ce                              => '1',
        clr                             => '0',
        i                               => Mhz10ClkDcxo1_ClkIn,
        o                               => Mhz10ClkDcxo1_Clk
    );   
   
    -- BufrDcxo2_Inst : Bufr   
    -- port map (
        -- ce                              => '1',
        -- clr                             => '0',
        -- i                               => Mhz10ClkDcxo2_ClkIn,
        -- o                               => Mhz10ClkDcxo2_Clk
    -- );      

    Bd_Inst: component TimeCard_wrapper
    port map (
        Mhz200Clk_ClkIn_clk_n       => Mhz200ClkN_ClkIn,
        Mhz200Clk_ClkIn_clk_p       => Mhz200ClkP_ClkIn,
        
        Mhz10ClkMac_ClkIn           => Mhz10Clk0_Clk,
        Mhz10ClkSma_ClkIn           => SmaIn1_DatIn,
        Mhz10ClkDcxo1_ClkIn         => Mhz10ClkDcxo1_Clk,
        Mhz10ClkDcxo2_ClkIn         => '0',
        
        ResetN_RstIn                => RstN_RstIn,
        GoldenImageN_EnaIn          => GoldenImageN_Ena,
       
        -- Internal 50MHz (does no change on clock source switch)
        Mhz50Clk_ClkOut_0           => Mhz50Clk_Clk_0,
        Reset50MhzN_RstOut_0(0)     => Mhz50RstN_Rst_0,    
        
        Mhz50Clk_ClkOut             => Mhz50Clk_Clk,
        Reset50MhzN_RstOut(0)       => Mhz50RstN_Rst,       

        Mhz62_5Clk_ClkOut           => Mhz62_5Clk_Clk,
        Reset62_5MhzN_RstOut(0)     => Mhz62_5RstN_Rst,

      
        InHoldover_DatOut           => open,
        InSync_DatOut               => open,
        
        Ext_DatIn_tri_i             => Ext_DatIn,
        Ext_DatOut                  => Ext_DatOut,
        
        I2c_scl_io                  => I2cScl_ClkInOut,
        I2c_sda_io                  => I2cSda_DatInOut,
        
        SpiFlash_io0_io             => SpiFlashDq0_DatInOut,
        SpiFlash_io1_io             => SpiFlashDq1_DatInOut,
        SpiFlash_io2_io             => SpiFlashDq2_DatInOut,
        SpiFlash_io3_io             => SpiFlashDq3_DatInOut,
        SpiFlash_ss_io(0)           => SpiFlashCsN_Ena,
        
        StartUpIo_cfgclk            => StartUpIo_cfgclk,
        StartUpIo_cfgmclk           => StartUpIo_cfgmclk,
        StartUpIo_preq              => StartUpIo_preq,
        
        GpioGnss_DatOut_tri_o       => GpioGnss_DatOut,

        GpioMac_DatIn_tri_i(0)      => MacAlarm_DatIn,
        GpioMac_DatIn_tri_i(1)      => MacBite_DatIn,
        
        SmaIn1_DatIn                => SmaIn1_DatIn,
        SmaIn1_EnOut                => SmaIn1_En,
        SmaIn2_DatIn                => SmaIn2_DatIn,
        SmaIn2_EnOut                => SmaIn2_En,
        SmaIn3_DatIn                => SmaIn3_DatIn,
        SmaIn3_EnOut                => SmaIn3_En,
        SmaIn4_DatIn                => SmaIn4_DatIn,
        SmaIn4_EnOut                => SmaIn4_En,
        
        SmaOut1_DatOut              => SmaOut1_DatOut,
        SmaOut1_EnOut               => SmaOut1_En,
        SmaOut2_DatOut              => SmaOut2_DatOut,
        SmaOut2_EnOut               => SmaOut2_En,
        SmaOut3_DatOut              => SmaOut3_DatOut,
        SmaOut3_EnOut               => SmaOut3_En,
        SmaOut4_DatOut              => SmaOut4_DatOut,
        SmaOut4_EnOut               => SmaOut4_En,

        PpsGnss1_EvtIn              => Gnss1Tp_DatIn(0),
        PpsGnss2_EvtIn              => Gnss2Tp_DatIn(0),
        Pps_EvtOut                  => Pps_EvtOut,
        
        MacPps_EvtIn                => MacPps_EvtIn,
        MacPps0_EvtOut              => MacPps0_Evt,
        MacPps1_EvtOut              => MacPps1_Evt,

        UartGnss1Rx_DatIn           => UartGnss1RxDat_DatIn,
        UartGnss1Tx_DatOut          => UartGnss1TxDat_Dat,
        
        UartGnss2Rx_DatIn           => UartGnss2RxDat_DatIn,
        UartGnss2Tx_DatOut          => UartGnss2TxDat_Dat,
        
        Clk_RxSda_DatIn             => Clk_RxSda_i,
        Clk_RxSda_DatOut            => Clk_RxSda_o,
        Clk_RxSdaT_EnaOut           => Clk_RxSda_t,
        Clk_TxSclT_EnaOut           => Clk_TxScl_t,
        Clk_TxScl_DatIn             => Clk_TxScl_i,
        Clk_TxScl_DatOut            => Clk_TxScl_o,
        
        PcieRefClockN(0)            => PcieRefClkN_ClkIn,
        PcieRefClockP(0)            => PcieRefClkP_ClkIn,
        PciePerstN_RstIn            => PciePerstN_Rst,
        pcie_7x_mgt_0_rxn           => pcie_7x_mgt_0_rxn,
        pcie_7x_mgt_0_rxp           => pcie_7x_mgt_0_rxp,
        pcie_7x_mgt_0_txn           => pcie_7x_mgt_0_txn,
        pcie_7x_mgt_0_txp           => pcie_7x_mgt_0_txp
    );

end TimeCardTop_Arch;
