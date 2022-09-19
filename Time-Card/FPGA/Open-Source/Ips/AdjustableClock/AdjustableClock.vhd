--*****************************************************************************************
-- Project: Time Card
--
-- Author: Ioannis Sotiropoulos, NetTimeLogic GmbH
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
-- The Adjustable Clock is a time counter that can set its value directly or adjust its  --
-- phase and frequency smoothly based on input adjustments. The component supports up to --
-- 5 external adjustments, plus a register adjustment, which is provided by the CPU via  --
-- the AXI slave. Each adjustment input can provide a direct time set to the time counter--
-- (TimeAdjustment), or a phase correction (OffsetAdjustment), or a frequency correction --
-- (DriftAdjustment). The component provides to the output the adjustable clock ClockTime--
-- and its status flags InSync and InHoldover. Also, the PI servo coefficients which are --
-- received by the AXI registers are forwarded to the output.                            --
-------------------------------------------------------------------------------------------
entity AdjustableClock is
    generic (
        ClockPeriod_Gen                             :       natural := 20;                  -- 50MHz system clock, period in nanoseconds
        ClockInSyncThreshold_Gen                    :       natural := 20;                  -- threshold in nanoseconds
        ClockInHoldoverTimeoutSecond_Gen            :       natural range 3 to 1000 := 3    -- holdover in seconds
    );  
    port (  
        -- System   
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
    
        -- Input 1  
        -- Time Adjustment Input    
        TimeAdjustmentIn1_Second_DatIn              : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn1_Nanosecond_DatIn          : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn1_ValIn                     : in    std_logic := '0';
        -- Offset Adjustment Input  
        OffsetAdjustmentIn1_Second_DatIn            : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn1_Nanosecond_DatIn        : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn1_Sign_DatIn              : in    std_logic := '0';
        OffsetAdjustmentIn1_Interval_DatIn          : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn1_ValIn                   : in    std_logic := '0';
        -- Drift Adjustment Input   
        DriftAdjustmentIn1_Nanosecond_DatIn         : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn1_Sign_DatIn               : in    std_logic := '0';
        DriftAdjustmentIn1_Interval_DatIn           : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn1_ValIn                    : in    std_logic := '0';
    
        -- Input 2  
        -- Time Adjustment Input    
        TimeAdjustmentIn2_Second_DatIn              : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn2_Nanosecond_DatIn          : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn2_ValIn                     : in    std_logic := '0';
        -- Offset Adjustment Input  
        OffsetAdjustmentIn2_Second_DatIn            : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn2_Nanosecond_DatIn        : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn2_Sign_DatIn              : in    std_logic := '0';
        OffsetAdjustmentIn2_Interval_DatIn          : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn2_ValIn                   : in    std_logic := '0';
        -- Drift Adjustment Input   
        DriftAdjustmentIn2_Nanosecond_DatIn         : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn2_Sign_DatIn               : in    std_logic := '0';
        DriftAdjustmentIn2_Interval_DatIn           : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn2_ValIn                    : in    std_logic := '0';
    
        -- Input 3  
        -- Time Adjustment Input    
        TimeAdjustmentIn3_Second_DatIn              : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn3_Nanosecond_DatIn          : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn3_ValIn                     : in    std_logic := '0';
        -- Offset Adjustment Input  
        OffsetAdjustmentIn3_Second_DatIn            : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn3_Nanosecond_DatIn        : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn3_Sign_DatIn              : in    std_logic := '0';
        OffsetAdjustmentIn3_Interval_DatIn          : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn3_ValIn                   : in    std_logic := '0';
        -- Drift Adjustment Input   
        DriftAdjustmentIn3_Nanosecond_DatIn         : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn3_Sign_DatIn               : in    std_logic := '0';
        DriftAdjustmentIn3_Interval_DatIn           : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn3_ValIn                    : in    std_logic := '0';
    
        -- Input 4  
        -- Time Adjustment Input    
        TimeAdjustmentIn4_Second_DatIn              : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn4_Nanosecond_DatIn          : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn4_ValIn                     : in    std_logic := '0';
        -- Offset Adjustment Input  
        OffsetAdjustmentIn4_Second_DatIn            : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn4_Nanosecond_DatIn        : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn4_Sign_DatIn              : in    std_logic := '0';
        OffsetAdjustmentIn4_Interval_DatIn          : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn4_ValIn                   : in    std_logic := '0';
        -- Drift Adjustment Input   
        DriftAdjustmentIn4_Nanosecond_DatIn         : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn4_Sign_DatIn               : in    std_logic := '0';
        DriftAdjustmentIn4_Interval_DatIn           : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn4_ValIn                    : in    std_logic := '0';
    
        -- Input 5  
        -- Time Adjustment Input    
        TimeAdjustmentIn5_Second_DatIn              : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn5_Nanosecond_DatIn          : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustmentIn5_ValIn                     : in    std_logic := '0';
        -- Offset Adjustment Input  
        OffsetAdjustmentIn5_Second_DatIn            : in    std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn5_Nanosecond_DatIn        : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn5_Sign_DatIn              : in    std_logic := '0';
        OffsetAdjustmentIn5_Interval_DatIn          : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        OffsetAdjustmentIn5_ValIn                   : in    std_logic := '0';
        -- Drift Adjustment Input   
        DriftAdjustmentIn5_Nanosecond_DatIn         : in    std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn5_Sign_DatIn               : in    std_logic := '0';
        DriftAdjustmentIn5_Interval_DatIn           : in    std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
        DriftAdjustmentIn5_ValIn                    : in    std_logic := '0';
    
        -- Time Output  
        ClockTime_Second_DatOut                     : out   std_logic_vector((SecondWidth_Con-1) downto 0);
        ClockTime_Nanosecond_DatOut                 : out   std_logic_vector((NanosecondWidth_Con-1) downto 0);
        ClockTime_TimeJump_DatOut                   : out   std_logic;
        ClockTime_ValOut                            : out   std_logic;
    
        -- In Sync Output   
        InSync_DatOut                               : out   std_logic;
        InHoldover_DatOut                           : out   std_logic;
    
        -- Servo Output 
        ServoFactorsValid_ValOut                    : out   std_logic;
        ServoOffsetFactorP_DatOut                   : out   std_logic_vector(31 downto 0);
        ServoOffsetFactorI_DatOut                   : out   std_logic_vector(31 downto 0);
        ServoDriftFactorP_DatOut                    : out   std_logic_vector(31 downto 0);
        ServoDriftFactorI_DatOut                    : out   std_logic_vector(31 downto 0);
    
        -- Axi  
        AxiWriteAddrValid_ValIn                     : in    std_logic;
        AxiWriteAddrReady_RdyOut                    : out   std_logic;
        AxiWriteAddrAddress_AdrIn                   : in    std_logic_vector(15 downto 0);
        AxiWriteAddrProt_DatIn                      : in    std_logic_vector(2 downto 0);
    
        AxiWriteDataValid_ValIn                     : in    std_logic;
        AxiWriteDataReady_RdyOut                    : out   std_logic;
        AxiWriteDataData_DatIn                      : in    std_logic_vector(31 downto 0);
        AxiWriteDataStrobe_DatIn                    : in    std_logic_vector(3 downto 0);
    
        AxiWriteRespValid_ValOut                    : out   std_logic;
        AxiWriteRespReady_RdyIn                     : in    std_logic;
        AxiWriteRespResponse_DatOut                 : out   std_logic_vector(1 downto 0);
    
        AxiReadAddrValid_ValIn                      : in    std_logic;
        AxiReadAddrReady_RdyOut                     : out   std_logic;
        AxiReadAddrAddress_AdrIn                    : in    std_logic_vector(15 downto 0);
        AxiReadAddrProt_DatIn                       : in    std_logic_vector(2 downto 0);
    
        AxiReadDataValid_ValOut                     : out   std_logic;
        AxiReadDataReady_RdyIn                      : in    std_logic;
        AxiReadDataResponse_DatOut                  : out   std_logic_vector(1 downto 0);
        AxiReadDataData_DatOut                      : out   std_logic_vector(31 downto 0)

    );
end entity AdjustableClock;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture AdjustableClock_Arch of AdjustableClock is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant ClockMajorVersion_Con                  : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant ClockMinorVersion_Con                  : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant ClockBuildVersion_Con                  : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant ClockVersion_Con                       : std_logic_vector(31 downto 0) := ClockMajorVersion_Con & ClockMinorVersion_Con & ClockBuildVersion_Con;
    
    -- AXI registers    
    constant ClockControl_Reg_Con                   : Axi_Reg_Type:= (x"00000000", x"C000010F", Rw_E, x"00000000");
    constant ClockStatus_Reg_Con                    : Axi_Reg_Type:= (x"00000004", x"00000003", Ro_E, x"00000000");
    constant ClockSelect_Reg_Con                    : Axi_Reg_Type:= (x"00000008", x"00FF00FF", Rw_E, x"00000000");
    constant ClockVersion_Reg_Con                   : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, ClockVersion_Con);
    constant ClockTimeValueL_Reg_Con                : Axi_Reg_Type:= (x"00000010", x"FFFFFFFF", Ro_E, x"00000000");
    constant ClockTimeValueH_Reg_Con                : Axi_Reg_Type:= (x"00000014", x"FFFFFFFF", Ro_E, x"00000000");
    constant ClockTimeAdjValueL_Reg_Con             : Axi_Reg_Type:= (x"00000020", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockTimeAdjValueH_Reg_Con             : Axi_Reg_Type:= (x"00000024", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockOffsetAdjValue_Reg_Con            : Axi_Reg_Type:= (x"00000030", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockOffsetAdjInterval_Reg_Con         : Axi_Reg_Type:= (x"00000034", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockDriftAdjValue_Reg_Con             : Axi_Reg_Type:= (x"00000040", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockDriftAdjInterval_Reg_Con          : Axi_Reg_Type:= (x"00000044", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockInSyncThreshold_Reg_Con           : Axi_Reg_Type:= (x"00000050", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockServoOffsetFactorP_Reg_Con        : Axi_Reg_Type:= (x"00000060", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockServoOffsetFactorI_Reg_Con        : Axi_Reg_Type:= (x"00000064", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockServoDriftFactorP_Reg_Con         : Axi_Reg_Type:= (x"00000068", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockServoDriftFactorI_Reg_Con         : Axi_Reg_Type:= (x"0000006C", x"FFFFFFFF", Rw_E, x"00000000");
    constant ClockStatusOffset_Reg_Con              : Axi_Reg_Type:= (x"00000070", x"FFFFFFFF", Ro_E, x"00000000");
    constant ClockStatusDrift_Reg_Con               : Axi_Reg_Type:= (x"00000074", x"FFFFFFFF", Ro_E, x"00000000");
    
    -- AXI reg bits 
    constant ClockControl_EnableBit_Con             : natural := 0;
    constant ClockControl_TimeAdjValBit_Con         : natural := 1;
    constant ClockControl_OffsetAdjValBit_Con       : natural := 2;
    constant ClockControl_DriftAdjValBit_Con        : natural := 3;
    constant ClockControl_ServoValBit_Con           : natural := 8;
    constant ClockControl_TimeReadValBit_Con        : natural := 30;
    constant ClockControl_TimeReadDValBit_Con       : natural := 31;
    
    constant ClockStatus_InSyncBit_Con              : natural := 0;
    constant ClockStatus_InHoldoverBit_Con          : natural := 1;
        
    -- max 2 ns extra per clock cycle   
    constant CountAdjustmentWidth_Con               : natural := integer(ceil(log2(real(2 + 1)))); -- 3 possible corection values: 0,1,2
    constant ClockIncrementWidth_Con                : natural := integer(ceil(log2(real((ClockPeriod_Gen + 2) + 1)))); 

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    -- Enable clock
    signal Enable_Ena                               : std_logic;
    signal SelectInput_Dat                          : unsigned(7 downto 0);

    -- Register Mapped Time Adjustment
    signal TimeAdjustmentReg_Second_Dat             : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal TimeAdjustmentReg_Nanosecond_Dat         : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal TimeAdjustmentReg_Val                    : std_logic;
    -- Register Mapped Offset Adjustment
    signal OffsetAdjustmentReg_Second_Dat           : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal OffsetAdjustmentReg_Nanosecond_Dat       : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal OffsetAdjustmentReg_Sign_Dat             : std_logic;
    signal OffsetAdjustmentReg_Interval_Dat         : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetAdjustmentReg_Val                  : std_logic;
    -- Register Mapped Adjustment
    signal DriftAdjustmentReg_Nanosecond_Dat        : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal DriftAdjustmentReg_Sign_Dat              : std_logic;
    signal DriftAdjustmentReg_Interval_Dat          : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftAdjustmentReg_Val                   : std_logic;
    -- Multiplexed Time Adjustment
    signal TimeAdjustmentMux_Second_Dat             : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal TimeAdjustmentMux_Nanosecond_Dat         : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal TimeAdjustmentMux_Val                    : std_logic;
    -- Multiplexed Offset Adjustment
    signal OffsetAdjustmentMux_Second_Dat           : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_Nanosecond_Dat       : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_Sign_Dat             : std_logic;
    signal OffsetAdjustmentMux_Interval_Dat         : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_Val                  : std_logic;
    -- Multiplexed Drift Adjustment
    signal DriftAdjustmentMux_Nanosecond_Dat        : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal DriftAdjustmentMux_Sign_Dat              : std_logic;
    signal DriftAdjustmentMux_Interval_Dat          : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftAdjustmentMux_Val                   : std_logic;

    -- Calculate Drift Adjustment
    signal DriftAdjustmentMux_ValReg                : std_logic;
    signal DriftAdjustmentMux_IntervalExtend_DatReg : std_logic_vector(((2*AdjustmentIntervalWidth_Con)-1) downto 0);
    signal DriftAdjustmentMux_PeriodExtend_DatReg   : std_logic_vector(((2*AdjustmentIntervalWidth_Con)-1) downto 0);
    signal DriftAdjustmentMux_Nanosecond_DatReg     : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftAdjustmentMux_Interval_DatReg       : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftAdjustmentMux_Sign_DatReg           : std_logic;
    signal StartCalcDriftInterval_EvtReg            : std_logic;
    signal CalcDriftInterval_StaReg                 : std_logic;
    signal CalcDriftIntervalStep_CntReg             : natural;
    signal DriftAdjustmentOld_Nanosecond_DatReg     : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftAdjustmentOld_Sign_DatReg           : std_logic;

    signal DriftCount_CntReg                        : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftInterval_DatReg                     : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftNanosecond_DatReg                   : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal DriftSign_DatReg                         : std_logic;

    -- Calculate Offset Adjustment
    signal OffsetAdjustmentMux_ValReg               : std_logic;
    signal OffsetAdjustmentMux_Second_DatReg        : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_Nanosecond_DatReg    : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_Sign_DatReg          : std_logic;
    signal OffsetAdjustmentMux_Interval_DatReg      : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetAdjustmentMux_IntervalExtend_DatReg: std_logic_vector(((2*AdjustmentIntervalWidth_Con)-1) downto 0);
    signal OffsetAdjustmentMux_PeriodExtend_DatReg  : std_logic_vector(((2*AdjustmentIntervalWidth_Con)-1) downto 0);
    signal StartCalcOffsetInterval_EvtReg           : std_logic;
    signal CalcOffsetInterval_StaReg                : std_logic;
    signal CalcOffsetIntervalStep_CntReg            : natural;

    signal OffsetCount_CntReg                       : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetInterval_DatReg                    : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetSecond_DatReg                      : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal OffsetNanosecond_DatReg                  : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetNanosecondOrigin_DatReg            : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    signal OffsetSign_DatReg                        : std_logic;

    -- Reg Time Adjustment
    signal TimeAdjustmentMux_ValReg                 : std_logic;

    -- Clock Time
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_TimeJump_DatReg                : std_logic;
    signal ClockTime_ValReg                         : std_logic;
    -- clock increase per correction
    signal ClockIncrement_DatReg                    : std_logic_vector((ClockIncrementWidth_Con-1) downto 0);
    -- time adjustment
    signal TimeAdjust_Second_DatReg                 : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal TimeAdjust_Nanosecond_DatReg             : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal TimeAdjust_ValReg                        : std_logic;
    -- counter adjustment
    signal CountAdjust_Nanosecond_DatReg            : std_logic_vector((CountAdjustmentWidth_Con-1) downto 0);
    signal CountAdjust_Sign_DatReg                  : std_logic;
    signal CountAdjust_ValReg                       : std_logic;

    -- in sync and in holdover
    signal InSync_DatReg                            : std_logic;
    signal InHoldover_DatReg                        : std_logic;
    signal ClockTimeB0_DatReg                       : std_logic;
    signal Holdover_CntReg                          : natural range 0 to ClockInHoldoverTimeoutSecond_Gen;
    signal OffsetArray_DatReg                       : std_logic_vector(3 downto 0);

    -- Axi
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;

    signal AxiWriteAddrReady_RdyReg                 : std_logic;
    signal AxiWriteDataReady_RdyReg                 : std_logic;
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);
    signal AxiReadAddrReady_RdyReg                  : std_logic;
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);

    signal ClockSelect_DatReg                       : std_logic_vector(31 downto 0);
    signal ClockStatus_DatReg                       : std_logic_vector(31 downto 0);
    signal ClockControl_DatReg                      : std_logic_vector(31 downto 0);
    signal ClockVersion_DatReg                      : std_logic_vector(31 downto 0);
    signal ClockTimeValueL_DatReg                   : std_logic_vector(31 downto 0);
    signal ClockTimeValueH_DatReg                   : std_logic_vector(31 downto 0);
    signal ClockTimeAdjValueL_DatReg                : std_logic_vector(31 downto 0);
    signal ClockTimeAdjValueH_DatReg                : std_logic_vector(31 downto 0);
    signal ClockOffsetAdjValue_DatReg               : std_logic_vector(31 downto 0);
    signal ClockOffsetAdjInterval_DatReg            : std_logic_vector(31 downto 0);
    signal ClockDriftAdjValue_DatReg                : std_logic_vector(31 downto 0);
    signal ClockDriftAdjInterval_DatReg             : std_logic_vector(31 downto 0);
    signal ClockInSyncThreshold_DatReg              : std_logic_vector(31 downto 0) := std_logic_vector(to_unsigned(ClockInSyncThreshold_Gen, 32));
    signal ClockServoOffsetFactorP_DatReg           : std_logic_vector(31 downto 0) := OffsetFactorP_Con;
    signal ClockServoOffsetFactorI_DatReg           : std_logic_vector(31 downto 0) := OffsetFactorI_Con;
    signal ClockServoDriftFactorP_DatReg            : std_logic_vector(31 downto 0) := DriftFactorP_Con;
    signal ClockServoDriftFactorI_DatReg            : std_logic_vector(31 downto 0) := DriftFactorI_Con;
    signal ClockStatusOffset_DatReg                 : std_logic_vector(31 downto 0);
    signal ClockStatusDrift_DatReg                  : std_logic_vector(31 downto 0);

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    ClockTime_Second_DatOut                         <= ClockTime_Second_DatReg;
    ClockTime_Nanosecond_DatOut                     <= ClockTime_Nanosecond_DatReg;
    ClockTime_TimeJump_DatOut                       <= ClockTime_TimeJump_DatReg;
    ClockTime_ValOut                                <= ClockTime_ValReg;
    
    InSync_DatOut                                   <= InSync_DatReg;
    InHoldover_DatOut                               <= InHoldover_DatReg;
    
    ServoFactorsValid_ValOut                        <= ClockControl_DatReg(ClockControl_ServoValBit_Con);
    ServoOffsetFactorP_DatOut                       <= ClockServoOffsetFactorP_DatReg;
    ServoOffsetFactorI_DatOut                       <= ClockServoOffsetFactorI_DatReg;
    ServoDriftFactorP_DatOut                        <= ClockServoDriftFactorP_DatReg;
    ServoDriftFactorI_DatOut                        <= ClockServoDriftFactorI_DatReg;
    
    Enable_Ena                                      <= ClockControl_DatReg(ClockControl_EnableBit_Con);
    SelectInput_Dat                                 <= unsigned(ClockSelect_DatReg(7 downto 0));

    -- Clock Source Mux for Time, Offset and Drift adjustments
    TimeAdjustmentMux_Second_Dat                    <= TimeAdjustmentIn1_Second_DatIn when (SelectInput_Dat = 1) else
                                                       TimeAdjustmentIn2_Second_DatIn when (SelectInput_Dat = 2) else
                                                       TimeAdjustmentIn3_Second_DatIn when (SelectInput_Dat = 3) else
                                                       TimeAdjustmentIn4_Second_DatIn when (SelectInput_Dat = 4) else
                                                       TimeAdjustmentIn5_Second_DatIn when (SelectInput_Dat = 5) else
                                                       TimeAdjustmentReg_Second_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    TimeAdjustmentMux_Nanosecond_Dat                <= TimeAdjustmentIn1_Nanosecond_DatIn when (SelectInput_Dat = 1) else
                                                       TimeAdjustmentIn2_Nanosecond_DatIn when (SelectInput_Dat = 2) else
                                                       TimeAdjustmentIn3_Nanosecond_DatIn when (SelectInput_Dat = 3) else
                                                       TimeAdjustmentIn4_Nanosecond_DatIn when (SelectInput_Dat = 4) else
                                                       TimeAdjustmentIn5_Nanosecond_DatIn when (SelectInput_Dat = 5) else
                                                       TimeAdjustmentReg_Nanosecond_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    TimeAdjustmentMux_Val                           <= TimeAdjustmentIn1_ValIn when (SelectInput_Dat = 1) else
                                                       TimeAdjustmentIn2_ValIn when (SelectInput_Dat = 2) else
                                                       TimeAdjustmentIn3_ValIn when (SelectInput_Dat = 3) else
                                                       TimeAdjustmentIn4_ValIn when (SelectInput_Dat = 4) else
                                                       TimeAdjustmentIn5_ValIn when (SelectInput_Dat = 5) else
                                                       TimeAdjustmentReg_Val when (SelectInput_Dat = 254) else
                                                       '0';
    
    OffsetAdjustmentMux_Second_Dat                  <= OffsetAdjustmentIn1_Second_DatIn when (SelectInput_Dat = 1) else
                                                       OffsetAdjustmentIn2_Second_DatIn when (SelectInput_Dat = 2) else
                                                       OffsetAdjustmentIn3_Second_DatIn when (SelectInput_Dat = 3) else
                                                       OffsetAdjustmentIn4_Second_DatIn when (SelectInput_Dat = 4) else
                                                       OffsetAdjustmentIn5_Second_DatIn when (SelectInput_Dat = 5) else
                                                       OffsetAdjustmentReg_Second_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    OffsetAdjustmentMux_Nanosecond_Dat              <= OffsetAdjustmentIn1_Nanosecond_DatIn when (SelectInput_Dat = 1) else
                                                       OffsetAdjustmentIn2_Nanosecond_DatIn when (SelectInput_Dat = 2) else
                                                       OffsetAdjustmentIn3_Nanosecond_DatIn when (SelectInput_Dat = 3) else
                                                       OffsetAdjustmentIn4_Nanosecond_DatIn when (SelectInput_Dat = 4) else
                                                       OffsetAdjustmentIn5_Nanosecond_DatIn when (SelectInput_Dat = 5) else
                                                       OffsetAdjustmentReg_Nanosecond_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    OffsetAdjustmentMux_Sign_Dat                    <= OffsetAdjustmentIn1_Sign_DatIn when (SelectInput_Dat = 1) else
                                                       OffsetAdjustmentIn2_Sign_DatIn when (SelectInput_Dat = 2) else
                                                       OffsetAdjustmentIn3_Sign_DatIn when (SelectInput_Dat = 3) else
                                                       OffsetAdjustmentIn4_Sign_DatIn when (SelectInput_Dat = 4) else
                                                       OffsetAdjustmentIn5_Sign_DatIn when (SelectInput_Dat = 5) else
                                                       OffsetAdjustmentReg_Sign_Dat when (SelectInput_Dat = 254) else
                                                       '0';
    OffsetAdjustmentMux_Interval_Dat                <= OffsetAdjustmentIn1_Interval_DatIn when (SelectInput_Dat = 1) else
                                                       OffsetAdjustmentIn2_Interval_DatIn when (SelectInput_Dat = 2) else
                                                       OffsetAdjustmentIn3_Interval_DatIn when (SelectInput_Dat = 3) else
                                                       OffsetAdjustmentIn4_Interval_DatIn when (SelectInput_Dat = 4) else
                                                       OffsetAdjustmentIn5_Interval_DatIn when (SelectInput_Dat = 5) else
                                                       OffsetAdjustmentReg_Interval_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    OffsetAdjustmentMux_Val                         <= OffsetAdjustmentIn1_ValIn when (SelectInput_Dat = 1) else
                                                       OffsetAdjustmentIn2_ValIn when (SelectInput_Dat = 2) else
                                                       OffsetAdjustmentIn3_ValIn when (SelectInput_Dat = 3) else
                                                       OffsetAdjustmentIn4_ValIn when (SelectInput_Dat = 4) else
                                                       OffsetAdjustmentIn5_ValIn when (SelectInput_Dat = 5) else
                                                       OffsetAdjustmentReg_Val when (SelectInput_Dat = 254) else
                                                       '0';
    
    DriftAdjustmentMux_Nanosecond_Dat               <= DriftAdjustmentIn1_Nanosecond_DatIn when (SelectInput_Dat = 1) else
                                                       DriftAdjustmentIn2_Nanosecond_DatIn when (SelectInput_Dat = 2) else
                                                       DriftAdjustmentIn3_Nanosecond_DatIn when (SelectInput_Dat = 3) else
                                                       DriftAdjustmentIn4_Nanosecond_DatIn when (SelectInput_Dat = 4) else
                                                       DriftAdjustmentIn5_Nanosecond_DatIn when (SelectInput_Dat = 5) else
                                                       DriftAdjustmentReg_Nanosecond_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    DriftAdjustmentMux_Sign_Dat                     <= DriftAdjustmentIn1_Sign_DatIn when (SelectInput_Dat = 1) else
                                                       DriftAdjustmentIn2_Sign_DatIn when (SelectInput_Dat = 2) else
                                                       DriftAdjustmentIn3_Sign_DatIn when (SelectInput_Dat = 3) else
                                                       DriftAdjustmentIn4_Sign_DatIn when (SelectInput_Dat = 4) else
                                                       DriftAdjustmentIn5_Sign_DatIn when (SelectInput_Dat = 5) else
                                                       DriftAdjustmentReg_Sign_Dat when (SelectInput_Dat = 254) else
                                                       '0';
    DriftAdjustmentMux_Interval_Dat                 <= DriftAdjustmentIn1_Interval_DatIn when (SelectInput_Dat = 1) else
                                                       DriftAdjustmentIn2_Interval_DatIn when (SelectInput_Dat = 2) else
                                                       DriftAdjustmentIn3_Interval_DatIn when (SelectInput_Dat = 3) else
                                                       DriftAdjustmentIn4_Interval_DatIn when (SelectInput_Dat = 4) else
                                                       DriftAdjustmentIn5_Interval_DatIn when (SelectInput_Dat = 5) else
                                                       DriftAdjustmentReg_Interval_Dat when (SelectInput_Dat = 254) else
                                                       (others => '0');
    DriftAdjustmentMux_Val                          <= DriftAdjustmentIn1_ValIn when (SelectInput_Dat = 1) else
                                                       DriftAdjustmentIn2_ValIn when (SelectInput_Dat = 2) else
                                                       DriftAdjustmentIn3_ValIn when (SelectInput_Dat = 3) else
                                                       DriftAdjustmentIn4_ValIn when (SelectInput_Dat = 4) else
                                                       DriftAdjustmentIn5_ValIn when (SelectInput_Dat = 5) else
                                                       DriftAdjustmentReg_Val when (SelectInput_Dat = 254) else
                                                       '0';
    
    -- Register Mapped Adjustments  
    TimeAdjustmentReg_Second_Dat                    <= ClockTimeAdjValueH_DatReg;
    TimeAdjustmentReg_Nanosecond_Dat                <= ClockTimeAdjValueL_DatReg;
    TimeAdjustmentReg_Val                           <= ClockControl_DatReg(ClockControl_TimeAdjValBit_Con);
    
    OffsetAdjustmentReg_Second_Dat                  <= (others => '0');
    OffsetAdjustmentReg_Nanosecond_Dat              <= "00" & ClockOffsetAdjValue_DatReg(29 downto 0);
    OffsetAdjustmentReg_Sign_Dat                    <= ClockOffsetAdjValue_DatReg(31);
    OffsetAdjustmentReg_Interval_Dat                <= ClockOffsetAdjInterval_DatReg;
    OffsetAdjustmentReg_Val                         <= ClockControl_DatReg(ClockControl_OffsetAdjValBit_Con);
    
    DriftAdjustmentReg_Nanosecond_Dat               <= "00" & ClockDriftAdjValue_DatReg(29 downto 0);
    DriftAdjustmentReg_Sign_Dat                     <= ClockDriftAdjValue_DatReg(31);
    DriftAdjustmentReg_Interval_Dat                 <= ClockDriftAdjInterval_DatReg;
    DriftAdjustmentReg_Val                          <= ClockControl_DatReg(ClockControl_DriftAdjValBit_Con);
    
    AxiWriteAddrReady_RdyOut                        <= AxiWriteAddrReady_RdyReg;
    AxiWriteDataReady_RdyOut                        <= AxiWriteDataReady_RdyReg;
    AxiWriteRespValid_ValOut                        <= AxiWriteRespValid_ValReg;
    AxiWriteRespResponse_DatOut                     <= AxiWriteRespResponse_DatReg;
    AxiReadAddrReady_RdyOut                         <= AxiReadAddrReady_RdyReg;
    AxiReadDataValid_ValOut                         <= AxiReadDataValid_ValReg;
    AxiReadDataResponse_DatOut                      <= AxiReadDataResponse_DatReg;
    AxiReadDataData_DatOut                          <= AxiReadDataData_DatReg;

    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    -- The process provides when and how big should be the clock adjustment.
    -- A time adjustment indicates a direct time set of the adjustable clock.
    -- The offset and drift adjustments indicate smooth corrections of the adjustable clock. The offset and drift adjustments shuold be applied over given intervals.
    -- Exceptionally, if the offset adjustment is too large to adjust smoothly, then a direct time set will be applied.
    -- The offset and drift adjustments are spread through the corresponding intervals as corrections of 1ns. Each adjustment corrects the period of the the system clock by 1ns.
    -- Depending on the sign of the adjustment, the correction is added or subtracted to the period of the system clock.
    -- Therefore, the period of the system clock is equal to:
    --      - "the original period", if no adjustment is applied at the current clock cycle
    --      - "the original period - 1ns", if an offset or a drift adjustment is applied at the current clock cycle, with negative sign
    --      - "the original period + 1ns", if an offset or a drift adjustment is applied at the current clock cycle, with positive sign
    --      - "the original period - 2ns", if an offset and a drift adjustment is applied at the current clock cycle, both with negative sign
    --      - "the original period + 2ns", if an offset and a drift adjustment is applied at the current clock cycle, both with positive sign
    --      - "the original period", if an offset and a drift adjustment is applied at the current clock cycle, with different signs
    Adjust_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    variable OffsetAdjustmentMux_IntervalTicks_DatVar   : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    variable DriftAdjustmentMux_IntervalTicks_DatVar    : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
    begin
        if (SysRstN_RstIn = '0') then
            DriftAdjustmentMux_ValReg <= '0';
            DriftAdjustmentMux_Nanosecond_DatReg <= (others => '0');
            DriftAdjustmentMux_Sign_DatReg <= '0';
            DriftAdjustmentOld_Nanosecond_DatReg <= (others => '0');
            DriftAdjustmentOld_Sign_DatReg <= '0';
            DriftAdjustmentMux_Interval_DatReg <= (others => '0');
            DriftAdjustmentMux_IntervalExtend_DatReg <= (others => '0');
            DriftAdjustmentMux_PeriodExtend_DatReg <= (others => '0');
            DriftAdjustmentMux_IntervalTicks_DatVar := (others => '0');
            StartCalcDriftInterval_EvtReg <= '0';
            CalcDriftInterval_StaReg <= '0';
            CalcDriftIntervalStep_CntReg <= 0;
            DriftCount_CntReg <= (others => '0');
            DriftInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
            DriftNanosecond_DatReg <= (others => '0');
            DriftSign_DatReg <= '0';

            OffsetAdjustmentMux_ValReg <= '0';
            OffsetAdjustmentMux_Second_DatReg <= (others => '0');
            OffsetAdjustmentMux_Nanosecond_DatReg <= (others => '0');
            OffsetAdjustmentMux_Sign_DatReg <= '0';
            OffsetAdjustmentMux_Interval_DatReg <= (others => '0');
            OffsetAdjustmentMux_IntervalExtend_DatReg <= (others => '0');
            OffsetAdjustmentMux_PeriodExtend_DatReg <= (others => '0');
            OffsetAdjustmentMux_IntervalTicks_DatVar := (others => '0');
            StartCalcOffsetInterval_EvtReg <= '0';
            CalcOffsetInterval_StaReg <= '0';
            CalcOffsetIntervalStep_CntReg <= 0;
            OffsetCount_CntReg <= (others => '0');
            OffsetInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
            OffsetSecond_DatReg <= (others => '0');
            OffsetNanosecond_DatReg <= (others => '0');
            OffsetNanosecondOrigin_DatReg <= (others => '0');
            OffsetSign_DatReg <= '0';

            TimeAdjustmentMux_ValReg <= '0';

            TimeAdjust_Second_DatReg <= (others => '0');
            TimeAdjust_Nanosecond_DatReg <= (others => '0');
            TimeAdjust_ValReg <= '0';

            CountAdjust_Nanosecond_DatReg <= (others => '0');
            CountAdjust_Sign_DatReg <= '0';
            CountAdjust_ValReg <= '0';
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            TimeAdjust_ValReg <= '0';
            
            -- offset and drift correction
            CountAdjust_ValReg <= '0';
            CountAdjust_Nanosecond_DatReg <= (others => '0');
            CountAdjust_Sign_DatReg <= '0';
            
            if ((((unsigned(OffsetNanosecond_DatReg) > 0) and (unsigned(OffsetSecond_DatReg) = 0)) and
                 ((unsigned(OffsetNanosecondOrigin_DatReg) /= 0) and (unsigned(OffsetInterval_DatReg) /= 0)) and
                 ((unsigned(OffsetCount_CntReg) + unsigned(OffsetNanosecondOrigin_DatReg)) >= unsigned(OffsetInterval_DatReg))) and
                (((unsigned(DriftNanosecond_DatReg) /= 0) and (unsigned(DriftInterval_DatReg) /= 0)) and
                 ((unsigned(DriftCount_CntReg) + unsigned(DriftNanosecond_DatReg)) >= unsigned(DriftInterval_DatReg)))) then
                if ((OffsetSign_DatReg = '1') and (DriftSign_DatReg = '1')) then -- both negative
                    CountAdjust_Nanosecond_DatReg <= std_logic_vector(to_unsigned(2, CountAdjustmentWidth_Con));
                    CountAdjust_Sign_DatReg <= '1';
                    CountAdjust_ValReg <= '1';
                elsif (OffsetSign_DatReg /= DriftSign_DatReg) then                  -- no correction
                    CountAdjust_Nanosecond_DatReg <= std_logic_vector(to_unsigned(0, CountAdjustmentWidth_Con));
                    CountAdjust_Sign_DatReg <= OffsetSign_DatReg;
                    CountAdjust_ValReg <= '1';
                elsif ((OffsetSign_DatReg = '0') and (DriftSign_DatReg = '0')) then -- both positive
                    CountAdjust_Nanosecond_DatReg <= std_logic_vector(to_unsigned(2, CountAdjustmentWidth_Con));
                    CountAdjust_Sign_DatReg <= '0';
                    CountAdjust_ValReg <= '1';
                end if;
            elsif (((unsigned(OffsetNanosecond_DatReg) > 0) and (unsigned(OffsetSecond_DatReg) = 0)) and
                   ((unsigned(OffsetNanosecondOrigin_DatReg) /= 0) and (unsigned(OffsetInterval_DatReg) /= 0)) and
                   ((unsigned(OffsetCount_CntReg) + unsigned(OffsetNanosecondOrigin_DatReg)) >= unsigned(OffsetInterval_DatReg))) then
                CountAdjust_Nanosecond_DatReg <= std_logic_vector(to_unsigned(1, CountAdjustmentWidth_Con));
                CountAdjust_Sign_DatReg <= OffsetSign_DatReg;
                CountAdjust_ValReg <= '1';
            elsif ((unsigned(DriftNanosecond_DatReg) /= 0) and (unsigned(DriftInterval_DatReg) /= 0) and
            ((unsigned(DriftCount_CntReg)+unsigned(DriftNanosecond_DatReg))>=unsigned(DriftInterval_DatReg))) then
                CountAdjust_Nanosecond_DatReg <= std_logic_vector(to_unsigned(1, CountAdjustmentWidth_Con));
                CountAdjust_Sign_DatReg <= DriftSign_DatReg;
                CountAdjust_ValReg <= '1';
            end if;

            --calc offset count
            if ((unsigned(OffsetSecond_DatReg) = 0) and (unsigned(OffsetNanosecond_DatReg) = 0)) then -- no adjustment
                OffsetCount_CntReg <= (others => '0');
                OffsetInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
                OffsetSecond_DatReg <= (others => '0');
                OffsetNanosecond_DatReg <= (others => '0');
                OffsetNanosecondOrigin_DatReg <= (others => '0');
                OffsetSign_DatReg <= '0';
                TimeAdjust_Second_DatReg <= (others => '0');
                TimeAdjust_Nanosecond_DatReg <= (others => '0');
                TimeAdjust_ValReg <= '0';
            elsif ((unsigned(OffsetSecond_DatReg) /= 0) or (unsigned(OffsetNanosecond_DatReg) >= unsigned(OffsetInterval_DatReg))) then -- if the adjustment is too large to set smoothly or larger than one second, hard set time by adding the offset to the current time
                OffsetCount_CntReg <= (others => '0');
                OffsetInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
                OffsetSecond_DatReg <= (others => '0');
                OffsetNanosecond_DatReg <= (others => '0');
                OffsetNanosecondOrigin_DatReg <= (others => '0');
                OffsetSign_DatReg <= '0';
                TimeAdjust_ValReg <= '1';
                if (OffsetSign_DatReg = '1') then -- negative offset
                    if ((unsigned(ClockTime_Second_DatReg)) < unsigned(OffsetSecond_DatReg)) then --error case
                        TimeAdjust_Second_DatReg <= (others => '0');
                        TimeAdjust_Nanosecond_DatReg <= (others => '0');
                    else
                        if ((unsigned(ClockTime_Nanosecond_DatReg) + (2*ClockPeriod_Gen)) < unsigned(OffsetNanosecond_DatReg)) then
                            if (unsigned(ClockTime_Second_DatReg) - unsigned(OffsetSecond_DatReg) >= 1) then
                                TimeAdjust_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) - unsigned(OffsetSecond_DatReg) - 1);
                                TimeAdjust_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(ClockTime_Nanosecond_DatReg) + SecondNanoseconds_Con + (2*ClockPeriod_Gen) - unsigned(OffsetNanosecond_DatReg) ), NanosecondWidth_Con));
                            else -- error case
                                TimeAdjust_Second_DatReg <= (others => '0');
                                TimeAdjust_Nanosecond_DatReg <= (others => '0');
                            end if;
                        else
                            TimeAdjust_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) - unsigned(OffsetSecond_DatReg));
                            TimeAdjust_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(ClockTime_Nanosecond_DatReg) + (2*ClockPeriod_Gen) - (unsigned(OffsetNanosecond_DatReg)), NanosecondWidth_Con));
                        end if;
                    end if;
                else -- positive offset
                    if ((unsigned(ClockTime_Nanosecond_DatReg) + unsigned(OffsetNanosecond_DatReg) + (2*ClockPeriod_Gen)) >= SecondNanoseconds_Con) then
                        TimeAdjust_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) + unsigned(OffsetSecond_DatReg) + 1);
                        TimeAdjust_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(ClockTime_Nanosecond_DatReg) + unsigned(OffsetNanosecond_DatReg) + (2*ClockPeriod_Gen) - SecondNanoseconds_Con), NanosecondWidth_Con));
                    else
                        TimeAdjust_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) + unsigned(OffsetSecond_DatReg));
                        TimeAdjust_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(ClockTime_Nanosecond_DatReg) + (unsigned(OffsetNanosecond_DatReg) + (2*ClockPeriod_Gen)), NanosecondWidth_Con));
                    end if;
                end if;
            else
                if (unsigned(OffsetNanosecond_DatReg) > 0) then
                    if (((unsigned(OffsetNanosecondOrigin_DatReg) /= 0) and (unsigned(OffsetInterval_DatReg) /= 0)) and
                        ((unsigned(OffsetCount_CntReg) + unsigned(OffsetNanosecondOrigin_DatReg)) >= unsigned(OffsetInterval_DatReg))) then
                        OffsetCount_CntReg <= std_logic_vector(resize(unsigned(OffsetCount_CntReg) + unsigned(OffsetNanosecondOrigin_DatReg) - unsigned(OffsetInterval_DatReg), NanosecondWidth_Con));
                        OffsetNanosecond_DatReg <= std_logic_vector(unsigned(OffsetNanosecond_DatReg)- 1);
                    else
                        OffsetCount_CntReg <= std_logic_vector(resize(unsigned(OffsetCount_CntReg) + unsigned(OffsetNanosecondOrigin_DatReg), NanosecondWidth_Con));
                    end if;
                else
                    OffsetCount_CntReg <= (others => '0');
                    OffsetInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
                    OffsetSecond_DatReg <= (others => '0');
                    OffsetNanosecond_DatReg <= (others => '0');
                    OffsetNanosecondOrigin_DatReg <= (others => '0');
                    OffsetSign_DatReg <= '0';
                end if;
            end if;

            --calc drift count
            if ((unsigned(DriftCount_CntReg) + unsigned(DriftNanosecond_DatReg)) >= unsigned(DriftInterval_DatReg)) then
                DriftCount_CntReg <= std_logic_vector(resize((unsigned(DriftCount_CntReg) + unsigned(DriftNanosecond_DatReg) - unsigned(DriftInterval_DatReg)), AdjustmentIntervalWidth_Con));
            else
                DriftCount_CntReg <= std_logic_vector(resize((unsigned(DriftCount_CntReg) + unsigned(DriftNanosecond_DatReg)), AdjustmentIntervalWidth_Con));
            end if;

            -- Calc Offset interval
            OffsetAdjustmentMux_ValReg <= OffsetAdjustmentMux_Val;
            StartCalcOffsetInterval_EvtReg <= '0';
            if ((OffsetAdjustmentMux_Val = '1') and (OffsetAdjustmentMux_ValReg = '0')) then  -- edge triggered adjustment inputs
                OffsetAdjustmentMux_Second_DatReg <= OffsetAdjustmentMux_Second_Dat;
                OffsetAdjustmentMux_Nanosecond_DatReg <= OffsetAdjustmentMux_Nanosecond_Dat;
                OffsetAdjustmentMux_Sign_DatReg <= OffsetAdjustmentMux_Sign_Dat;
                OffsetAdjustmentMux_Interval_DatReg <= OffsetAdjustmentMux_Interval_Dat;
                StartCalcOffsetInterval_EvtReg <= '1';
                CalcOffsetInterval_StaReg <= '0';
            elsif (StartCalcOffsetInterval_EvtReg = '1') then
                OffsetAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto (AdjustmentIntervalWidth_Con)) <= (others => '0');
                OffsetAdjustmentMux_IntervalExtend_DatReg((AdjustmentIntervalWidth_Con-1) downto 0)  <= OffsetAdjustmentMux_Interval_DatReg;
                OffsetAdjustmentMux_PeriodExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto (AdjustmentIntervalWidth_Con)) <= std_logic_vector(to_unsigned(ClockPeriod_Gen,AdjustmentIntervalWidth_Con));
                OffsetAdjustmentMux_PeriodExtend_DatReg((AdjustmentIntervalWidth_Con-1) downto 0) <= (others => '0');
                OffsetAdjustmentMux_IntervalTicks_DatVar := (others => '0');
                CalcOffsetIntervalStep_CntReg <= AdjustmentIntervalWidth_Con-1;
                CalcOffsetInterval_StaReg <= '1';
            elsif (CalcOffsetInterval_StaReg = '1') then
                if (unsigned(OffsetAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0') >= unsigned(OffsetAdjustmentMux_PeriodExtend_DatReg)) then
                    OffsetAdjustmentMux_IntervalExtend_DatReg <= std_logic_vector(resize(unsigned(OffsetAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2)  downto 0) & '0') - unsigned(OffsetAdjustmentMux_PeriodExtend_DatReg), (2*AdjustmentIntervalWidth_Con)));
                    OffsetAdjustmentMux_IntervalTicks_DatVar(CalcOffsetIntervalStep_CntReg) := '1';
                else
                    OffsetAdjustmentMux_IntervalExtend_DatReg <= (OffsetAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2)  downto 0) & '0');
                    OffsetAdjustmentMux_IntervalTicks_DatVar(CalcOffsetIntervalStep_CntReg) := '0';
                end if;

                if (CalcOffsetIntervalStep_CntReg > 0) then
                    CalcOffsetIntervalStep_CntReg <= CalcOffsetIntervalStep_CntReg - 1;
                else
                    CalcOffsetInterval_StaReg <= '0'; -- recalculate offset interval only when a new offset is received
                    OffsetSecond_DatReg <= OffsetAdjustmentMux_Second_DatReg;
                    OffsetNanosecond_DatReg <= OffsetAdjustmentMux_Nanosecond_DatReg;
                    OffsetSign_DatReg <= OffsetAdjustmentMux_Sign_DatReg;
                    OffsetCount_CntReg <= (others => '0');
                    if ((unsigned(OffsetAdjustmentMux_Second_DatReg) /= 0) or
                        (unsigned(OffsetAdjustmentMux_Nanosecond_DatReg) >= unsigned(OffsetAdjustmentMux_IntervalTicks_DatVar))) then -- too big to be corrected smoothly
                        OffsetInterval_DatReg <= (others => '0'); -- so that we don't fall in the smooth correction
                        OffsetNanosecondOrigin_DatReg<= (others => '0');
                    else
                        OffsetInterval_DatReg <= OffsetAdjustmentMux_IntervalTicks_DatVar;
                        OffsetNanosecondOrigin_DatReg <= OffsetAdjustmentMux_Nanosecond_DatReg;
                    end if;
                end if;
            end if;

            -- Time Adjust register
            TimeAdjustmentMux_ValReg <= TimeAdjustmentMux_Val;
            if ((TimeAdjustmentMux_Val = '1') and (TimeAdjustmentMux_ValReg = '0')) then -- edge triggered adjustment inputs
                TimeAdjust_Second_DatReg <= TimeAdjustmentMux_Second_Dat;
                TimeAdjust_Nanosecond_DatReg <= TimeAdjustmentMux_Nanosecond_Dat;
                TimeAdjust_ValReg <= '1';
            end if;

            -- Calc Drift interval
            DriftAdjustmentMux_ValReg <= DriftAdjustmentMux_Val;
            StartCalcDriftInterval_EvtReg  <= '0';
            if ((DriftAdjustmentMux_Val = '1') and (DriftAdjustmentMux_ValReg = '0')) then  -- edge triggered adjustment inputs
                if (SelectInput_Dat = 254) then -- bypass sum
                    DriftAdjustmentMux_Nanosecond_DatReg <= DriftAdjustmentMux_Nanosecond_Dat;
                    DriftAdjustmentMux_Sign_DatReg <= DriftAdjustmentMux_Sign_Dat;
                else -- continuously accumulate the drift
                    if (DriftAdjustmentMux_Sign_Dat = DriftAdjustmentOld_Sign_DatReg) then -- both negative or both positive
                        DriftAdjustmentMux_Sign_DatReg <= DriftAdjustmentMux_Sign_Dat;
                        if ((unsigned(DriftAdjustmentMux_Nanosecond_Dat) + unsigned(DriftAdjustmentOld_Nanosecond_DatReg)) < SecondNanoseconds_Con) then
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(unsigned(DriftAdjustmentOld_Nanosecond_DatReg) + unsigned(DriftAdjustmentMux_Nanosecond_Dat));
                        else
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(to_unsigned((SecondNanoseconds_Con - 1), AdjustmentIntervalWidth_Con)); --max value
                        end if;
                    elsif ((DriftAdjustmentMux_Sign_Dat = '1') and (DriftAdjustmentOld_Sign_DatReg = '0')) then
                        if (unsigned(DriftAdjustmentMux_Nanosecond_Dat) >= unsigned(DriftAdjustmentOld_Nanosecond_DatReg)) then
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(unsigned(DriftAdjustmentMux_Nanosecond_Dat) - unsigned(DriftAdjustmentOld_Nanosecond_DatReg));
                            DriftAdjustmentMux_Sign_DatReg <= '1';
                        else
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(unsigned(DriftAdjustmentOld_Nanosecond_DatReg) - unsigned(DriftAdjustmentMux_Nanosecond_Dat));
                            DriftAdjustmentMux_Sign_DatReg <= '0';
                        end if;
                    elsif ((DriftAdjustmentMux_Sign_Dat = '0') and (DriftAdjustmentOld_Sign_DatReg = '1')) then
                        if (unsigned(DriftAdjustmentMux_Nanosecond_Dat) >= unsigned(DriftAdjustmentOld_Nanosecond_DatReg)) then
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(unsigned(DriftAdjustmentMux_Nanosecond_Dat) - unsigned(DriftAdjustmentOld_Nanosecond_DatReg));
                            DriftAdjustmentMux_Sign_DatReg <= '0';
                        else
                            DriftAdjustmentMux_Nanosecond_DatReg <= std_logic_vector(unsigned(DriftAdjustmentOld_Nanosecond_DatReg) - unsigned(DriftAdjustmentMux_Nanosecond_Dat));
                            DriftAdjustmentMux_Sign_DatReg <= '1';
                        end if;
                    end if;
                end if;
                DriftAdjustmentMux_Interval_DatReg <= DriftAdjustmentMux_Interval_Dat;
                StartCalcDriftInterval_EvtReg <= '1';
                CalcDriftInterval_StaReg <= '0';
            elsif (StartCalcDriftInterval_EvtReg = '1') then
                DriftAdjustmentOld_Nanosecond_DatReg <= DriftAdjustmentMux_Nanosecond_DatReg; --save the value
                DriftAdjustmentOld_Sign_DatReg <= DriftAdjustmentMux_Sign_DatReg;
                DriftAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto (AdjustmentIntervalWidth_Con)) <= (others => '0');
                DriftAdjustmentMux_IntervalExtend_DatReg((AdjustmentIntervalWidth_Con-1) downto 0)  <= DriftAdjustmentMux_Interval_DatReg;
                DriftAdjustmentMux_PeriodExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto (AdjustmentIntervalWidth_Con)) <= std_logic_vector(to_unsigned(ClockPeriod_Gen,AdjustmentIntervalWidth_Con));
                DriftAdjustmentMux_PeriodExtend_DatReg((AdjustmentIntervalWidth_Con-1) downto 0) <= (others => '0');
                DriftAdjustmentMux_IntervalTicks_DatVar := (others => '0');
                CalcDriftIntervalStep_CntReg <= AdjustmentIntervalWidth_Con - 1;
                CalcDriftInterval_StaReg <= '1';
            elsif (CalcDriftInterval_StaReg = '1') then
                if (unsigned(DriftAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0') >= unsigned(DriftAdjustmentMux_PeriodExtend_DatReg)) then
                    DriftAdjustmentMux_IntervalExtend_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0') - unsigned(DriftAdjustmentMux_PeriodExtend_DatReg), (2*AdjustmentIntervalWidth_Con)));
                    DriftAdjustmentMux_IntervalTicks_DatVar(CalcDriftIntervalStep_CntReg) := '1';
                else
                    DriftAdjustmentMux_IntervalExtend_DatReg <= (DriftAdjustmentMux_IntervalExtend_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0');
                    DriftAdjustmentMux_IntervalTicks_DatVar(CalcDriftIntervalStep_CntReg) := '0';
                end if;

                if (CalcDriftIntervalStep_CntReg > 0) then
                    CalcDriftIntervalStep_CntReg <= CalcDriftIntervalStep_CntReg - 1;
                else
                    CalcDriftInterval_StaReg <= '0'; -- recalculate drift interval only when a new drift is received
                    DriftCount_CntReg <= (others => '0');
                    DriftInterval_DatReg <= DriftAdjustmentMux_IntervalTicks_DatVar;
                    if (unsigned(DriftAdjustmentMux_Nanosecond_DatReg) > unsigned(DriftAdjustmentMux_IntervalTicks_DatVar)) then -- limit the correction to the max value
                        DriftNanosecond_DatReg <= DriftAdjustmentMux_IntervalTicks_DatVar;
                    else
                        DriftNanosecond_DatReg <= DriftAdjustmentMux_Nanosecond_DatReg;
                    end if;
                    DriftSign_DatReg <= DriftAdjustmentMux_Sign_DatReg;
                end if;
            end if;

            if (Enable_Ena = '0') then --equivalent to reset
                DriftAdjustmentMux_ValReg <= '0';
                DriftAdjustmentMux_Nanosecond_DatReg <= (others => '0');
                DriftAdjustmentMux_Sign_DatReg <= '0';
                DriftAdjustmentOld_Nanosecond_DatReg <= (others => '0');
                DriftAdjustmentOld_Sign_DatReg <= '0';
                DriftAdjustmentMux_Interval_DatReg <= (others => '0');
                DriftAdjustmentMux_IntervalExtend_DatReg <= (others => '0');
                DriftAdjustmentMux_PeriodExtend_DatReg <= (others => '0');
                DriftAdjustmentMux_IntervalTicks_DatVar := (others => '0');
                StartCalcDriftInterval_EvtReg <= '0';
                CalcDriftInterval_StaReg <= '0';
                DriftCount_CntReg <= (others => '0');
                DriftInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
                DriftNanosecond_DatReg <= (others => '0');
                DriftSign_DatReg <= '0';

                TimeAdjustmentMux_ValReg <= '0';

                OffsetAdjustmentMux_ValReg <= '0';
                OffsetAdjustmentMux_Second_DatReg <= (others => '0');
                OffsetAdjustmentMux_Nanosecond_DatReg <= (others => '0');
                OffsetAdjustmentMux_Sign_DatReg <= '0';
                OffsetAdjustmentMux_Interval_DatReg <= (others => '0');
                OffsetAdjustmentMux_IntervalExtend_DatReg <= (others => '0');
                OffsetAdjustmentMux_PeriodExtend_DatReg <= (others => '0');
                OffsetAdjustmentMux_IntervalTicks_DatVar := (others => '0');
                StartCalcOffsetInterval_EvtReg <= '0';
                CalcOffsetInterval_StaReg <= '0';
                OffsetCount_CntReg <= (others => '0');
                OffsetInterval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con/ClockPeriod_Gen, AdjustmentIntervalWidth_Con));
                OffsetSecond_DatReg <= (others => '0');
                OffsetNanosecond_DatReg <= (others => '0');
                OffsetNanosecondOrigin_DatReg <= (others => '0');
                OffsetSign_DatReg <= '0';

                TimeAdjust_Second_DatReg <= (others => '0');
                TimeAdjust_Nanosecond_DatReg <= (others => '0');
                TimeAdjust_ValReg <= '0';

                CountAdjust_Nanosecond_DatReg <= (others => '0');
                CountAdjust_Sign_DatReg <= '0';
                CountAdjust_ValReg <= '0';
            end if;
        end if;
    end process Adjust_Prc;

    -- The process provides the adjustable clock ClockTime. At each system clock cycle the time increases by the period of the system clock,
    -- unless an adjustment has to be applied. In this case the time increases by the "adjusted" period of the system clock (max +/- 2ns).
    ClockTime_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            ClockTime_Second_DatReg <= (others => '0');
            ClockTime_Nanosecond_DatReg <= (others => '0');
            ClockTime_TimeJump_DatReg <= '0';
            ClockTime_ValReg <= '0';
            ClockIncrement_DatReg <= (others => '0');

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClockTime_ValReg <= '1';

            if (TimeAdjust_ValReg = '1') then -- hard set time
                ClockTime_Second_DatReg <= TimeAdjust_Second_DatReg;
                ClockTime_Nanosecond_DatReg <= TimeAdjust_Nanosecond_DatReg;
                ClockTime_TimeJump_DatReg <= '1';
            else
                if ((unsigned(ClockTime_Nanosecond_DatReg) + unsigned(ClockIncrement_DatReg)) >= SecondNanoseconds_Con) then
                    ClockTime_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) + 1);
                    ClockTime_Nanosecond_DatReg <= std_logic_vector((unsigned(ClockTime_Nanosecond_DatReg) + unsigned(ClockIncrement_DatReg)) - SecondNanoseconds_Con);
                else
                    ClockTime_Nanosecond_DatReg <= std_logic_vector(unsigned(ClockTime_Nanosecond_DatReg) + unsigned(ClockIncrement_DatReg));
                end if;
                ClockTime_TimeJump_DatReg <= '0';
            end if;

            if (CountAdjust_ValReg = '1') then
                if CountAdjust_Sign_DatReg = '1' then
                    ClockIncrement_DatReg  <= std_logic_vector(to_unsigned(ClockPeriod_Gen, ClockIncrementWidth_Con) - unsigned(CountAdjust_Nanosecond_DatReg));
                else
                    ClockIncrement_DatReg  <= std_logic_vector(to_unsigned(ClockPeriod_Gen, ClockIncrementWidth_Con) + unsigned(CountAdjust_Nanosecond_DatReg));
                end if;
            else
                ClockIncrement_DatReg  <= std_logic_vector(to_unsigned(ClockPeriod_Gen, ClockIncrementWidth_Con));
            end if;

            if (Enable_Ena = '0') then
                ClockTime_Second_DatReg <= (others => '0');
                ClockTime_Nanosecond_DatReg <= (others => '0');
                ClockTime_TimeJump_DatReg <= '0';
                ClockTime_ValReg <= '0';
                ClockIncrement_DatReg <= (others => '0');
            end if;
        end if;
    end process ClockTime_Prc;

    -- The process provides the status flags of the adjustable clock
    -- The InSync flag is activated, if for 4 consecutive offset adjustments the corrections are less than a predefined threshold.
    -- The Insync flag is deactivated, if an offset adjustment bigger than the threshold is received or if a time adjustment is applied (e.g. time jump) or if the clock is disabled.
    -- The InHoldover flag is activated, if the adjustable clock has been InSync and an offset adjustment has not been received for a predefined threshold time
    -- The InHoldover flag is deactivated, if the adjustable clock goes out of sync or if a time or offset adjustment is received or if the clock is disabled
    InSyncOffset_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            InSync_DatReg <= '0';
            InHoldover_DatReg <= '0';
            ClockTimeB0_DatReg <= '0';
            Holdover_CntReg <= 0;
            OffsetArray_DatReg <= (others => '0');

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClockTimeB0_DatReg <= ClockTime_Second_DatReg(0);

            -- count holdover, will be reset on every adjustment
            if ((ClockTime_TimeJump_DatReg = '1') or (ClockTime_ValReg = '0')) then
                Holdover_CntReg <= 0;
            elsif ((ClockTimeB0_DatReg /=ClockTime_Second_DatReg(0)) and (Holdover_CntReg < ClockInHoldoverTimeoutSecond_Gen)) then -- on second overflow
                Holdover_CntReg <= Holdover_CntReg + 1;
            end if;

            if ((TimeAdjustmentMux_Val = '1') and (TimeAdjustmentMux_ValReg = '0')) then
                OffsetArray_DatReg <= OffsetArray_DatReg(2 downto 0) & '0'; -- this will make sure the in sync flag will go low
                Holdover_CntReg <= 0;
            elsif ((OffsetAdjustmentMux_Val = '1') and (OffsetAdjustmentMux_ValReg = '0')) then
                if ((unsigned(OffsetAdjustmentMux_Second_Dat) = 0) and
                    (unsigned(OffsetAdjustmentMux_Nanosecond_Dat) <= unsigned(ClockInSyncThreshold_DatReg))) then
                    OffsetArray_DatReg <= OffsetArray_DatReg(2 downto 0) & '1';
                else
                    OffsetArray_DatReg <= OffsetArray_DatReg(2 downto 0) & '0';
                end if;
                Holdover_CntReg <= 0;
            end if;

            -- check the last 4 offset calculations
            if (OffsetArray_DatReg = "1111") then
                InSync_DatReg <= '1';
                if (Holdover_CntReg = ClockInHoldoverTimeoutSecond_Gen) then -- only if in sync we go to holdover
                    InHoldover_DatReg <= '1';
                else
                    InHoldover_DatReg <= '0';
                end if;
            else
                InSync_DatReg <= '0';
                InHoldover_DatReg <= '0';
            end if;

            if (Enable_Ena = '0') then
                InSync_DatReg <= '0';
                InHoldover_DatReg <= '0';
                OffsetArray_DatReg <= (others => '0');
                Holdover_CntReg <= 0;
            end if;
        end if;
    end process InSyncOffset_Prc;

    -- Read and Write AXI access of the registers
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress                : std_logic_vector(31 downto 0) := (others => '0');
    begin
        if (SysRstN_RstIn = '0') then
            AxiWriteAddrReady_RdyReg <= '0';
            AxiWriteDataReady_RdyReg <= '0';

            AxiWriteRespValid_ValReg <= '0';
            AxiWriteRespResponse_DatReg <= (others => '0');

            AxiReadAddrReady_RdyReg <= '0';

            AxiReadDataValid_ValReg <= '0';
            AxiReadDataResponse_DatReg <= (others => '0');
            AxiReadDataData_DatReg <= (others => '0');

            Axi_AccessState_StaReg <= Axi_AccessState_Type_Rst_Con;

            Axi_Init_Proc(ClockControl_Reg_Con, ClockControl_DatReg);
            Axi_Init_Proc(ClockStatus_Reg_Con, ClockStatus_DatReg);

            ClockSelect_DatReg <= (others => '0');
            Axi_Init_Proc(ClockSelect_Reg_Con, ClockSelect_DatReg);
            ClockSelect_DatReg(31 downto 0) <= (others => '0');
            Axi_Init_Proc(ClockVersion_Reg_Con, ClockVersion_DatReg);
            Axi_Init_Proc(ClockTimeValueL_Reg_Con, ClockTimeValueL_DatReg);
            Axi_Init_Proc(ClockTimeValueH_Reg_Con, ClockTimeValueH_DatReg);
            Axi_Init_Proc(ClockTimeAdjValueL_Reg_Con, ClockTimeAdjValueL_DatReg);
            Axi_Init_Proc(ClockTimeAdjValueH_Reg_Con, ClockTimeAdjValueH_DatReg);
            Axi_Init_Proc(ClockOffsetAdjValue_Reg_Con, ClockOffsetAdjValue_DatReg);
            Axi_Init_Proc(ClockOffsetAdjInterval_Reg_Con, ClockOffsetAdjInterval_DatReg);
            Axi_Init_Proc(ClockDriftAdjValue_Reg_Con, ClockDriftAdjValue_DatReg);
            Axi_Init_Proc(ClockDriftAdjInterval_Reg_Con, ClockDriftAdjInterval_DatReg);
            Axi_Init_Proc(ClockInSyncThreshold_Reg_Con, ClockInSyncThreshold_DatReg);
            ClockInSyncThreshold_DatReg <= std_logic_vector(to_unsigned(ClockInSyncThreshold_Gen, 32));
            Axi_Init_Proc(ClockServoOffsetFactorP_Reg_Con, ClockServoOffsetFactorP_DatReg);
            Axi_Init_Proc(ClockServoOffsetFactorI_Reg_Con, ClockServoOffsetFactorI_DatReg);
            Axi_Init_Proc(ClockServoDriftFactorP_Reg_Con, ClockServoDriftFactorP_DatReg);
            Axi_Init_Proc(ClockServoDriftFactorI_Reg_Con, ClockServoDriftFactorI_DatReg);
            ClockServoOffsetFactorP_DatReg <= OffsetFactorP_Con;
            ClockServoOffsetFactorI_DatReg <= OffsetFactorI_Con;
            ClockServoDriftFactorP_DatReg <= DriftFactorP_Con;
            ClockServoDriftFactorI_DatReg <= DriftFactorI_Con;
            Axi_Init_Proc(ClockStatusOffset_Reg_Con, ClockStatusOffset_DatReg);
            Axi_Init_Proc(ClockStatusDrift_Reg_Con, ClockStatusDrift_DatReg);

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClockSelect_DatReg(15 downto 0) <= ClockSelect_DatReg(15 downto 0) and ClockSelect_Reg_Con.Mask(15 downto 0); -- make sure it is in a defined range

            if ((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) then
                AxiWriteAddrReady_RdyReg <= '0';
            end if;

            if ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1')) then
                AxiWriteDataReady_RdyReg <= '0';
            end if;

            if ((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) then
                AxiWriteRespValid_ValReg <= '0';
            end if;

            if ((AxiReadAddrValid_ValIn = '1') and (AxiReadAddrReady_RdyReg = '1')) then
                AxiReadAddrReady_RdyReg <= '0';
            end if;

            if ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1')) then
                AxiReadDataValid_ValReg <= '0';
            end if;

            case (Axi_AccessState_StaReg) is
                when Idle_St =>
                if ((AxiWriteAddrValid_ValIn = '1') and (AxiWriteDataValid_ValIn = '1')) then
                        AxiWriteAddrReady_RdyReg <= '1';
                        AxiWriteDataReady_RdyReg <= '1';
                        Axi_AccessState_StaReg <= Write_St;
                    elsif (AxiReadAddrValid_ValIn = '1') then
                        AxiReadAddrReady_RdyReg <= '1';
                        Axi_AccessState_StaReg <= Read_St;
                    end if;
                when Read_St =>
                    if ((AxiReadAddrValid_ValIn = '1') and (AxiReadAddrReady_RdyReg = '1')) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiReadAddrAddress_AdrIn), 32));
                        AxiReadDataValid_ValReg <= '1';
                        AxiReadDataResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Read_Proc(ClockControl_Reg_Con, ClockControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockStatus_Reg_Con, ClockStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockSelect_Reg_Con, ClockSelect_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockVersion_Reg_Con, ClockVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockTimeValueL_Reg_Con, ClockTimeValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockTimeValueH_Reg_Con, ClockTimeValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockTimeAdjValueL_Reg_Con, ClockTimeAdjValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockTimeAdjValueH_Reg_Con, ClockTimeAdjValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockOffsetAdjValue_Reg_Con, ClockOffsetAdjValue_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockOffsetAdjInterval_Reg_Con, ClockOffsetAdjInterval_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockDriftAdjValue_Reg_Con, ClockDriftAdjValue_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockDriftAdjInterval_Reg_Con, ClockDriftAdjInterval_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockInSyncThreshold_Reg_Con, ClockInSyncThreshold_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockServoOffsetFactorP_Reg_Con, ClockServoOffsetFactorP_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockServoOffsetFactorI_Reg_Con, ClockServoOffsetFactorI_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockServoDriftFactorP_Reg_Con, ClockServoDriftFactorP_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockServoDriftFactorI_Reg_Con, ClockServoDriftFactorI_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockStatusOffset_Reg_Con, ClockStatusOffset_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClockStatusDrift_Reg_Con, ClockStatusDrift_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(ClockControl_Reg_Con, ClockControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockStatus_Reg_Con, ClockStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockSelect_Reg_Con, ClockSelect_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockVersion_Reg_Con, ClockVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockTimeValueL_Reg_Con, ClockTimeValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockTimeValueH_Reg_Con, ClockTimeValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockTimeAdjValueL_Reg_Con, ClockTimeAdjValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockTimeAdjValueH_Reg_Con, ClockTimeAdjValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockOffsetAdjValue_Reg_Con, ClockOffsetAdjValue_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockOffsetAdjInterval_Reg_Con, ClockOffsetAdjInterval_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockDriftAdjValue_Reg_Con, ClockDriftAdjValue_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockDriftAdjInterval_Reg_Con, ClockDriftAdjInterval_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockInSyncThreshold_Reg_Con, ClockInSyncThreshold_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockServoOffsetFactorP_Reg_Con, ClockServoOffsetFactorP_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockServoOffsetFactorI_Reg_Con, ClockServoOffsetFactorI_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockServoDriftFactorP_Reg_Con, ClockServoDriftFactorP_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockServoDriftFactorI_Reg_Con, ClockServoDriftFactorI_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockStatusOffset_Reg_Con, ClockStatusOffset_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClockStatusDrift_Reg_Con, ClockStatusDrift_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                       ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;
                when others =>
            end case;

            ClockStatus_DatReg(ClockStatus_InSyncBit_Con) <= InSync_DatReg;
            ClockStatus_DatReg(ClockStatus_InHoldoverBit_Con) <= InHoldover_DatReg;

            ClockSelect_DatReg(31 downto 16) <= ClockSelect_DatReg(15 downto 0) and ClockSelect_Reg_Con.Mask(31 downto 16);

            -- Autoclear
            if (ClockControl_DatReg(ClockControl_TimeAdjValBit_Con) = '1') then
                ClockControl_DatReg(ClockControl_TimeAdjValBit_Con) <= '0';
            end if;

            if (ClockControl_DatReg(ClockControl_OffsetAdjValBit_Con) = '1') then
                ClockControl_DatReg(ClockControl_OffsetAdjValBit_Con) <= '0';
            end if;

            if (ClockControl_DatReg(ClockControl_DriftAdjValBit_Con) = '1') then
                ClockControl_DatReg(ClockControl_DriftAdjValBit_Con) <= '0';
            end if;

            if (ClockControl_DatReg(ClockControl_ServoValBit_Con) = '1') then
                ClockControl_DatReg(ClockControl_ServoValBit_Con) <= '0';
            end if;

            if (ClockControl_DatReg(ClockControl_TimeReadValBit_Con) = '1') then
                ClockControl_DatReg(ClockControl_TimeReadValBit_Con)  <= '0';
                ClockControl_DatReg(ClockControl_TimeReadDValBit_Con) <= '1';
                ClockTimeValueL_DatReg <= ClockTime_Nanosecond_DatReg;
                ClockTimeValueH_DatReg <= ClockTime_Second_DatReg;
            end if;

            if (OffsetAdjustmentMux_Val = '1') then
                ClockStatusOffset_DatReg(31) <= OffsetAdjustmentMux_Sign_Dat;
                ClockStatusOffset_DatReg(30 downto 0) <= OffsetAdjustmentMux_Nanosecond_Dat(30 downto 0);
            end if;

            if (DriftAdjustmentMux_ValReg = '1') then  -- provide the accumulated drift
                ClockStatusDrift_DatReg(31) <= DriftAdjustmentMux_Sign_DatReg;
                ClockStatusDrift_DatReg(30 downto 0) <= DriftAdjustmentMux_Nanosecond_DatREg(30 downto 0);
            end if;

        end if;
    end process Axi_Prc;

end architecture AdjustableClock_Arch;