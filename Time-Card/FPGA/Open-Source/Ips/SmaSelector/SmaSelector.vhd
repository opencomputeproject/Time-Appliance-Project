--*****************************************************************************************
-- Project: Time Card
--
-- Author: Thomas Schaub, NetTimeLogic GmbH
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
use ieee.math_real.all;

--*****************************************************************************************
-- Specific Libraries
--*****************************************************************************************
library TimecardLib;
use TimecardLib.Timecard_Package.all;

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The SMA Selector multiplexes the output and demultiplexes the inputs of the 4 SMA     --
-- connectors of the Timecard. Each connector can be configured as input or output,      --
-- depending on the configured mapping. The configured mapping is done via 2 AXI4L slave --
-- interfaces. Each slave interface controls one mapping option.                         --
-------------------------------------------------------------------------------------------
entity SmaSelector is
    generic (
        SmaInput1SourceSelect_Gen                   :       std_logic_vector(15 downto 0) := x"8000"; 
        SmaInput2SourceSelect_Gen                   :       std_logic_vector(15 downto 0) := x"8001";
        SmaInput3SourceSelect_Gen                   :       std_logic_vector(15 downto 0) := x"0000";
        SmaInput4SourceSelect_Gen                   :       std_logic_vector(15 downto 0) := x"0000";
        SmaOutput1SourceSelect_Gen                  :       std_logic_vector(15 downto 0) := x"0000";
        SmaOutput2SourceSelect_Gen                  :       std_logic_vector(15 downto 0) := x"0000";
        SmaOutput3SourceSelect_Gen                  :       std_logic_vector(15 downto 0) := x"8000";
        SmaOutput4SourceSelect_Gen                  :       std_logic_vector(15 downto 0) := x"8001"
    );
    port (
        -- System
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
                    
        -- Sma Input Sources                
        Sma10MHzSourceEnable_EnOut                  : out   std_logic;
        SmaExtPpsSource1_EvtOut                     : out   std_logic;
        SmaExtPpsSource2_EvtOut                     : out   std_logic;
        SmaTs1Source_EvtOut                         : out   std_logic;
        SmaTs2Source_EvtOut                         : out   std_logic;
        SmaTs3Source_EvtOut                         : out   std_logic;
        SmaTs4Source_EvtOut                         : out   std_logic;
        SmaFreqCnt1Source_EvtOut                    : out   std_logic;
        SmaFreqCnt2Source_EvtOut                    : out   std_logic;
        SmaFreqCnt3Source_EvtOut                    : out   std_logic;
        SmaFreqCnt4Source_EvtOut                    : out   std_logic;
        SmaIrigSlaveSource_DatOut                   : out   std_logic;
        SmaDcfSlaveSource_DatOut                    : out   std_logic;
        SmaUartExtSource_DatOut                     : out   std_logic;
                    
        -- Sma Output Sources           
        Sma10MHzSource_ClkIn                        : in    std_logic;
        SmaFpgaPpsSource_EvtIn                      : in    std_logic;
        SmaMacPpsSource_EvtIn                       : in    std_logic;
        SmaGnss1PpsSource_EvtIn                     : in    std_logic;
        SmaGnss2PpsSource_EvtIn                     : in    std_logic;
        SmaIrigMasterSource_DatIn                   : in    std_logic;
        SmaDcfMasterSource_DatIn                    : in    std_logic;
        SmaSignalGen1Source_DatIn                   : in    std_logic;
        SmaSignalGen2Source_DatIn                   : in    std_logic;
        SmaSignalGen3Source_DatIn                   : in    std_logic;
        SmaSignalGen4Source_DatIn                   : in    std_logic;
        SmaUartGnss1Source_DatIn                    : in    std_logic;
        SmaUartGnss2Source_DatIn                    : in    std_logic;
        SmaUartExtSource_DatIn                      : in    std_logic;
                        
        -- Sma Input            
        SmaIn1_DatIn                                : in    std_logic;
        SmaIn2_DatIn                                : in    std_logic;
        SmaIn3_DatIn                                : in    std_logic;
        SmaIn4_DatIn                                : in    std_logic;
                    
        -- Sma Input            
        SmaOut1_DatOut                              : out   std_logic;
        SmaOut2_DatOut                              : out   std_logic;
        SmaOut3_DatOut                              : out   std_logic;
        SmaOut4_DatOut                              : out   std_logic;
                    
        -- Buffer enable            
        SmaIn1_EnOut                                : out   std_logic;
        SmaIn2_EnOut                                : out   std_logic;
        SmaIn3_EnOut                                : out   std_logic;
        SmaIn4_EnOut                                : out   std_logic;
                    
        SmaOut1_EnOut                               : out   std_logic;
        SmaOut2_EnOut                               : out   std_logic;
        SmaOut3_EnOut                               : out   std_logic;
        SmaOut4_EnOut                               : out   std_logic;
        
        -- Axi 1
        Axi1WriteAddrValid_ValIn                    : in    std_logic;
        Axi1WriteAddrReady_RdyOut                   : out   std_logic;
        Axi1WriteAddrAddress_AdrIn                  : in    std_logic_vector(15 downto 0);
        Axi1WriteAddrProt_DatIn                     : in    std_logic_vector(2 downto 0);
                                                    
        Axi1WriteDataValid_ValIn                    : in    std_logic;
        Axi1WriteDataReady_RdyOut                   : out   std_logic;
        Axi1WriteDataData_DatIn                     : in    std_logic_vector(31 downto 0);
        Axi1WriteDataStrobe_DatIn                   : in    std_logic_vector(3 downto 0);
                                                    
        Axi1WriteRespValid_ValOut                   : out   std_logic;
        Axi1WriteRespReady_RdyIn                    : in    std_logic;
        Axi1WriteRespResponse_DatOut                : out   std_logic_vector(1 downto 0);
                                                    
        Axi1ReadAddrValid_ValIn                     : in    std_logic;
        Axi1ReadAddrReady_RdyOut                    : out   std_logic;
        Axi1ReadAddrAddress_AdrIn                   : in    std_logic_vector(15 downto 0);
        Axi1ReadAddrProt_DatIn                      : in    std_logic_vector(2 downto 0);
                                                    
        Axi1ReadDataValid_ValOut                    : out   std_logic;
        Axi1ReadDataReady_RdyIn                     : in    std_logic;
        Axi1ReadDataResponse_DatOut                 : out   std_logic_vector(1 downto 0);
        Axi1ReadDataData_DatOut                     : out   std_logic_vector(31 downto 0);
                                                    
        -- Axi 2                                    
        Axi2WriteAddrValid_ValIn                    : in    std_logic;
        Axi2WriteAddrReady_RdyOut                   : out   std_logic;
        Axi2WriteAddrAddress_AdrIn                  : in    std_logic_vector(15 downto 0);
        Axi2WriteAddrProt_DatIn                     : in    std_logic_vector(2 downto 0);
                                                    
        Axi2WriteDataValid_ValIn                    : in    std_logic;
        Axi2WriteDataReady_RdyOut                   : out   std_logic;
        Axi2WriteDataData_DatIn                     : in    std_logic_vector(31 downto 0);
        Axi2WriteDataStrobe_DatIn                   : in    std_logic_vector(3 downto 0);
                                                    
        Axi2WriteRespValid_ValOut                   : out   std_logic;
        Axi2WriteRespReady_RdyIn                    : in    std_logic;
        Axi2WriteRespResponse_DatOut                : out   std_logic_vector(1 downto 0);
                                                    
        Axi2ReadAddrValid_ValIn                     : in    std_logic;
        Axi2ReadAddrReady_RdyOut                    : out   std_logic;
        Axi2ReadAddrAddress_AdrIn                   : in    std_logic_vector(15 downto 0);
        Axi2ReadAddrProt_DatIn                      : in    std_logic_vector(2 downto 0);
                                                    
        Axi2ReadDataValid_ValOut                    : out   std_logic;
        Axi2ReadDataReady_RdyIn                     : in    std_logic;
        Axi2ReadDataResponse_DatOut                 : out   std_logic_vector(1 downto 0);
        Axi2ReadDataData_DatOut                     : out   std_logic_vector(31 downto 0)
    );
end entity SmaSelector;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture SmaSelector_Arch of SmaSelector is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant SmaOutputSource10Mhz_Con               : std_logic_vector(14 downto 0) := (others => '0');
    constant SmaOutputSourceFpgaPps_Con             : std_logic_vector(14 downto 0) := (0 => '1', others => '0');
    constant SmaOutputSourceMacPps_Con              : std_logic_vector(14 downto 0) := (1 => '1', others => '0');
    constant SmaOutputSourceGnss1Pps_Con            : std_logic_vector(14 downto 0) := (2 => '1', others => '0');
    constant SmaOutputSourceGnss2Pps_Con            : std_logic_vector(14 downto 0) := (3 => '1', others => '0');
    constant SmaOutputSourceIrigMaster_Con          : std_logic_vector(14 downto 0) := (4 => '1', others => '0');
    constant SmaOutputSourceDcfMaster_Con           : std_logic_vector(14 downto 0) := (5 => '1', others => '0');
    constant SmaOutputSourceSignalGen1_Con          : std_logic_vector(14 downto 0) := (6 => '1', others => '0');
    constant SmaOutputSourceSignalGen2_Con          : std_logic_vector(14 downto 0) := (7 => '1', others => '0');
    constant SmaOutputSourceSignalGen3_Con          : std_logic_vector(14 downto 0) := (8 => '1', others => '0');
    constant SmaOutputSourceSignalGen4_Con          : std_logic_vector(14 downto 0) := (9 => '1', others => '0');
    constant SmaOutputSourceUartGnss1_Con           : std_logic_vector(14 downto 0) := (10 => '1', others => '0');
    constant SmaOutputSourceUartGnss2_Con           : std_logic_vector(14 downto 0) := (11 => '1', others => '0');
    constant SmaOutputSourceUartExt_Con             : std_logic_vector(14 downto 0) := (12 => '1', others => '0');
            
    constant SmaOutputSourceGnd_Con                 : std_logic_vector(14 downto 0) := (13 => '1', others => '0');
    constant SmaOutputSourceVcc_Con                 : std_logic_vector(14 downto 0) := (14 => '1', others => '0');
    
    -- SMA Selector version
    constant SmaSelectorMajorVersion_Con            : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant SmaSelectorMinorVersion_Con            : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant SmaSelectorBuildVersion_Con            : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(1, 16));
    constant SmaSelectorVersion_Con                 : std_logic_vector(31 downto 0) := SmaSelectorMajorVersion_Con & SmaSelectorMinorVersion_Con & SmaSelectorBuildVersion_Con;

    -- AXI 1 regs                                                     Addr       , Mask       , RW  , Reset
    constant SmaInputSelect1_Reg_Con                : Axi_Reg_Type:= (x"00000000", x"FFFFFFFF", Rw_E, (SmaInput2SourceSelect_Gen & SmaInput1SourceSelect_Gen));
    constant SmaOutputSelect1_Reg_Con               : Axi_Reg_Type:= (x"00000008", x"FFFFFFFF", Rw_E, (SmaOutput4SourceSelect_Gen & SmaOutput3SourceSelect_Gen));
    constant SmaSelectorVersion1_Reg_Con            : Axi_Reg_Type:= (x"00000010", x"FFFFFFFF", Ro_E, SmaSelectorVersion_Con);
    constant SmaInputStatus_Reg_Con                 : Axi_Reg_Type:= (x"00002000", x"00003333", Ro_E, x"00000000");

    -- AXI 2 regs                                                     Addr       , Mask       , RW  , Reset
    constant SmaInputSelect2_Reg_Con                : Axi_Reg_Type:= (x"00000000", x"FFFFFFFF", Rw_E, (SmaInput4SourceSelect_Gen & SmaInput3SourceSelect_Gen));
    constant SmaOutputSelect2_Reg_Con               : Axi_Reg_Type:= (x"00000008", x"FFFFFFFF", Rw_E, (SmaOutput2SourceSelect_Gen & SmaOutput1SourceSelect_Gen));
    constant SmaSelectorVersion2_Reg_Con            : Axi_Reg_Type:= (x"00000010", x"FFFFFFFF", Ro_E, SmaSelectorVersion_Con);
    
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Sma10MHzSourceEnable_EnReg               : std_logic;
    -- SMA status                   
    signal SmaInputStatus_Dat                       : std_logic_vector(31 downto 0) := (others => '0');    
    signal SmaInputStatus_DatReg                    : std_logic_vector(31 downto 0) := (others => '0');    
        
    -- Selection Map1       
    signal SmaInput1SourceSelect_DatReg             : std_logic_vector(15 downto 0);
    signal SmaInput2SourceSelect_DatReg             : std_logic_vector(15 downto 0);
                                                
    signal SmaOutput3SourceSelect_DatReg            : std_logic_vector(15 downto 0);
    signal SmaOutput4SourceSelect_DatReg            : std_logic_vector(15 downto 0);
            
    -- Selection Map2       
    signal SmaInput3SourceSelect_DatReg             : std_logic_vector(15 downto 0);
    signal SmaInput4SourceSelect_DatReg             : std_logic_vector(15 downto 0);
                                                    
    signal SmaOutput1SourceSelect_DatReg            : std_logic_vector(15 downto 0);
    signal SmaOutput2SourceSelect_DatReg            : std_logic_vector(15 downto 0);
    
    -- AXI4L slave 1 signals and regs
    signal Axi1_AccessState_StaReg                  : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;
    signal Axi1WriteAddrReady_RdyReg                : std_logic;       
    signal Axi1WriteDataReady_RdyReg                : std_logic;   
    signal Axi1WriteRespValid_ValReg                : std_logic;
    signal Axi1WriteRespResponse_DatReg             : std_logic_vector(1 downto 0);   
    signal Axi1ReadAddrReady_RdyReg                 : std_logic;          
    signal Axi1ReadDataValid_ValReg                 : std_logic;
    signal Axi1ReadDataResponse_DatReg              : std_logic_vector(1 downto 0);
    signal Axi1ReadDataData_DatReg                  : std_logic_vector(31 downto 0);

    signal SmaInputSelect1_DatReg                   : std_logic_vector(31 downto 0);
    signal SmaOutputSelect1_DatReg                  : std_logic_vector(31 downto 0);
    signal SmaSelectorVersion1_DatReg               : std_logic_vector(31 downto 0);

    -- AXI4L slave 2 signals and regs
    signal Axi2_AccessState_StaReg                  : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;
    signal Axi2WriteAddrReady_RdyReg                : std_logic;       
    signal Axi2WriteDataReady_RdyReg                : std_logic;   
    signal Axi2WriteRespValid_ValReg                : std_logic;
    signal Axi2WriteRespResponse_DatReg             : std_logic_vector(1 downto 0);   
    signal Axi2ReadAddrReady_RdyReg                 : std_logic;          
    signal Axi2ReadDataValid_ValReg                 : std_logic;
    signal Axi2ReadDataResponse_DatReg              : std_logic_vector(1 downto 0);
    signal Axi2ReadDataData_DatReg                  : std_logic_vector(31 downto 0);

    signal SmaInputSelect2_DatReg                   : std_logic_vector(31 downto 0);
    signal SmaOutputSelect2_DatReg                  : std_logic_vector(31 downto 0);
    signal SmaSelectorVersion2_DatReg               : std_logic_vector(31 downto 0);
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    
    -- SMA status 
    SmaInputStatus_Dat(1 downto 0) <= SmaIn1_DatIn & SmaInput1SourceSelect_DatReg(15);
    SmaInputStatus_Dat(5 downto 4) <= SmaIn2_DatIn & SmaInput2SourceSelect_DatReg(15);
    SmaInputStatus_Dat(9 downto 8) <= SmaIn3_DatIn & SmaInput3SourceSelect_DatReg(15);
    SmaInputStatus_Dat(13 downto 12) <= SmaIn4_DatIn & SmaInput4SourceSelect_DatReg(15);
    
    -- Clock only supported via Sma Input 0 and must be enabled
    Sma10MHzSourceEnable_EnOut                      <= Sma10MHzSourceEnable_EnReg;
                   
    SmaIn1_EnOut <= SmaInput1SourceSelect_DatReg(15);
    SmaIn2_EnOut <= SmaInput2SourceSelect_DatReg(15);
    SmaIn3_EnOut <= SmaInput3SourceSelect_DatReg(15);
    SmaIn4_EnOut <= SmaInput4SourceSelect_DatReg(15);
                       
    SmaOut1_EnOut <= SmaOutput1SourceSelect_DatReg(15);
    SmaOut2_EnOut <= SmaOutput2SourceSelect_DatReg(15);
    SmaOut3_EnOut <= SmaOutput3SourceSelect_DatReg(15);
    SmaOut4_EnOut <= SmaOutput4SourceSelect_DatReg(15);
    
    -- Demultiplex the SMA inputs according to configuration
    SmaExtPpsSource1_EvtOut <=  SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(0) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(0) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(0) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(0) = '1' else '0';

    SmaExtPpsSource2_EvtOut <=  SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(1) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(1) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(1) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(1) = '1' else '0';

    SmaTs1Source_EvtOut <=      SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(2) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(2) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(2) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(2) = '1' else '0';

    SmaTs2Source_EvtOut <=      SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(3) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(3) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(3) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(3) = '1' else '0';

    SmaIrigSlaveSource_DatOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(4) = '1' else
                                 SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(4) = '1' else
                                 SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(4) = '1' else
                                 SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(4) = '1' else '0';
                                 
    SmaDcfSlaveSource_DatOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(5) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(5) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(5) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(5) = '1' else '0';
                                
    SmaTs3Source_EvtOut <=      SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(6) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(6) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(6) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(6) = '1' else '0';

    SmaTs4Source_EvtOut <=      SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(7) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(7) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(7) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(7) = '1' else '0';
                                
    SmaFreqCnt1Source_EvtOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(8) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(8) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(8) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(8) = '1' else '0';
                                
    SmaFreqCnt2Source_EvtOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(9) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(9) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(9) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(9) = '1' else '0';
                                
    SmaFreqCnt3Source_EvtOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(10) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(10) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(10) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(10) = '1' else '0';
                                
    SmaFreqCnt4Source_EvtOut <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(11) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(11) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(11) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(11) = '1' else '0';
    
    SmaUartExtSource_DatOut  <= SmaIn1_DatIn when SmaInput1SourceSelect_DatReg(12) = '1' else
                                SmaIn2_DatIn when SmaInput2SourceSelect_DatReg(12) = '1' else
                                SmaIn3_DatIn when SmaInput3SourceSelect_DatReg(12) = '1' else
                                SmaIn4_DatIn when SmaInput4SourceSelect_DatReg(12) = '1' else '0';
    
                                
    
    -- Multiplex the SMA outputs according to configuration
    with SmaOutput1SourceSelect_DatReg(14 downto 0) select SmaOut1_DatOut <= Sma10MHzSource_ClkIn when SmaOutputSource10Mhz_Con,
                                                                            SmaFpgaPpsSource_EvtIn when SmaOutputSourceFpgaPps_Con,
                                                                            SmaMacPpsSource_EvtIn when SmaOutputSourceMacPps_Con,
                                                                            SmaGnss1PpsSource_EvtIn when SmaOutputSourceGnss1Pps_Con,
                                                                            SmaGnss2PpsSource_EvtIn when SmaOutputSourceGnss2Pps_Con,
                                                                            SmaIrigMasterSource_DatIn when SmaOutputSourceIrigMaster_Con,
                                                                            SmaDcfMasterSource_DatIn when SmaOutputSourceDcfMaster_Con,
                                                                            SmaSignalGen1Source_DatIn when SmaOutputSourceSignalGen1_Con,
                                                                            SmaSignalGen2Source_DatIn when SmaOutputSourceSignalGen2_Con,
                                                                            SmaSignalGen3Source_DatIn when SmaOutputSourceSignalGen3_Con,
                                                                            SmaSignalGen4Source_DatIn when SmaOutputSourceSignalGen4_Con,
                                                                            SmaUartGnss1Source_DatIn when SmaOutputSourceUartGnss1_Con,
                                                                            SmaUartGnss2Source_DatIn when SmaOutputSourceUartGnss2_Con,
                                                                            SmaUartExtSource_DatIn when SmaOutputSourceUartExt_Con,
                                                                            '0' when SmaOutputSourceGnd_Con,
                                                                            '1' when SmaOutputSourceVcc_Con,
                                                                            Sma10MHzSource_ClkIn when others;
    
    with SmaOutput2SourceSelect_DatReg(14 downto 0) select SmaOut2_DatOut <= Sma10MHzSource_ClkIn when SmaOutputSource10Mhz_Con,
                                                                            SmaFpgaPpsSource_EvtIn when SmaOutputSourceFpgaPps_Con,
                                                                            SmaMacPpsSource_EvtIn when SmaOutputSourceMacPps_Con,
                                                                            SmaGnss1PpsSource_EvtIn when SmaOutputSourceGnss1Pps_Con,
                                                                            SmaGnss2PpsSource_EvtIn when SmaOutputSourceGnss2Pps_Con,
                                                                            SmaIrigMasterSource_DatIn when SmaOutputSourceIrigMaster_Con,
                                                                            SmaDcfMasterSource_DatIn when SmaOutputSourceDcfMaster_Con,
                                                                            SmaSignalGen1Source_DatIn when SmaOutputSourceSignalGen1_Con,
                                                                            SmaSignalGen2Source_DatIn when SmaOutputSourceSignalGen2_Con,
                                                                            SmaSignalGen3Source_DatIn when SmaOutputSourceSignalGen3_Con,
                                                                            SmaSignalGen4Source_DatIn when SmaOutputSourceSignalGen4_Con,
                                                                            SmaUartGnss1Source_DatIn when SmaOutputSourceUartGnss1_Con,
                                                                            SmaUartGnss2Source_DatIn when SmaOutputSourceUartGnss2_Con,
                                                                            SmaUartExtSource_DatIn when SmaOutputSourceUartExt_Con,
                                                                            '0' when SmaOutputSourceGnd_Con,
                                                                            '1' when SmaOutputSourceVcc_Con,
                                                                            SmaFpgaPpsSource_EvtIn when others;
    
    with SmaOutput3SourceSelect_DatReg(14 downto 0) select SmaOut3_DatOut <= Sma10MHzSource_ClkIn when SmaOutputSource10Mhz_Con,
                                                                            SmaFpgaPpsSource_EvtIn when SmaOutputSourceFpgaPps_Con,
                                                                            SmaMacPpsSource_EvtIn when SmaOutputSourceMacPps_Con,
                                                                            SmaGnss1PpsSource_EvtIn when SmaOutputSourceGnss1Pps_Con,
                                                                            SmaGnss2PpsSource_EvtIn when SmaOutputSourceGnss2Pps_Con,
                                                                            SmaIrigMasterSource_DatIn when SmaOutputSourceIrigMaster_Con,
                                                                            SmaDcfMasterSource_DatIn when SmaOutputSourceDcfMaster_Con,
                                                                            SmaSignalGen1Source_DatIn when SmaOutputSourceSignalGen1_Con,
                                                                            SmaSignalGen2Source_DatIn when SmaOutputSourceSignalGen2_Con,
                                                                            SmaSignalGen3Source_DatIn when SmaOutputSourceSignalGen3_Con,
                                                                            SmaSignalGen4Source_DatIn when SmaOutputSourceSignalGen4_Con,
                                                                            SmaUartGnss1Source_DatIn when SmaOutputSourceUartGnss1_Con,
                                                                            SmaUartGnss2Source_DatIn when SmaOutputSourceUartGnss2_Con,
                                                                            SmaUartExtSource_DatIn when SmaOutputSourceUartExt_Con,
                                                                            '0' when SmaOutputSourceGnd_Con,
                                                                            '1' when SmaOutputSourceVcc_Con,
                                                                            Sma10MHzSource_ClkIn when others;
    
    with SmaOutput4SourceSelect_DatReg(14 downto 0) select SmaOut4_DatOut <= Sma10MHzSource_ClkIn when SmaOutputSource10Mhz_Con,
                                                                            SmaFpgaPpsSource_EvtIn when SmaOutputSourceFpgaPps_Con,
                                                                            SmaMacPpsSource_EvtIn when SmaOutputSourceMacPps_Con,
                                                                            SmaGnss1PpsSource_EvtIn when SmaOutputSourceGnss1Pps_Con,
                                                                            SmaGnss2PpsSource_EvtIn when SmaOutputSourceGnss2Pps_Con,
                                                                            SmaIrigMasterSource_DatIn when SmaOutputSourceIrigMaster_Con,
                                                                            SmaDcfMasterSource_DatIn when SmaOutputSourceDcfMaster_Con,
                                                                            SmaSignalGen1Source_DatIn when SmaOutputSourceSignalGen1_Con,
                                                                            SmaSignalGen2Source_DatIn when SmaOutputSourceSignalGen2_Con,
                                                                            SmaSignalGen3Source_DatIn when SmaOutputSourceSignalGen3_Con,
                                                                            SmaSignalGen4Source_DatIn when SmaOutputSourceSignalGen4_Con,
                                                                            SmaUartGnss1Source_DatIn when SmaOutputSourceUartGnss1_Con,
                                                                            SmaUartGnss2Source_DatIn when SmaOutputSourceUartGnss2_Con,
                                                                            SmaUartExtSource_DatIn when SmaOutputSourceUartExt_Con,
                                                                            '0' when SmaOutputSourceGnd_Con,
                                                                            '1' when SmaOutputSourceVcc_Con,
                                                                            SmaFpgaPpsSource_EvtIn when others;
    
    -- AXI assignments
    Axi1WriteAddrReady_RdyOut                        <= Axi1WriteAddrReady_RdyReg;        
    Axi1WriteDataReady_RdyOut                        <= Axi1WriteDataReady_RdyReg;   
    Axi1WriteRespValid_ValOut                        <= Axi1WriteRespValid_ValReg;
    Axi1WriteRespResponse_DatOut                     <= Axi1WriteRespResponse_DatReg;   
    Axi1ReadAddrReady_RdyOut                         <= Axi1ReadAddrReady_RdyReg;          
    Axi1ReadDataValid_ValOut                         <= Axi1ReadDataValid_ValReg;
    Axi1ReadDataResponse_DatOut                      <= Axi1ReadDataResponse_DatReg;
    Axi1ReadDataData_DatOut                          <= Axi1ReadDataData_DatReg;

    Axi2WriteAddrReady_RdyOut                        <= Axi2WriteAddrReady_RdyReg;        
    Axi2WriteDataReady_RdyOut                        <= Axi2WriteDataReady_RdyReg;   
    Axi2WriteRespValid_ValOut                        <= Axi2WriteRespValid_ValReg;
    Axi2WriteRespResponse_DatOut                     <= Axi2WriteRespResponse_DatReg;   
    Axi2ReadAddrReady_RdyOut                         <= Axi2ReadAddrReady_RdyReg;          
    Axi2ReadDataValid_ValOut                         <= Axi2ReadDataValid_ValReg;
    Axi2ReadDataResponse_DatOut                      <= Axi2ReadDataResponse_DatReg;
    Axi2ReadDataData_DatOut                          <= Axi2ReadDataData_DatReg;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    
    -- Process to enable the 10 MHz clock
    ClockEnable_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            Sma10MHzSourceEnable_EnReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            -- Clock only supported via Sma Input 0 and must be enabled
            if (unsigned(SmaInput1SourceSelect_DatReg(14 downto 0)) = 0) then
                Sma10MHzSourceEnable_EnReg <= '1';
            else
                Sma10MHzSourceEnable_EnReg <= '0';
            end if;
        end if;
    end process ClockEnable_Prc;
    
    -- Access configuration and monitoring registers via the AXI4L slave 1
    -- Set the SMA Input 1/2 and SMA Output 3/4
    Axi1_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress1                : std_logic_vector(31 downto 0) := (others => '0');    
    begin
        if (SysRstN_RstIn = '0') then            
            Axi1WriteAddrReady_RdyReg <= '0';
            Axi1WriteDataReady_RdyReg <= '0';

            Axi1WriteRespValid_ValReg <= '0';
            Axi1WriteRespResponse_DatReg <= (others => '0');
                                
            Axi1ReadAddrReady_RdyReg <= '0';
            
            Axi1ReadDataValid_ValReg <= '0';
            Axi1ReadDataResponse_DatReg <= (others => '0');
            Axi1ReadDataData_DatReg <= (others => '0');
               
            Axi1_AccessState_StaReg <= Axi_AccessState_Type_Rst_Con;
            
            Axi_Init_Proc(SmaInputSelect1_Reg_Con, SmaInputSelect1_DatReg);
            Axi_Init_Proc(SmaOutputSelect1_Reg_Con, SmaOutputSelect1_DatReg);
            Axi_Init_Proc(SmaSelectorVersion1_Reg_Con, SmaSelectorVersion1_DatReg);
            Axi_Init_Proc(SmaInputStatus_Reg_Con, SmaInputStatus_DatReg);

            SmaInput1SourceSelect_DatReg <= (others => '0');
            SmaInput2SourceSelect_DatReg <= (others => '0');
            SmaOutput3SourceSelect_DatReg <= (others => '0');
            SmaOutput4SourceSelect_DatReg <= (others => '0');
            SmaInputStatus_DatReg <= (others=>'0');
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            if ((Axi1WriteAddrValid_ValIn = '1') and (Axi1WriteAddrReady_RdyReg = '1')) then
                Axi1WriteAddrReady_RdyReg <= '0';
            end if;
            
            if ((Axi1WriteDataValid_ValIn = '1') and (Axi1WriteDataReady_RdyReg = '1')) then
                Axi1WriteDataReady_RdyReg <= '0';
            end if;
            
            if ((Axi1WriteRespValid_ValReg = '1') and (Axi1WriteRespReady_RdyIn = '1')) then
                Axi1WriteRespValid_ValReg <= '0';
            end if;
    
            if ((Axi1ReadAddrValid_ValIn = '1') and (Axi1ReadAddrReady_RdyReg = '1')) then
                Axi1ReadAddrReady_RdyReg <= '0';
            end if;
    
            if ((Axi1ReadDataValid_ValReg = '1') and (Axi1ReadDataReady_RdyIn = '1')) then
                Axi1ReadDataValid_ValReg <= '0';
            end if;
                                    
            case (Axi1_AccessState_StaReg) is
                when Idle_St =>
                    if ((Axi1WriteAddrValid_ValIn = '1') and (Axi1WriteDataValid_ValIn = '1')) then
                        Axi1WriteAddrReady_RdyReg <= '1';
                        Axi1WriteDataReady_RdyReg <= '1';
                        Axi1_AccessState_StaReg <= Write_St;    
                    elsif (Axi1ReadAddrValid_ValIn = '1') then
                        Axi1ReadAddrReady_RdyReg <= '1';
                        Axi1_AccessState_StaReg <= Read_St;    
                    end if;
                    
                when Read_St =>
                    if ((Axi1ReadAddrValid_ValIn = '1') and (Axi1ReadAddrReady_RdyReg = '1')) then
                        TempAddress1 := std_logic_vector(resize(unsigned(Axi1ReadAddrAddress_AdrIn), 32));
                        Axi1ReadDataValid_ValReg <= '1';
                        Axi1ReadDataResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Read_Proc(SmaInputSelect1_Reg_Con, SmaInputSelect1_DatReg, TempAddress1, Axi1ReadDataData_DatReg, Axi1ReadDataResponse_DatReg);
                        Axi_Read_Proc(SmaOutputSelect1_Reg_Con, SmaOutputSelect1_DatReg, TempAddress1, Axi1ReadDataData_DatReg, Axi1ReadDataResponse_DatReg);
                        Axi_Read_Proc(SmaSelectorVersion1_Reg_Con, SmaSelectorVersion1_DatReg, TempAddress1, Axi1ReadDataData_DatReg, Axi1ReadDataResponse_DatReg);
                        Axi_Read_Proc(SmaInputStatus_Reg_Con, SmaInputStatus_DatReg, TempAddress1, Axi1ReadDataData_DatReg, Axi1ReadDataResponse_DatReg);
                        Axi1_AccessState_StaReg <= Resp_St;    
                    end if;
                    
                when Write_St => 
                    if (((Axi1WriteAddrValid_ValIn = '1') and (Axi1WriteAddrReady_RdyReg = '1')) and
                        ((Axi1WriteDataValid_ValIn = '1') and (Axi1WriteDataReady_RdyReg = '1'))) then
                        TempAddress1 := std_logic_vector(resize(unsigned(Axi1WriteAddrAddress_AdrIn), 32));
                        Axi1WriteRespValid_ValReg <= '1';
                        Axi1WriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(SmaInputSelect1_Reg_Con, SmaInputSelect1_DatReg, TempAddress1, Axi1WriteDataData_DatIn, Axi1WriteRespResponse_DatReg);
                        Axi_Write_Proc(SmaOutputSelect1_Reg_Con, SmaOutputSelect1_DatReg, TempAddress1, Axi1WriteDataData_DatIn, Axi1WriteRespResponse_DatReg);
                        Axi_Write_Proc(SmaSelectorVersion1_Reg_Con, SmaSelectorVersion1_DatReg, TempAddress1, Axi1WriteDataData_DatIn, Axi1WriteRespResponse_DatReg);
                        Axi_Write_Proc(SmaInputStatus_Reg_Con, SmaInputStatus_DatReg, TempAddress1, Axi1WriteDataData_DatIn, Axi1WriteRespResponse_DatReg);
                        Axi1_AccessState_StaReg <= Resp_St;
                    end if; 
                    
                when Resp_St =>
                    if (((Axi1WriteRespValid_ValReg = '1') and (Axi1WriteRespReady_RdyIn = '1')) or 
                        ((Axi1ReadDataValid_ValReg = '1') and (Axi1ReadDataReady_RdyIn = '1'))) then
                        Axi1_AccessState_StaReg <= Idle_St;    
                    end if;               
                    
                when others =>
                
            end case;  
            
            SmaInput1SourceSelect_DatReg <= SmaInputSelect1_DatReg(15 downto 0);
            SmaInput2SourceSelect_DatReg <= SmaInputSelect1_DatReg(31 downto 16);
            
            SmaOutput3SourceSelect_DatReg <= SmaOutputSelect1_DatReg(15 downto 0);
            SmaOutput4SourceSelect_DatReg <= SmaOutputSelect1_DatReg(31 downto 16);
            
            SmaInputStatus_DatReg <= SmaInputStatus_Dat;
            
        end if;
    end process Axi1_Prc;

    -- Access configuration and monitoring registers via the AXI4L slave 2
    -- Set the SMA Input 3/4 and SMA Output 1/2
    Axi2_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress2                : std_logic_vector(31 downto 0) := (others => '0');    
    begin
        if (SysRstN_RstIn = '0') then            
            Axi2WriteAddrReady_RdyReg <= '0';
            Axi2WriteDataReady_RdyReg <= '0';

            Axi2WriteRespValid_ValReg <= '0';
            Axi2WriteRespResponse_DatReg <= (others => '0');
                                
            Axi2ReadAddrReady_RdyReg <= '0';
            
            Axi2ReadDataValid_ValReg <= '0';
            Axi2ReadDataResponse_DatReg <= (others => '0');
            Axi2ReadDataData_DatReg <= (others => '0');
               
            Axi2_AccessState_StaReg <= Axi_AccessState_Type_Rst_Con;
            
            Axi_Init_Proc(SmaInputSelect2_Reg_Con, SmaInputSelect2_DatReg);
            Axi_Init_Proc(SmaOutputSelect2_Reg_Con, SmaOutputSelect2_DatReg);
            Axi_Init_Proc(SmaSelectorVersion2_Reg_Con, SmaSelectorVersion2_DatReg);

            SmaInput3SourceSelect_DatReg <= (others => '0');
            SmaInput4SourceSelect_DatReg <= (others => '0');
            SmaOutput1SourceSelect_DatReg <= (others => '0');
            SmaOutput2SourceSelect_DatReg <= (others => '0');
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            if ((Axi2WriteAddrValid_ValIn = '1') and (Axi2WriteAddrReady_RdyReg = '1')) then
                Axi2WriteAddrReady_RdyReg <= '0';
            end if;
            
            if ((Axi2WriteDataValid_ValIn = '1') and (Axi2WriteDataReady_RdyReg = '1')) then
                Axi2WriteDataReady_RdyReg <= '0';
            end if;
            
            if ((Axi2WriteRespValid_ValReg = '1') and (Axi2WriteRespReady_RdyIn = '1')) then
                Axi2WriteRespValid_ValReg <= '0';
            end if;
    
            if ((Axi2ReadAddrValid_ValIn = '1') and (Axi2ReadAddrReady_RdyReg = '1')) then
                Axi2ReadAddrReady_RdyReg <= '0';
            end if;
    
            if ((Axi2ReadDataValid_ValReg = '1') and (Axi2ReadDataReady_RdyIn = '1')) then
                Axi2ReadDataValid_ValReg <= '0';
            end if;
                                    
            case (Axi2_AccessState_StaReg) is
                when Idle_St =>
                    if ((Axi2WriteAddrValid_ValIn = '1') and (Axi2WriteDataValid_ValIn = '1')) then
                        Axi2WriteAddrReady_RdyReg <= '1';
                        Axi2WriteDataReady_RdyReg <= '1';
                        Axi2_AccessState_StaReg <= Write_St;    
                    elsif (Axi2ReadAddrValid_ValIn = '1') then
                        Axi2ReadAddrReady_RdyReg <= '1';
                        Axi2_AccessState_StaReg <= Read_St;    
                    end if;
                    
                when Read_St =>
                    if ((Axi2ReadAddrValid_ValIn = '1') and (Axi2ReadAddrReady_RdyReg = '1')) then
                        TempAddress2 := std_logic_vector(resize(unsigned(Axi2ReadAddrAddress_AdrIn), 32));
                        Axi2ReadDataValid_ValReg <= '1';
                        Axi2ReadDataResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Read_Proc(SmaInputSelect2_Reg_Con, SmaInputSelect2_DatReg, TempAddress2, Axi2ReadDataData_DatReg, Axi2ReadDataResponse_DatReg);
                        Axi_Read_Proc(SmaOutputSelect2_Reg_Con, SmaOutputSelect2_DatReg, TempAddress2, Axi2ReadDataData_DatReg, Axi2ReadDataResponse_DatReg);
                        Axi_Read_Proc(SmaSelectorVersion2_Reg_Con, SmaSelectorVersion2_DatReg, TempAddress2, Axi2ReadDataData_DatReg, Axi2ReadDataResponse_DatReg);
                        Axi2_AccessState_StaReg <= Resp_St;    
                    end if;
                    
                when Write_St => 
                    if (((Axi2WriteAddrValid_ValIn = '1') and (Axi2WriteAddrReady_RdyReg = '1')) and
                        ((Axi2WriteDataValid_ValIn = '1') and (Axi2WriteDataReady_RdyReg = '1'))) then
                        TempAddress2 := std_logic_vector(resize(unsigned(Axi2WriteAddrAddress_AdrIn), 32));
                        Axi2WriteRespValid_ValReg <= '1';
                        Axi2WriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(SmaInputSelect2_Reg_Con, SmaInputSelect2_DatReg, TempAddress2, Axi2WriteDataData_DatIn, Axi2WriteRespResponse_DatReg);
                        Axi_Write_Proc(SmaOutputSelect2_Reg_Con, SmaOutputSelect2_DatReg, TempAddress2, Axi2WriteDataData_DatIn, Axi2WriteRespResponse_DatReg);
                        Axi_Write_Proc(SmaSelectorVersion2_Reg_Con, SmaSelectorVersion2_DatReg, TempAddress2, Axi2WriteDataData_DatIn, Axi2WriteRespResponse_DatReg);
                        Axi2_AccessState_StaReg <= Resp_St;                                                        
                    end if; 
                    
                when Resp_St =>
                    if (((Axi2WriteRespValid_ValReg = '1') and (Axi2WriteRespReady_RdyIn = '1')) or 
                        ((Axi2ReadDataValid_ValReg = '1') and (Axi2ReadDataReady_RdyIn = '1'))) then
                        Axi2_AccessState_StaReg <= Idle_St;    
                    end if;               
                    
                when others =>
                
            end case;  
            
            SmaInput3SourceSelect_DatReg <= SmaInputSelect2_DatReg(15 downto 0);
            SmaInput4SourceSelect_DatReg <= SmaInputSelect2_DatReg(31 downto 16);
            
            SmaOutput1SourceSelect_DatReg <= SmaOutputSelect2_DatReg(15 downto 0);
            SmaOutput2SourceSelect_DatReg <= SmaOutputSelect2_DatReg(31 downto 16);
            
        end if;
    end process Axi2_Prc;

    
    --*************************************************************************************
    -- InstSmaiations and Port mapping
    --*************************************************************************************
        
end architecture SmaSelector_Arch;