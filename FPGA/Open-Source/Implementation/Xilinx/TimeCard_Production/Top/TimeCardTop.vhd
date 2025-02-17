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
        
        -- Board Revision
        BoardRev0_DatIn             : in    std_logic;
        BoardRev1_DatIn             : in    std_logic;
        BoardRev2_DatIn             : in    std_logic;
        
        -- LEDs
        Led_DatOut                  : out   std_logic_vector(3 downto 0);
        FpgaLed_DatOut              : out   std_logic_vector(1 downto 0);
        
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
        
        Sma1LedBlueN_DatOut         : out   std_logic;
        Sma1LedGreenN_DatOut        : out   std_logic;
        Sma1LedRedN_DatOut          : out   std_logic;
            
        Sma2LedBlueN_DatOut         : out   std_logic;
        Sma2LedGreenN_DatOut        : out   std_logic;
        Sma2LedRedN_DatOut          : out   std_logic;
        
        Sma3LedBlueN_DatOut         : out   std_logic;
        Sma3LedGreenN_DatOut        : out   std_logic;
        Sma3LedRedN_DatOut          : out   std_logic;
            
        Sma4LedBlueN_DatOut         : out   std_logic;
        Sma4LedGreenN_DatOut        : out   std_logic;
        Sma4LedRedN_DatOut          : out   std_logic;
        
        
        -----------------------------------------------------------------
        --EEPROM I2C
        -----------------------------------------------------------------
        EepromI2cScl_ClkInOut       : inout std_logic;
        EepromI2cSda_DatInOut       : inout std_logic;
        EepromWp_DatOut             : out   std_logic;
        
        -----------------------------------------------------------------
        --PCA9546 I2C
        -----------------------------------------------------------------
        Pca9546I2cScl_ClkInOut      : inout std_logic;
        Pca9546I2cSda_DatInOut      : inout std_logic;
        Pca9546RstN_RstOut          : out   std_logic;
   
        -----------------------------------------------------------------
        --RGB I2C
        -----------------------------------------------------------------
        RgbI2cScl_ClkInOut          : inout std_logic;
        RgbI2cSda_DatInOut          : inout std_logic;
        RgbShutDownN_EnOut          : out   std_logic;
        
        -----------------------------------------------------------------
        --SHT3X
        -----------------------------------------------------------------
        Sht3xAlertN_DatIn           : in    std_logic;
        Sht3xRstN_RstOut            : out   std_logic;
        
        -----------------------------------------------------------------
        --LM75B
        -----------------------------------------------------------------
        Lm75BInt1N_EvtIn            : in    std_logic;
        Lm75BInt2N_EvtIn            : in    std_logic;
        Lm75BInt3N_EvtIn            : in    std_logic;
        
        -----------------------------------------------------------------
        --BNO
        -----------------------------------------------------------------
        BnoRstN_RstOut              : out   std_logic;
        BnoIntN_EvtIn               : in    std_logic;
        BnoBootN_DatOut             : out   std_logic; 
        
        -----------------------------------------------------------------
        --SMBUS
        -----------------------------------------------------------------
        SmI2CBufEn_EnOut            : out   std_logic;
        
        -----------------------------------------------------------------
        --USB Mux
        -----------------------------------------------------------------
        DebugUsbMuxSel_DatOut       : out   std_logic;

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
        
        Gnss1LedBlueN_DatOut        : out   std_logic;
        Gnss1LedGreenN_DatOut       : out   std_logic;
        Gnss1LedRedN_DatOut         : out   std_logic;
        
        -----------------------------------------------------------------
        --MAC
        -----------------------------------------------------------------
        MacTxDat_DatOut             : out   std_logic;
        MacRxDat_DatIn              : in    std_logic;
        
        MacFreqControl_DatOut       : out   std_logic;
        MacAlarm_DatIn              : in    std_logic;
        MacBite_DatIn               : in    std_logic;
        
        MacUsbPower_EnOut           : out   std_logic;
        MacUsbP_DatOut              : out   std_logic;
        MacUsbN_DatOut              : out   std_logic;
        
        MacUsbOcN_DatIn             : in    std_logic;
        
        MacPpsP_EvtIn               : in    std_logic;
        MacPpsN_EvtIn               : in    std_logic;
        
        MacPps0P_EvtOut             : out   std_logic;
        MacPps0N_EvtOut             : out   std_logic;

        MacPps1P_EvtOut             : out   std_logic;
        MacPps1N_EvtOut             : out   std_logic; 

        -----------------------------------------------------------------
        --PCIe
        -----------------------------------------------------------------
        PciePerstN_RstIn            : in    std_logic;
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
            Ext_DatIn_tri_i : in STD_LOGIC_VECTOR ( 31 downto 0 );
            Ext_DatOut_tri_o : out STD_LOGIC_VECTOR ( 31 downto 0 );
            GoldenImageN_EnaIn : in STD_LOGIC;
            GpioGnss_DatOut_tri_o : out STD_LOGIC_VECTOR ( 1 downto 0 );
            GpioMac_DatIn_tri_i : in STD_LOGIC_VECTOR ( 1 downto 0 );
            GpioRgb_DatOut_tri_o : out STD_LOGIC_VECTOR ( 31 downto 0 );
            I2c_eeprom_scl_io : inout STD_LOGIC;
            I2c_eeprom_sda_io : inout STD_LOGIC;
            I2c_rgb_scl_io : inout STD_LOGIC;
            I2c_rgb_sda_io : inout STD_LOGIC;
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
            UartMacRx_DatIn : in STD_LOGIC;
            UartMacTx_DatOut : out STD_LOGIC;
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
    signal Mhz10Clk0_Clk                : std_logic;

    signal Mhz50Clk_Clk                 : std_logic;
    signal Mhz50RstN_Rst                : std_logic;
    
    signal Mhz50Clk_Clk_0               : std_logic;
    signal Mhz50RstN_Rst_0              : std_logic;
    
    signal Mhz62_5Clk_Clk               : std_logic;
    signal Mhz62_5RstN_Rst              : std_logic;
    
    signal RstCount_CntReg              : natural := 0;
    
    signal BnoRstN_Rst                  : std_logic;
    
    -- Led
    signal BlinkingLed_DatReg           : std_logic;
    signal BlinkingLedCount_CntReg      : natural;
    
    signal BlinkingLed2_DatReg          : std_logic;
    signal BlinkingLed2Count_CntReg     : natural;
    
    signal GnssDataOe_EnaReg            : std_logic;
    
    signal Ext_DatIn                    : std_logic_vector(31 downto 0);
    signal Ext_DatOut                   : std_logic_vector(31 downto 0);
    
    signal Pps_EvtOut                   : std_logic;

    signal MacPps_EvtIn                 : std_logic;
    signal MacPps0_Evt                  : std_logic;
    signal MacPps1_Evt                  : std_logic;
    
    signal UartGnss1TxDat_Dat           : std_logic;
    signal GpioGnss_DatOut              : std_logic_vector(1 downto 0);
    signal GpioRgb_DatOut               : std_logic_vector(31 downto 0);
        
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
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin
    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    GoldenImageN_Ena <= '0' when GoldenImage_Gen = true else '1';
    
    -- Misc external Inputs
    Ext_DatIn(3 downto 0) <= (others => '0');
    
    Ext_DatIn(4) <= BoardRev0_DatIn;
    Ext_DatIn(5) <= BoardRev1_DatIn;
    Ext_DatIn(6) <= BoardRev2_DatIn;
    Ext_DatIn(7) <= '0';
    
    Ext_DatIn(8) <= not Sht3xAlertN_DatIn;
    
    Ext_DatIn(9) <= not Lm75BInt1N_EvtIn;
    Ext_DatIn(10) <= not Lm75BInt2N_EvtIn;
    Ext_DatIn(11) <= not Lm75BInt3N_EvtIn;
    
    Ext_DatIn(12) <= not BnoIntN_EvtIn;
    
    Ext_DatIn(31 downto 13) <= (others => '0');
    
    -- LED Outputs
    Led_DatOut(3) <= MacPps_EvtIn;          -- Ext_DatOut(3);
    Led_DatOut(2) <= Pps_EvtOut;            -- Ext_DatOut(2);
    Led_DatOut(1) <= BlinkingLed2_DatReg;   -- Ext_DatOut(1);
    Led_DatOut(0) <= BlinkingLed_DatReg;    -- Ext_DatOut(0);

    FpgaLed_DatOut(1) <= BlinkingLed2_DatReg;
    FpgaLed_DatOut(0) <= BlinkingLed_DatReg;

    Gnss1LedBlueN_DatOut <= not GpioRgb_DatOut(18);
    Gnss1LedGreenN_DatOut <= not GpioRgb_DatOut(17);
    Gnss1LedRedN_DatOut <= not GpioRgb_DatOut(16);
    
    -- Wait 1s until enable Gnss1 Uart Tx Output
    UartGnss1TxDat_DatOut <= 'Z' when GnssDataOe_EnaReg = '0' else UartGnss1TxDat_Dat;
    
    -- SMA
    Sma1InBufEnableN_EnOut <= not SmaIn1_En;
    Sma2InBufEnableN_EnOut <= not SmaIn2_En;
    Sma3InBufEnableN_EnOut <= not SmaIn3_En;
    Sma4InBufEnableN_EnOut <= not SmaIn4_En;
    
    Sma1OutBufEnableN_EnOut <= not SmaOut1_En;
    Sma2OutBufEnableN_EnOut <= not SmaOut2_En;
    Sma3OutBufEnableN_EnOut <= not SmaOut3_En;
    Sma4OutBufEnableN_EnOut <= not SmaOut4_En;
    
    Sma1LedBlueN_DatOut <= not GpioRgb_DatOut(2) when GoldenImage_Gen = false else '1';
    -- Special Version
    -- Sma1LedBlueN_DatOut <= BlinkingLed_DatReg when GoldenImage_Gen = false else '1';
    Sma1LedGreenN_DatOut <= not GpioRgb_DatOut(1) when GoldenImage_Gen = false else '1';
    Sma1LedRedN_DatOut <= not GpioRgb_DatOut(0) when GoldenImage_Gen = false else BlinkingLed_DatReg;

    Sma2LedBlueN_DatOut <= not GpioRgb_DatOut(6) when GoldenImage_Gen = false else '1';
    Sma2LedGreenN_DatOut <= not GpioRgb_DatOut(5) when GoldenImage_Gen = false else '1';
    Sma2LedRedN_DatOut <= not GpioRgb_DatOut(4) when GoldenImage_Gen = false else BlinkingLed_DatReg;

    Sma3LedBlueN_DatOut <= not GpioRgb_DatOut(10) when GoldenImage_Gen = false else '1';
    Sma3LedGreenN_DatOut <= not GpioRgb_DatOut(9) when GoldenImage_Gen = false else '1';
    Sma3LedRedN_DatOut <= not GpioRgb_DatOut(8) when GoldenImage_Gen = false else BlinkingLed_DatReg;

    Sma4LedBlueN_DatOut <= not GpioRgb_DatOut(14) when GoldenImage_Gen = false else '1';
    Sma4LedGreenN_DatOut <= not GpioRgb_DatOut(13) when GoldenImage_Gen = false else '1';
    Sma4LedRedN_DatOut <= not GpioRgb_DatOut(12) when GoldenImage_Gen = false else BlinkingLed_DatReg;
    
    -- GNSS Outputs
    Gnss1RstN_RstOut <= not GpioGnss_DatOut(0);

    
    -- SPI Flash
    SpiFlashCsN_EnaOut <= SpiFlashCsN_Ena;
    
    -- MAC
    MacFreqControl_DatOut <= '0';
    MacUsbPower_EnOut <= '0';
    
        -- Misc external Outputs
    -- EEPROM
    EepromWp_DatOut <= Ext_DatOut(4);
    
    --PCA9546 & I2C 
    Pca9546RstN_RstOut <= not Ext_DatOut(8);

    --RGB & I2C
    RgbShutDownN_EnOut <= not Ext_DatOut(9);

    --SHT3X
    Sht3xRstN_RstOut <= not Ext_DatOut(10);

    --BNO
    BnoRstN_RstOut <= BnoRstN_Rst and (not Ext_DatOut(11));
    BnoBootN_DatOut <= not Ext_DatOut(12);

    --SMBUS
    SmI2CBufEn_EnOut <= Ext_DatOut(13);
    
    --Debug
    DebugUsbMuxSel_DatOut <= Ext_DatOut(14);

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
            BnoRstN_Rst <= '1';
            GnssDataOe_EnaReg <= '0';
            RstCount_CntReg <= 0;
        elsif ((Mhz50Clk_Clk'event) and (Mhz50Clk_Clk = '1')) then
            if (RstCount_CntReg < 2000000000) then
                RstCount_CntReg <= RstCount_CntReg + ClkPeriodNanosecond_Con;
                if (RstCount_CntReg < 100000) then        -- 100 us
                    BnoRstN_Rst <= '0';  
                elsif (RstCount_CntReg < 1000000000) then -- 1000ms
                   BnoRstN_Rst <= '1';
                   GnssDataOe_EnaReg <= '0';
                else
                    BnoRstN_Rst <= '1';
                    GnssDataOe_EnaReg <= '1';
                end if;
            else
                RstCount_CntReg <= RstCount_CntReg;
                BnoRstN_Rst <= '1';
                GnssDataOe_EnaReg <= '1';
            end if;
        end if;
    end process Rst_Prc;
    
    --*************************************************************************************
    -- Instantiation and Port mapping
    --*************************************************************************************
    
    MacPpsIn_Inst: component Ibufds 
    port map(
            i                       => MacPpsP_EvtIn,
            ib                      => MacPpsN_EvtIn,
            o                       => MacPps_EvtIn
    );
    
    MacPps0_Inst: component Obufds 
    port map(
        o                           => MacPps0P_EvtOut,
        ob                          => MacPps0N_EvtOut,
        i                           => MacPps0_Evt
    );
    
    MacPps1_Inst: component Obufds 
    port map(
        o                           => MacPps1P_EvtOut,
        ob                          => MacPps1N_EvtOut,
        i                           => MacPps1_Evt
    );
    
    -- MacUsb_Inst: component Obufds 
    -- port map(
        -- o                           => MacUsbP_DatOut,
        -- ob                          => MacUsbN_DatOut,
        -- i                           => '1'
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

    Bd_Inst: component TimeCard_wrapper
    port map (
        Mhz200Clk_ClkIn_clk_n       => Mhz200ClkN_ClkIn,
        Mhz200Clk_ClkIn_clk_p       => Mhz200ClkP_ClkIn,
        
        Mhz10ClkMac_ClkIn           => Mhz10Clk0_Clk,
        Mhz10ClkSma_ClkIn           => SmaIn1_DatIn,

        Mhz10ClkDcxo1_ClkIn         => '0',
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
        Ext_DatOut_tri_o            => Ext_DatOut,
        
        I2c_eeprom_scl_io           => EepromI2cScl_ClkInOut,
        I2c_eeprom_sda_io           => EepromI2cSda_DatInOut,
        
        I2c_rgb_scl_io              => RgbI2cScl_ClkInOut,
        I2c_rgb_sda_io              => RgbI2cSda_DatInOut,
        
        I2c_scl_io                  => Pca9546I2cScl_ClkInOut,
        I2c_sda_io                  => Pca9546I2cSda_DatInOut,
        
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
        
        GpioRgb_DatOut_tri_o        => GpioRgb_DatOut,

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
        PpsGnss2_EvtIn              => '0',
        Pps_EvtOut                  => Pps_EvtOut,
        
        MacPps_EvtIn                => MacPps_EvtIn,
        MacPps0_EvtOut              => MacPps0_Evt,
        MacPps1_EvtOut              => MacPps1_Evt,

        UartGnss1Rx_DatIn           => UartGnss1RxDat_DatIn,
        UartGnss1Tx_DatOut          => UartGnss1TxDat_Dat,
        
        UartGnss2Rx_DatIn           => '0',
        UartGnss2Tx_DatOut          => open,
        
        UartMacRx_DatIn             => MacRxDat_DatIn,
        UartMacTx_DatOut            => MacTxDat_DatOut,
        
        PcieRefClockN(0)            => PcieRefClkN_ClkIn,
        PcieRefClockP(0)            => PcieRefClkP_ClkIn,
        PciePerstN_RstIn            => PciePerstN_RstIn,
        pcie_7x_mgt_0_rxn           => pcie_7x_mgt_0_rxn,
        pcie_7x_mgt_0_rxp           => pcie_7x_mgt_0_rxp,
        pcie_7x_mgt_0_txn           => pcie_7x_mgt_0_txn,
        pcie_7x_mgt_0_txp           => pcie_7x_mgt_0_txp
    );

end TimeCardTop_Arch;
