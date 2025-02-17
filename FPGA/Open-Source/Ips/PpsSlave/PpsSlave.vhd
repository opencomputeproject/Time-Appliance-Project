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
-- Timestamp the incoming event and evaluate its period and pulse width, If it is        --
-- accepted as PPS input, calculate its offset and drift, which are sent to the          --
-- corresponding PI servos.                                                              --
-------------------------------------------------------------------------------------------
entity PpsSlave is
    generic (
        ClockPeriod_Gen                             :       natural := 20;  
        CableDelay_Gen                              :       boolean := false;
        InputDelay_Gen                              :       natural := 0;             
        InputPolarity_Gen                           :       boolean := true;        
        HighResFreqMultiply_Gen                     :       natural range 4 to 10 := 5;

        DriftMulP_Gen                               :       natural range 0 to 1024 := 3;
        DriftDivP_Gen                               :       natural range 0 to 1024 := 4;
        DriftMulI_Gen                               :       natural range 0 to 1024 := 3;
        DriftDivI_Gen                               :       natural range 0 to 1024 := 16;
        OffsetMulP_Gen                              :       natural range 0 to 1024 := 3;
        OffsetDivP_Gen                              :       natural range 0 to 1024 := 4;
        OffsetMulI_Gen                              :       natural range 0 to 1024 := 3;
        OffsetDivI_Gen                              :       natural range 0 to 1024 := 16;

        Sim_Gen                                     :       boolean := false
    );
    port (
        -- System
        SysClk_ClkIn                                : in    std_logic;
        SysClkNx_ClkIn                              : in    std_logic := '0';
        SysRstN_RstIn                               : in    std_logic;

        -- Time Input
        ClockTime_Second_DatIn                      : in    std_logic_vector((SecondWidth_Con-1) downto 0);
        ClockTime_Nanosecond_DatIn                  : in    std_logic_vector((NanosecondWidth_Con-1) downto 0);
        ClockTime_TimeJump_DatIn                    : in    std_logic;
        ClockTime_ValIn                             : in    std_logic;

        -- Pps Input
        Pps_EvtIn                                   : in    std_logic;

        -- Servo Parameters
        Servo_ValIn                                 : in    std_logic := '0';
        ServoOffsetFactorP_DatIn                    : in    std_logic_vector(31 downto 0) := (others => '0');
        ServoOffsetFactorI_DatIn                    : in    std_logic_vector(31 downto 0) := (others => '0');
        ServoDriftFactorP_DatIn                     : in    std_logic_vector(31 downto 0) := (others => '0');
        ServoDriftFactorI_DatIn                     : in    std_logic_vector(31 downto 0) := (others => '0');

        -- Offset Adjustment Output
        OffsetAdjustment_Second_DatOut              : out   std_logic_vector((SecondWidth_Con-1) downto 0);
        OffsetAdjustment_Nanosecond_DatOut          : out   std_logic_vector((NanosecondWidth_Con-1) downto 0);
        OffsetAdjustment_Sign_DatOut                : out   std_logic;
        OffsetAdjustment_Interval_DatOut            : out   std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
        OffsetAdjustment_ValOut                     : out   std_logic;

        -- Drift Adjustment Output
        DriftAdjustment_Nanosecond_DatOut           : out   std_logic_vector((NanosecondWidth_Con-1) downto 0);
        DriftAdjustment_Sign_DatOut                 : out   std_logic;
        DriftAdjustment_Interval_DatOut             : out   std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0);
        DriftAdjustment_ValOut                      : out   std_logic;

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
end entity PpsSlave;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture PpsSlave_Arch of PpsSlave is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant ClkCyclesInMillisecond_Con             : natural := (1000000/ClockPeriod_Gen);
    constant PeriodWindow_Con                       : natural range 0 to 500 := 100; -- in milliseconds
    constant PulseWidthMin_Con                      : natural range 0 to 1000 := 1; -- in milliseconds
    constant PulseWidthMax_Con                      : natural range 0 to 1000 := 999; -- in milliseconds
    constant WaitTimer_Con                          : natural range 0 to 16 := 16;
    
    constant ClkCyclesInSecond_Con                  : natural := (SecondNanoseconds_Con/ClockPeriod_Gen);
    -- Offset and Drift factors
    constant FactorSize_Con                         : natural := (AdjustmentIntervalWidth_Con + 12);
    constant OffsetFactorP_Con                      : unsigned((FactorSize_Con-1) downto 0) := to_unsigned(((OffsetMulP_Gen*(2**16))/OffsetDivP_Gen), FactorSize_Con);
    constant OffsetFactorI_Con                      : unsigned((FactorSize_Con-1) downto 0) := to_unsigned(((OffsetMulI_Gen*(2**16))/OffsetDivI_Gen), FactorSize_Con);
    constant DriftFactorP_Con                       : unsigned((FactorSize_Con-1) downto 0) := to_unsigned(((DriftMulP_Gen*(2**16))/DriftDivP_Gen), FactorSize_Con);
    constant DriftFactorI_Con                       : unsigned((FactorSize_Con-1) downto 0) := to_unsigned(((DriftMulI_Gen*(2**16))/DriftDivI_Gen), FactorSize_Con);
    
    constant IntegralMax_Con                        : unsigned((FactorSize_Con-1) downto 0) := (others => '1');

    -- PPS Slave version
    constant PpsSlaveMajorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant PpsSlaveMinorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant PpsSlaveBuildVersion_Con               : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant PpsSlaveVersion_Con                    : std_logic_vector(31 downto 0) := PpsSlaveMajorVersion_Con & PpsSlaveMinorVersion_Con & PpsSlaveBuildVersion_Con;

    -- AXI registers
    constant PpsSlaveControl_Reg_Con                : Axi_Reg_Type:= (x"00000000", x"00000003", Rw_E, x"00000000");
    constant PpsSlaveStatus_Reg_Con                 : Axi_Reg_Type:= (x"00000004", x"00000003", Wc_E, x"00000000");
    constant PpsSlavePolarity_Reg_Con               : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant PpsSlaveVersion_Reg_Con                : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, PpsSlaveVersion_Con);
    constant PpsSlavePulseWidth_Reg_Con             : Axi_Reg_Type:= (x"00000010", x"000003FF", Ro_E, x"00000000");
    constant PpsSlaveCableDelay_Reg_Con             : Axi_Reg_Type:= (x"00000020", x"0000FFFF", Rw_E, x"00000000");
    
    constant PpsSlaveControl_EnableBit_Con          : natural := 0;
    constant PpsSlaveStatus_PeriodErrorBit_Con      : natural := 0;    
    constant PpsSlaveStatus_PulseWidthErrorBit_Con  : natural := 1;    
    constant PpsSlavePolarity_PolarityBit_Con       : natural := 0;
    
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type OffsetCalcState_Type                       is (WaitTimestamp_St, WaitDrift_St);
    type DriftCalcState_Type                        is (WaitTimestamp_St, SubOffsetOld_St, Diff_St, Normalize_Step1_St, Normalize_Step2_St, Normalize_Step3_St);

    type PI_OffsetState_Type                        is (Idle_St, P_St, I_St);
    type PI_DriftState_Type                         is (Idle_St, P_St, I_St, Check_St);
    
    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    -- configuration 
    signal Enable_Ena                               : std_logic;
    signal CableDelay_Dat                           : std_logic_vector(15 downto 0);
    signal Polarity_Dat                             : std_logic;
    
    -- Timestamp
    signal Timestamper_Evt                          : std_logic;
    signal Timestamp_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal Timestamp_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal Timestamp_ValReg                         : std_logic;
    signal RegisterDelay_DatReg                     : integer range 0 to (3*ClockPeriod_Gen);
                                                    
    -- Time Input           
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_ValReg                         : std_logic;
    
    -- High resolution timestamp
    signal TimestampSysClkNx1_EvtReg                : std_logic := '0';
    signal TimestampSysClkNx2_EvtReg                : std_logic := '0';
    signal TimestampSysClkNx_EvtShiftReg            : std_logic_vector(HighResFreqMultiply_Gen*2-1 downto 0) := (others => '0');
    signal TimestampSysClk1_EvtReg                  : std_logic := '0';
    signal TimestampSysClk2_EvtReg                  : std_logic := '0';
    signal TimestampSysClk3_EvtReg                  : std_logic := '0';
    signal TimestampSysClk4_EvtReg                  : std_logic := '0';
    signal TimestampSysClk_EvtShiftReg              : std_logic_vector(HighResFreqMultiply_Gen*2-1 downto 0) := (others => '0');

    -- Pulse width and period count in milliseconds
    signal MillisecondCounter_CntReg                : natural range 0 to (ClkCyclesInMillisecond_Con-1) := 0;
    signal NewMillisecond_DatReg                    : std_logic := '0';
    signal PulseWidthTimer_CntReg                   : natural range 0 to (1000 + PeriodWindow_Con + 1) := 0; 
    signal PulseWidth_DatReg                        : std_logic_vector(9 downto 0); 
    signal PeriodTimer_CntReg                       : natural range 0 to (1000 + PeriodWindow_Con + 1) := 0; 
    
    -- Pulse validation flags
    signal PulseStarted_ValReg                      : std_logic_vector(1 downto 0) := (others => '0');
    signal PeriodIsOk_ValReg                        : std_logic := '0';
    signal PeriodError_DatReg                       : std_logic := '0';
    signal PulseWidthError_DatReg                   : std_logic := '0';
    
    -- Offset calculation
    signal OffsetCalcState_StaReg                   : OffsetCalcState_Type;

    signal OffsetCalcActive_ValReg                  : std_logic;
    signal OffsetAdjustment_Second_DatReg           : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal OffsetAdjustment_Nanosecond_DatReg       : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal OffsetAdjustment_Sign_DatReg             : std_logic := '0';
    signal OffsetAdjustment_Interval_DatReg         : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
    signal OffsetAdjustment_ValReg                  : std_logic := '0';
    signal OffsetAdjustment_ValOldReg               : std_logic := '0';
    signal OffsetAdjustmentInvalid_ValReg           : std_logic := '0';
    
    signal WaitTimer_CntReg                         : natural range 0 to WaitTimer_Con := 0; -- in milliseconds

    -- Drift calculation                            
    signal DriftCalcState_StaReg                    : DriftCalcState_Type;

    signal DriftCalcActive_ValReg                   : std_logic;
    signal DriftAdjustment_Nanosecond_DatReg        : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal DriftAdjustment_Sign_DatReg              : std_logic := '0';
    signal DriftAdjustment_Interval_DatReg          : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
    signal DriftAdjustment_ValReg                   : std_logic := '0';
    signal DriftAdjustment_ValOldReg                : std_logic := '0';
    signal DriftAdjustmentInvalid_ValReg            : std_logic := '0';

    signal DriftAdjustmentDelta_Second_DatReg       : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal DriftAdjustmentDelta_Nanosecond_DatReg   : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal DriftAdjustmentDelta_Sign_DatReg         : std_logic := '0';
    signal DriftAdjustmentDelta_Interval_DatReg     : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');

    signal Timestamp_Second_DatOldReg               : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal Timestamp_Nanosecond_DatOldReg           : std_logic_vector((NanosecondWidth_Con-1) downto 0);

    -- Drift Normalizer 
    signal Normalizer1_DatReg                       : std_logic_vector((2*AdjustmentIntervalWidth_Con-1) downto 0);
    signal Normalizer1_Result_DatReg                : std_logic_vector((2*AdjustmentIntervalWidth_Con-1) downto 0);
    signal NormalizeActive1_ValReg                  : std_logic;
    signal NormalizeActive2_ValReg                  : std_logic;
    signal Step_CntReg                              : natural range 0 to (2*AdjustmentIntervalWidth_Con-1);
    signal NormalizeProduct_DatReg                  : std_logic_vector(((2*2*AdjustmentIntervalWidth_Con)-1) downto 0);
    signal Normalizer2_DatReg                       : std_logic_vector((2*2*AdjustmentIntervalWidth_Con-1) downto 0);
    signal Normalizer2_Result_DatReg                : std_logic_vector((2*AdjustmentIntervalWidth_Con-1) downto 0);

    -- PI Servo factors
    signal OffsetFactorP_DatReg                     : unsigned((FactorSize_Con-1) downto 0);
    signal OffsetFactorI_DatReg                     : unsigned((FactorSize_Con-1) downto 0);
    signal DriftFactorP_DatReg                      : unsigned((FactorSize_Con-1) downto 0);
    signal DriftFactorI_DatReg                      : unsigned((FactorSize_Con-1) downto 0);
    
    -- PI Offset Adjustment 
    signal PI_OffsetAdjustment_Second_DatReg        : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustment_Nanosecond_DatReg    : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustment_Sign_DatReg          : std_logic := '0';
    signal PI_OffsetAdjustment_Interval_DatReg      : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustment_ValReg               : std_logic := '0';

    signal PI_OffsetAdjustRetain_Second_DatReg      : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustRetain_Nanosecond_DatReg  : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustRetain_Sign_DatReg        : std_logic := '0';
    signal PI_OffsetAdjustRetain_Interval_DatReg    : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
    signal PI_OffsetAdjustRetain_ValReg             : std_logic := '0';
    
    -- PI Servo offset calculation 
    signal PI_OffsetState_StaReg                    : PI_OffsetState_Type;
    signal PI_OffsetIntegral_DatReg                 : unsigned((FactorSize_Con-1) downto 0);
    signal PI_OffsetIntegralSign_DatReg             : std_logic;
    signal PI_OffsetMul_DatReg                      : std_logic_vector(((2*FactorSize_Con)-1) downto 0);

    -- PI Drift Adjustment 
    signal PI_DriftAdjustment_Nanosecond_DatReg     : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal PI_DriftAdjustment_Sign_DatReg           : std_logic := '0';
    signal PI_DriftAdjustment_Interval_DatReg       : std_logic_vector((AdjustmentIntervalWidth_Con-1) downto 0) := (others => '0');
    signal PI_DriftAdjustment_ValReg                : std_logic := '0';

    -- PI Servo Drift calculation                  
    signal PI_DriftState_StaReg                     : PI_DriftState_Type;
    signal PI_DriftIntegral_DatReg                  : unsigned((FactorSize_Con-1) downto 0);
    signal PI_DriftIntegralSign_DatReg              : std_logic;
    signal PI_DriftMul_DatReg                       : std_logic_vector(((2*FactorSize_Con)-1) downto 0);

    -- Axi Regs
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;

    signal AxiWriteAddrReady_RdyReg                 : std_logic;       
    signal AxiWriteDataReady_RdyReg                 : std_logic;   
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);   
    signal AxiReadAddrReady_RdyReg                  : std_logic;          
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);

    signal PpsSlaveControl_DatReg                   : std_logic_vector(31 downto 0);
    signal PpsSlaveStatus_DatReg                    : std_logic_vector(31 downto 0);
    signal PpsSlavePolarity_DatReg                  : std_logic_vector(31 downto 0);
    signal PpsSlaveVersion_DatReg                   : std_logic_vector(31 downto 0);
    signal PpsSlavePulseWidth_DatReg                : std_logic_vector(31 downto 0);
    signal PpsSlaveCableDelay_DatReg                : std_logic_vector(31 downto 0);

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    -- configuration from AXI
    Enable_Ena                                      <= PpsSlaveControl_DatReg(PpsSlaveControl_EnableBit_Con);
    Polarity_Dat                                    <= PpsSlavePolarity_DatReg(PpsSlavePolarity_PolarityBit_Con);
    CableDelay_Dat                                  <= PpsSlaveCableDelay_DatReg(15 downto 0) when (CableDelay_Gen = true) else 
                                                       (others => '0');
    -- Fix to active-high polarity 
    Timestamper_Evt                                 <= Pps_EvtIn when (Polarity_Dat = '1') else
                                                       (not Pps_EvtIn);
    -- PI result assignments
    OffsetAdjustment_Second_DatOut                  <= PI_OffsetAdjustment_Second_DatReg;
    OffsetAdjustment_Nanosecond_DatOut              <= PI_OffsetAdjustment_Nanosecond_DatReg;
    OffsetAdjustment_Sign_DatOut                    <= PI_OffsetAdjustment_Sign_DatReg;
    OffsetAdjustment_Interval_DatOut                <= PI_OffsetAdjustment_Interval_DatReg;
    OffsetAdjustment_ValOut                         <= PI_OffsetAdjustment_ValReg;

    DriftAdjustment_Nanosecond_DatOut               <= PI_DriftAdjustment_Nanosecond_DatReg;
    DriftAdjustment_Sign_DatOut                     <= PI_DriftAdjustment_Sign_DatReg;
    DriftAdjustment_Interval_DatOut                 <= PI_DriftAdjustment_Interval_DatReg;
    DriftAdjustment_ValOut                          <= PI_DriftAdjustment_ValReg;

    -- AXI assignments
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
    -- Mark an input event at the shift register of the high resolution clock domain
    SysClkNxReg_Prc : process(SysClkNx_ClkIn) is
    begin
        if ((SysClkNx_ClkIn'event) and (SysClkNx_ClkIn = '1')) then 
            TimestampSysClkNx1_EvtReg <= Timestamper_Evt;
            TimestampSysClkNx2_EvtReg <= TimestampSysClkNx1_EvtReg;
            TimestampSysClkNx_EvtShiftReg <= TimestampSysClkNx_EvtShiftReg((HighResFreqMultiply_Gen*2-2) downto 0) & TimestampSysClkNx2_EvtReg;
        end if;
    end process SysClkNxReg_Prc;
    
    -- Copy the event shift register of the high resolution clock domain to the system clock domain
    SysClkReg_Prc : process(SysClk_ClkIn) is
    begin
        if ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then 
            TimestampSysClk1_EvtReg <= Timestamper_Evt;
            TimestampSysClk2_EvtReg <= TimestampSysClk1_EvtReg;
            TimestampSysClk3_EvtReg <= TimestampSysClk2_EvtReg;
            TimestampSysClk4_EvtReg <= TimestampSysClk3_EvtReg;
            TimestampSysClk_EvtShiftReg <= TimestampSysClkNx_EvtShiftReg;
        end if;
    end process SysClkReg_Prc;

    -- Calculate the timestamp by compensating for the delays:
    --    - the timestamping at the high resolution clock domain and the corresponding register delays for switching the clock domains
    --    - the input delay, which is provided as generic input
    --    - the cable delay, which is received by the AXI register (and enabled by a generic input)
    -- Validate the input pulse's period and width by counting their duration in milliseconds.
    Timestamp_Prc: process(SysClk_ClkIn, SysRstN_RstIn) 
    begin
        if (SysRstN_RstIn = '0') then 
            Timestamp_ValReg <= '0';
            Timestamp_Second_DatReg <= (others => '0');
            Timestamp_Nanosecond_DatReg <= (others => '0');
            RegisterDelay_DatReg <= 0;
            ClockTime_Second_DatReg <= (others => '0');
            ClockTime_Nanosecond_DatReg <= (others => '0');
            ClockTime_ValReg <= '0';
            NewMillisecond_DatReg <= '0';
            MillisecondCounter_CntReg <= 0;
            PeriodTimer_CntReg <= 0;
            PulseWidthTimer_CntReg <= 0;
            PulseWidth_DatReg <= (others=>'1');
            PulseStarted_ValReg <= (others => '0');
            PeriodIsOk_ValReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            PulseStarted_ValReg(1) <= PulseStarted_ValReg(0);
            -- single pulse
            Timestamp_ValReg <= '0';
            NewMillisecond_DatReg <= '0';
            PeriodError_DatReg <= '0';
            PulseWidthError_DatReg <= '0';
            -- calculate the delay of the high resolution timestamping which consists of 
            --     - the fixed offset of the clock domain crossing 
            --     - the number of high res. clock periods, from the event until the next rising edge of the system clock
            if ((TimestampSysClk2_EvtReg = '1') and (TimestampSysClk3_EvtReg = '0')) then 
                -- store the current time 
                ClockTime_Second_DatReg <= ClockTime_Second_DatIn;
                ClockTime_Nanosecond_DatReg <= ClockTime_Nanosecond_DatIn;
                ClockTime_ValReg <= ClockTime_ValIn;
                for i in ((HighResFreqMultiply_Gen*2)-1) downto 0 loop 
                    if (i >= (HighResFreqMultiply_Gen*2-3)) then 
                        if (TimestampSysClk_EvtShiftReg(i) = '1') then    
                            RegisterDelay_DatReg <= 3*ClockPeriod_Gen;
                            exit;
                        end if;    
                    elsif (i >= (HighResFreqMultiply_Gen-3)) then 
                        if (TimestampSysClk_EvtShiftReg(i) = '1') then    
                            RegisterDelay_DatReg <= 2*ClockPeriod_Gen + integer(round((real(ClockPeriod_Gen)/real(2*HighResFreqMultiply_Gen))+(real((i-(HighResFreqMultiply_Gen-3))*ClockPeriod_Gen)/real(HighResFreqMultiply_Gen))));
                            exit;
                        end if;
                    else
                        RegisterDelay_DatReg <= 2*ClockPeriod_Gen;
                    end if;        
                end loop;
            end if;

            -- Compensate the timestamp delays. Ensure that the Nanosecond field does not underflow and the pulse period is in the expected window. 
            if ((PeriodIsOk_ValReg = '1') and (TimestampSysClk3_EvtReg = '1') and (TimestampSysClk4_EvtReg = '0')) then 
                Timestamp_ValReg <= '1';
                if (ClockTime_ValReg = '0') then
                    Timestamp_ValReg <= '0';
                    Timestamp_Second_DatReg <= (others => '0');
                    Timestamp_Nanosecond_DatReg <= (others => '0');
                else
                    if (to_integer(unsigned(ClockTime_Nanosecond_DatReg)) < InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(CableDelay_Dat))) then -- smaller than 0
                        Timestamp_Nanosecond_DatReg <= std_logic_vector(to_unsigned((SecondNanoseconds_Con + 
                                                       to_integer(unsigned(ClockTime_Nanosecond_DatReg)) - 
                                                       (InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(CableDelay_Dat)))), NanosecondWidth_Con));
                        Timestamp_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) - 1); 
                    else -- larger than/equal to 0
                        Timestamp_Nanosecond_DatReg <= std_logic_vector(to_unsigned((to_integer(unsigned(ClockTime_Nanosecond_DatReg)) - 
                                                       (InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(CableDelay_Dat)))), NanosecondWidth_Con));
                        Timestamp_Second_DatReg <= ClockTime_Second_DatReg; 
                    end if;
                end if;        
            end if;

            -- Millisecond flag
            if ((TimestampSysClk2_EvtReg = '1') and (TimestampSysClk3_EvtReg = '0')) then 
                MillisecondCounter_CntReg <= 0;
            elsif (((Sim_Gen = false) and (MillisecondCounter_CntReg < (ClkCyclesInMillisecond_Con-1))) or
                   ((Sim_Gen = true) and (MillisecondCounter_CntReg < ((ClkCyclesInMillisecond_Con/1000)-1))))then 
                MillisecondCounter_CntReg <= MillisecondCounter_CntReg + 1;
            else
                MillisecondCounter_CntReg <= 0;
                NewMillisecond_DatReg <= '1';
            end if;

            -- Count period
            if ((PulseStarted_ValReg(1) = '1') and (NewMillisecond_DatReg = '1')) then 
                if (((Sim_Gen = false) and ((PeriodTimer_CntReg < (1000+PeriodWindow_Con)))) or
                    ((Sim_Gen = true) and ((PeriodTimer_CntReg < ((1000+PeriodWindow_Con)/10)) ))) then
                    PeriodTimer_CntReg <= PeriodTimer_CntReg + 1;
                -- when the period's upper limit is exceeded, report an error  
                elsif (((Sim_Gen = false) and (PeriodTimer_CntReg = (1000+PeriodWindow_Con))) or
                    ((Sim_Gen = true) and (PeriodTimer_CntReg = ((1000+PeriodWindow_Con)/10)))) then
                    PeriodTimer_CntReg <= PeriodTimer_CntReg + 1; -- set to an invalid value until a next pulse begins
                    PeriodError_DatReg <= '1';
                end if;
            end if;
            
            -- if the new pulse comes check the min. period limit 
            if ((PulseStarted_ValReg(1) = '1') and (TimestampSysClk2_EvtReg = '1' and TimestampSysClk3_EvtReg = '0')) then 
                if (((Sim_Gen = false) and ((PeriodTimer_CntReg < (1000-PeriodWindow_Con)))) or
                    ((Sim_Gen = true) and ((PeriodTimer_CntReg < ((1000-PeriodWindow_Con)/10)) ))) then
                    PeriodError_DatReg <= '1';
                end if;    
            end if;
            
            -- Validate period
            if (((Sim_Gen = false) and ((PeriodTimer_CntReg < (1000+PeriodWindow_Con)) and (PeriodTimer_CntReg >= (1000-PeriodWindow_Con)))) or
                ((Sim_Gen = true) and ((PeriodTimer_CntReg < ((1000+PeriodWindow_Con)/10)) and (PeriodTimer_CntReg >= ((1000-PeriodWindow_Con)/10))))) then
                PeriodIsOk_ValReg <= '1';
            else
                PeriodIsOk_ValReg <= '0';
            end if;

            -- Count pulse width
            if ((TimestampSysClk2_EvtReg = '1') and (NewMillisecond_DatReg = '1')) then 
                if (((Sim_Gen = false) and (PulseWidthTimer_CntReg < PulseWidthMax_Con)) or
                    ((Sim_Gen = true) and (PulseWidthTimer_CntReg < PulseWidthMax_Con))) then
                    PulseWidthTimer_CntReg <= PulseWidthTimer_CntReg + 1;
                end if;
            elsif (TimestampSysClk2_EvtReg = '0') then 
                PulseWidthTimer_CntReg <= 0;
            end if;


            -- At the falling edge check the min. pulse width limit 
            if ((PulseStarted_ValReg(1) ='1') and (TimestampSysClk2_EvtReg = '0') and (TimestampSysClk3_EvtReg = '1')) then 
                PulseWidth_DatReg <= std_logic_vector(to_unsigned(PulseWidthTimer_CntReg,10));
                if (((Sim_Gen = false) and (PulseWidthTimer_CntReg < PulseWidthMin_Con)) or 
                    ((Sim_Gen = true) and (PulseWidthTimer_CntReg < integer(ceil(real(real(PulseWidthMin_Con)/real(10))))))) then -- this could be 0 if not ceiled
                    PulseWidthError_DatReg <= '1'; 
                    PulseWidth_DatReg <= (others=>'1'); -- width unknown or out-of-bounds
                end if;    
            end if;

            -- Check the max. pulse width limit 
            if ((PulseStarted_ValReg(1) ='1') and (TimestampSysClk2_EvtReg = '1') and (NewMillisecond_DatReg = '1')) then 
                if (((Sim_Gen = false) and (PulseWidthTimer_CntReg = PulseWidthMax_Con)) or
                    ((Sim_Gen = true) and (PulseWidthTimer_CntReg = (PulseWidthMax_Con/10)))) then 
                    PulseWidthTimer_CntReg <= PulseWidthTimer_CntReg + 1; -- set to an invalid value until a next pulse begins
                    PulseWidthError_DatReg <= '1'; 
                    PulseWidth_DatReg <= (others=>'1'); -- width unknown or out-of-bounds
                end if;
            end if;

            -- At a new event restart the counters
            if ((TimestampSysClk2_EvtReg = '1') and (TimestampSysClk3_EvtReg = '0')) then 
                PeriodTimer_CntReg <= 0;
                PulseWidthTimer_CntReg <= 0;
                PulseStarted_ValReg(0) <= '1'; 
            end if;

            if (Enable_Ena = '0') then 
                Timestamp_ValReg <= '0';
                Timestamp_Second_DatReg <= (others => '0');
                Timestamp_Nanosecond_DatReg <= (others => '0');
                PeriodTimer_CntReg <= 0;
                PulseWidthTimer_CntReg <= 0;
                PulseWidth_DatReg <= (others=>'1');
                PulseStarted_ValReg <= (others => '0');
            end if;
            
        end if;
    end process Timestamp_Prc;
    
    -- Calculate the new offset
    OffsetCalc_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            OffsetCalcState_StaReg <= WaitTimestamp_St;
            OffsetCalcActive_ValReg <= '0';
            OffsetAdjustment_Second_DatReg <= (others => '0');
            OffsetAdjustment_Nanosecond_DatReg <= (others => '0');
            OffsetAdjustment_Sign_DatReg <= '0';     
            OffsetAdjustment_Interval_DatReg <= (others => '0');
            OffsetAdjustment_ValReg <= '0';
            OffsetAdjustment_ValOldReg <= '0';
            OffsetAdjustmentInvalid_ValReg <= '0';
            WaitTimer_CntReg <= 0;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            OffsetAdjustment_ValOldReg <= OffsetAdjustment_ValReg;
            OffsetAdjustment_ValReg <= '0';
            OffsetAdjustmentInvalid_ValReg <= '0';
            
            if ((Enable_Ena = '0') or (PulseWidthError_DatReg = '1') or (PeriodError_DatReg = '1')) then
                OffsetCalcActive_ValReg <= '0';
            end if;
            
            case (OffsetCalcState_StaReg) is
                when WaitTimestamp_St => 
                    WaitTimer_CntReg <= 0;
                    if (Timestamp_ValReg = '1') then -- single pulse
                        if (OffsetCalcActive_ValReg = '0') then -- initialize calculation
                            OffsetCalcState_StaReg <= WaitTimestamp_St;
                            OffsetCalcActive_ValReg <= '1';
                            -- Error
                            OffsetAdjustment_Second_DatReg <= (others => '0');
                            OffsetAdjustment_Nanosecond_DatReg <= (others => '0');
                            OffsetAdjustment_Sign_DatReg <= '0';     
                            OffsetAdjustment_Interval_DatReg <= (others => '0');
                            OffsetAdjustment_ValReg <= '1';  -- trigger calculation
                            OffsetAdjustmentInvalid_ValReg <= '1';
                        else
                            OffsetCalcState_StaReg <= WaitDrift_St;
                            if (Sim_Gen = true) then 
                                if ((unsigned(Timestamp_Nanosecond_DatReg) mod (SecondNanoseconds_Con/10000)) >=  to_unsigned(((SecondNanoseconds_Con/10000)/2),NanosecondWidth_Con)) then -- correct in negative direction
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(0, SecondWidth_Con)); -- always zero
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con/10000,NanosecondWidth_Con)) - (unsigned(Timestamp_Nanosecond_DatReg) mod (SecondNanoseconds_Con/10000)), NanosecondWidth_Con));
                                    OffsetAdjustment_Sign_DatReg <= '0';
                                else -- correct in positive direction
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(0, SecondWidth_Con)); -- always zero
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatReg) mod (SecondNanoseconds_Con/10000), NanosecondWidth_Con));
                                    OffsetAdjustment_Sign_DatReg <= '1';
                                end if;
                            else
                                if (unsigned(Timestamp_Nanosecond_DatReg) >= to_unsigned((SecondNanoseconds_Con/2),NanosecondWidth_Con) ) then -- correct in negative direction
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(0, SecondWidth_Con)); -- always zero
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) - unsigned(Timestamp_Nanosecond_DatReg), NanosecondWidth_Con));
                                    OffsetAdjustment_Sign_DatReg <= '0';
                                else -- correct in positive direction
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(0, SecondWidth_Con)); -- always zero
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatReg), NanosecondWidth_Con));
                                    OffsetAdjustment_Sign_DatReg <= '1';
                                end if;
                            end if;
                        end if;
                    end if;
                
                when WaitDrift_St => 
                    if (PI_DriftAdjustment_ValReg = '1') then
                        OffsetCalcState_StaReg <= WaitTimestamp_St;
                        OffsetAdjustment_ValReg <= '1';  -- trigger calculation
                        -- subtract drift adjustment from offset adjustment, as it will be corrected in the next interval 
                        if (Sim_Gen = false) then 
                            if (OffsetAdjustment_Sign_DatReg = PI_DriftAdjustment_Sign_DatReg) then -- same signs of adjustments
                                OffsetAdjustment_Second_DatReg <= (others => '0');
                                if (unsigned(OffsetAdjustment_Nanosecond_DatReg) < unsigned(PI_DriftAdjustment_Nanosecond_DatReg)) then 
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftAdjustment_Nanosecond_DatReg) - unsigned(OffsetAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                    if (OffsetAdjustment_Sign_DatReg = '1') then -- offset and drift negative 
                                        OffsetAdjustment_Sign_DatReg <= '0'; 
                                    else -- offset and drift positive 
                                        OffsetAdjustment_Sign_DatReg <= '1'; 
                                    end if; 
                                else
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) - unsigned(PI_DriftAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                    if (OffsetAdjustment_Sign_DatReg = '1') then -- offset and drift negative 
                                        OffsetAdjustment_Sign_DatReg <= '1'; 
                                    else -- offset and drift positive 
                                        OffsetAdjustment_Sign_DatReg <= '0'; 
                                    end if; 
                                end if;
                            elsif (OffsetAdjustment_Sign_DatReg /=  PI_DriftAdjustment_Sign_DatReg) then -- different signs of adjustments 
                                if ((unsigned(OffsetAdjustment_Nanosecond_DatReg) + unsigned(PI_DriftAdjustment_Nanosecond_DatReg)) >= to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con)) then
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(OffsetAdjustment_Nanosecond_DatReg) + unsigned(PI_DriftAdjustment_Nanosecond_DatReg)) - to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con), NanosecondWidth_Con));
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(1,SecondWidth_Con));
                                else
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) + unsigned(PI_DriftAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                    OffsetAdjustment_Second_DatReg <= (others => '0');
                                end if;
                                if (OffsetAdjustment_Sign_DatReg = '1') then -- offset negative/drift positive 
                                    OffsetAdjustment_Sign_DatReg <= '1'; 
                                else -- offset positive/drift negative 
                                    OffsetAdjustment_Sign_DatReg <= '0'; 
                                end if; 
                            end if;
                        else
                            if (OffsetAdjustment_Sign_DatReg = PI_DriftAdjustment_Sign_DatReg) then -- same signs of adjustments
                                OffsetAdjustment_Second_DatReg <= (others => '0');
                                if (unsigned(OffsetAdjustment_Nanosecond_DatReg) < (unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000)) then 
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000) - unsigned(OffsetAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                    if (OffsetAdjustment_Sign_DatReg = '1') then -- offset and drift negative 
                                        OffsetAdjustment_Sign_DatReg <= '0'; 
                                    else -- offset and drift positive 
                                        OffsetAdjustment_Sign_DatReg <= '1'; 
                                    end if; 
                                else
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) - (unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000), NanosecondWidth_Con));
                                    if (OffsetAdjustment_Sign_DatReg = '1') then -- offset and drift negative 
                                        OffsetAdjustment_Sign_DatReg <= '1'; 
                                    else -- offset and drift positive 
                                        OffsetAdjustment_Sign_DatReg <= '0'; 
                                    end if; 
                                end if;
                            elsif (OffsetAdjustment_Sign_DatReg /=  PI_DriftAdjustment_Sign_DatReg) then -- different signs of adjustments 
                                if ((unsigned(OffsetAdjustment_Nanosecond_DatReg) + (unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000)) >= to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con)) then
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(OffsetAdjustment_Nanosecond_DatReg)) + (unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000) - to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con), NanosecondWidth_Con));
                                    OffsetAdjustment_Second_DatReg <= std_logic_vector(to_unsigned(1,SecondWidth_Con));
                                else
                                    OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize((unsigned(OffsetAdjustment_Nanosecond_DatReg)) + (unsigned(PI_DriftAdjustment_Nanosecond_DatReg)/10000), NanosecondWidth_Con));
                                    OffsetAdjustment_Second_DatReg <= (others => '0');
                                end if;
                                if (OffsetAdjustment_Sign_DatReg = '1') then -- offset negative/drift positive 
                                    OffsetAdjustment_Sign_DatReg <= '1'; 
                                else -- offset positive/drift negative 
                                    OffsetAdjustment_Sign_DatReg <= '0'; 
                                end if; 
                            end if;
                        end if;
                        -- assign the pps interval
                        if (Sim_Gen = false) then
                            OffsetAdjustment_Interval_DatReg <= std_logic_vector(to_unsigned((6*(SecondNanoseconds_Con/10)), AdjustmentIntervalWidth_Con)); -- 60% of a pps interval which means that this is the maximum rate
                        else
                            OffsetAdjustment_Interval_DatReg <= std_logic_vector(to_unsigned((6*(SecondNanoseconds_Con/100000)), AdjustmentIntervalWidth_Con)); -- correct in 60% pps interval
                        end if;
                    elsif (NewMillisecond_DatReg = '1') then 
                        if (WaitTimer_CntReg < WaitTimer_Con) then 
                            WaitTimer_CntReg <= WaitTimer_CntReg + 1;
                        else -- wait drift timeout 
                            OffsetCalcState_StaReg <= WaitTimestamp_St;
                            -- Error
                            OffsetAdjustment_Second_DatReg <= (others => '0');
                            OffsetAdjustment_Nanosecond_DatReg <= (others => '0');
                            OffsetAdjustment_Sign_DatReg <= '0';
                            OffsetAdjustment_Interval_DatReg <= (others => '0');
                            OffsetAdjustment_ValReg <= '1';  -- trigger calculation
                            OffsetAdjustmentInvalid_ValReg <= '1';
                        end if;
                    end if;
                    
                when others =>
                    OffsetCalcState_StaReg <= WaitTimestamp_St;
                        
            end case;                             
        end if;
    end process OffsetCalc_Prc;

    -- Calculate the new drift
    DriftCalc_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            DriftCalcState_StaReg <= WaitTimestamp_St;
            DriftCalcActive_ValReg <= '0';
            
            DriftAdjustment_Nanosecond_DatReg <= (others => '0');
            DriftAdjustment_Sign_DatReg <= '0';
            DriftAdjustment_Interval_DatReg <= (others => '0');
            DriftAdjustment_ValReg <= '0';
            DriftAdjustment_ValOldReg <= '0';
            DriftAdjustmentInvalid_ValReg <= '0';
            
            DriftAdjustmentDelta_Second_DatReg <= (others => '0');
            DriftAdjustmentDelta_Nanosecond_DatReg <= (others => '0');
            DriftAdjustmentDelta_Sign_DatReg <= '0';
            DriftAdjustmentDelta_Interval_DatReg <= (others => '0');

            PI_OffsetAdjustRetain_Second_DatReg <= (others => '0');
            PI_OffsetAdjustRetain_Nanosecond_DatReg <= (others => '0');
            PI_OffsetAdjustRetain_Sign_DatReg <= '0';
            PI_OffsetAdjustRetain_Interval_DatReg <= (others => '0');
            PI_OffsetAdjustRetain_ValReg <= '0';
            
            Timestamp_Second_DatOldReg <= (others => '0');
            Timestamp_Nanosecond_DatOldReg <= (others => '0');
            
            Normalizer1_DatReg <= (others => '0');
            Normalizer1_Result_DatReg <= (others => '0');
            NormalizeActive1_ValReg <= '0';
            NormalizeActive2_ValReg <= '0';
            Step_CntReg <= 0;
            NormalizeProduct_DatReg <= (others => '0');
            Normalizer2_DatReg <= (others => '0');
            Normalizer2_Result_DatReg <= (others => '0');
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            DriftAdjustment_ValOldReg <= DriftAdjustment_ValReg;
            DriftAdjustmentInvalid_ValReg <= '0';
            DriftAdjustment_ValReg <= '0'; -- active for 1 clk cycle
            
            if ((Enable_Ena = '0') or (PulseWidthError_DatReg = '1') or (PeriodError_DatReg = '1')) then
                DriftCalcActive_ValReg <= '0';
            end if;

            case (DriftCalcState_StaReg) is
            
                when WaitTimestamp_St =>
                    Normalizer1_DatReg <= (others => '0');
                    Normalizer1_Result_DatReg <= (others => '0');
                    NormalizeActive1_ValReg <= '0';
                    NormalizeActive2_ValReg <= '0';
                    Step_CntReg <= 0;
                    NormalizeProduct_DatReg <= (others => '0');
                    Normalizer2_DatReg <= (others => '0');
                    Normalizer2_Result_DatReg <= (others => '0');
                    if (PI_OffsetAdjustment_ValReg = '1') then 
                        PI_OffsetAdjustRetain_Second_DatReg <= PI_OffsetAdjustment_Second_DatReg;
                        PI_OffsetAdjustRetain_Nanosecond_DatReg <= PI_OffsetAdjustment_Nanosecond_DatReg;
                        PI_OffsetAdjustRetain_Sign_DatReg <= PI_OffsetAdjustment_Sign_DatReg;
                        PI_OffsetAdjustRetain_Interval_DatReg <= PI_OffsetAdjustment_Interval_DatReg;
                        PI_OffsetAdjustRetain_ValReg <= PI_OffsetAdjustment_ValReg;
                        if (unsigned(PI_OffsetAdjustRetain_Second_DatReg) /= 0) then 
                            DriftCalcActive_ValReg <= '0';
                        end if;
                    end if;
                    if (Timestamp_ValReg = '1') then -- single pulse
                        DriftCalcState_StaReg <= SubOffsetOld_St;
                        Timestamp_Second_DatOldReg <= Timestamp_Second_DatReg;
                        Timestamp_Nanosecond_DatOldReg <= Timestamp_Nanosecond_DatReg;
                        -- subtract the old TS from the new TS
                        if (unsigned(Timestamp_Second_DatReg) > unsigned(Timestamp_Second_DatOldReg)) then 
                            DriftAdjustmentDelta_Sign_DatReg <= '0';
                            if (unsigned(Timestamp_Nanosecond_DatReg) >= unsigned(Timestamp_Nanosecond_DatOldReg)) then 
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatReg) - unsigned(Timestamp_Nanosecond_DatOldReg), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Second_DatReg) - unsigned(Timestamp_Second_DatOldReg), SecondWidth_Con));
                            else
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(Timestamp_Nanosecond_DatReg)) - unsigned(Timestamp_Nanosecond_DatOldReg), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Second_DatReg) - unsigned(Timestamp_Second_DatOldReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                            end if;
                        elsif (unsigned(Timestamp_Second_DatReg) = unsigned(Timestamp_Second_DatOldReg)) then 
                            if (unsigned(Timestamp_Nanosecond_DatReg) >= unsigned(Timestamp_Nanosecond_DatOldReg)) then 
                                DriftAdjustmentDelta_Second_DatReg <= (others => '0');
                                DriftAdjustmentDelta_Sign_DatReg <= '0';
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatReg) - unsigned(Timestamp_Nanosecond_DatOldReg), NanosecondWidth_Con));
                            else
                                DriftAdjustmentDelta_Sign_DatReg <= '1'; 
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatOldReg) - unsigned(Timestamp_Nanosecond_DatReg), NanosecondWidth_Con)); 
                            end if;
                        else 
                            DriftAdjustmentDelta_Sign_DatReg <= '1'; 
                            if (unsigned(Timestamp_Nanosecond_DatOldReg) >= unsigned(Timestamp_Nanosecond_DatReg)) then 
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Nanosecond_DatOldReg) - unsigned(Timestamp_Nanosecond_DatReg), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Second_DatOldReg) - unsigned(Timestamp_Second_DatReg), SecondWidth_Con));
                            else
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(Timestamp_Nanosecond_DatOldReg)) - unsigned(Timestamp_Nanosecond_DatReg), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(Timestamp_Second_DatOldReg) - unsigned(Timestamp_Second_DatReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                            end if;
                        end if;
                    end if;
                
                when SubOffsetOld_St => 
                    if (DriftCalcActive_ValReg = '0') then 
                        DriftCalcState_StaReg <= WaitTimestamp_St;
                        DriftCalcActive_ValReg <= '1';
                        -- Error                                                                              
                        DriftAdjustment_Nanosecond_DatReg <=(others => '0'); -- no new drift
                        DriftAdjustment_Sign_DatReg <= '0';
                        DriftAdjustment_Interval_DatReg <= (others => '0');
                        DriftAdjustment_ValReg <= '1';  -- trigger calculation
                        DriftAdjustmentInvalid_ValReg <= '1';
                    else -- substract the offset which was corrected in the last sync interval
                        DriftCalcState_StaReg <= Diff_St;
                        if ((DriftAdjustmentDelta_Sign_DatReg = '0') and (PI_OffsetAdjustRetain_Sign_DatReg = '0')) then 
                            if (unsigned(DriftAdjustmentDelta_Second_DatReg) > unsigned(PI_OffsetAdjustRetain_Second_DatReg)) then 
                                DriftAdjustmentDelta_Sign_DatReg <= '0';
                                if (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) >= unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) - unsigned(PI_OffsetAdjustRetain_Second_DatReg), SecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(DriftAdjustmentDelta_Nanosecond_DatReg)) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) - unsigned(PI_OffsetAdjustRetain_Second_DatReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                                end if;
                            elsif (DriftAdjustmentDelta_Second_DatReg = PI_OffsetAdjustRetain_Second_DatReg) then 
                                if (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) >= unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Second_DatReg <= (others => '0');
                                    DriftAdjustmentDelta_Sign_DatReg <= '0';
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Sign_DatReg <= '1'; 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con)); 
                                end if;
                            else 
                                DriftAdjustmentDelta_Sign_DatReg <= '1'; 
                                if (unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) >= unsigned(DriftAdjustmentDelta_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Second_DatReg) - unsigned(DriftAdjustmentDelta_Second_DatReg), SecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Second_DatReg) - unsigned(DriftAdjustmentDelta_Second_DatReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                                end if;
                            end if;
                        elsif (DriftAdjustmentDelta_Sign_DatReg /= PI_OffsetAdjustRetain_Sign_DatReg) then
                            if (DriftAdjustmentDelta_Sign_DatReg = '1' and PI_OffsetAdjustRetain_Sign_DatReg = '0') then 
                                DriftAdjustmentDelta_Sign_DatReg <= '1';
                            else
                                DriftAdjustmentDelta_Sign_DatReg <= '0';
                            end if;
                            if (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) + unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) >= to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con)) then 
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) + unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) - to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) + unsigned(PI_OffsetAdjustRetain_Second_DatReg) + to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                            else
                                DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) + unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) + unsigned(PI_OffsetAdjustRetain_Second_DatReg), SecondWidth_Con));                            
                            end if;                            
                        elsif ((DriftAdjustmentDelta_Sign_DatReg = '1') and (PI_OffsetAdjustRetain_Sign_DatReg = '1')) then
                            if (unsigned(DriftAdjustmentDelta_Second_DatReg) > unsigned(PI_OffsetAdjustRetain_Second_DatReg)) then 
                                DriftAdjustmentDelta_Sign_DatReg <= '1';
                                if (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) >= unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) - unsigned(PI_OffsetAdjustRetain_Second_DatReg), SecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(DriftAdjustmentDelta_Nanosecond_DatReg)) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Second_DatReg) - unsigned(PI_OffsetAdjustRetain_Second_DatReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                                end if;
                            elsif (DriftAdjustmentDelta_Second_DatReg = PI_OffsetAdjustRetain_Second_DatReg) then 
                                if (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) >= unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Second_DatReg <= (others => '0');
                                    DriftAdjustmentDelta_Sign_DatReg <= '1';
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) - unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg), NanosecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Sign_DatReg <= '0'; 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con)); 
                                end if;
                            else 
                                DriftAdjustmentDelta_Sign_DatReg <= '0'; 
                                if (unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) >= unsigned(DriftAdjustmentDelta_Nanosecond_DatReg)) then 
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Second_DatReg) - unsigned(DriftAdjustmentDelta_Second_DatReg), SecondWidth_Con));
                                else
                                    DriftAdjustmentDelta_Nanosecond_DatReg <= std_logic_vector(resize((to_unsigned(SecondNanoseconds_Con,NanosecondWidth_Con) + unsigned(PI_OffsetAdjustRetain_Nanosecond_DatReg)) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con));
                                    DriftAdjustmentDelta_Second_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustRetain_Second_DatReg) - unsigned(DriftAdjustmentDelta_Second_DatReg) - to_unsigned(1,SecondWidth_Con), SecondWidth_Con));
                                end if;
                            end if;
                        end if;
                    end if;
                
                when Diff_St =>  -- diff to the norm second
                    if (((Sim_Gen = false) and (unsigned(DriftAdjustmentDelta_Second_DatReg) = 1) and (DriftAdjustmentDelta_Sign_DatReg = '0')) or -- larger than one second
                        ((Sim_Gen = true) and (unsigned(DriftAdjustmentDelta_Second_DatReg) = 0) and (DriftAdjustmentDelta_Sign_DatReg = '0') and (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) >= to_unsigned((SecondNanoseconds_Con/10000), NanosecondWidth_Con)))) then 
                        DriftCalcState_StaReg <= Normalize_Step1_St;
                        if (Sim_Gen = false) then 
                            DriftAdjustment_Interval_DatReg <= std_logic_vector(resize(to_unsigned(SecondNanoseconds_Con, NanosecondWidth_Con) + unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), AdjustmentIntervalWidth_Con));
                            DriftAdjustment_Nanosecond_DatReg <= DriftAdjustmentDelta_Nanosecond_DatReg; -- the drift "second" should be always '0'
                        else
                            DriftAdjustment_Interval_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), AdjustmentIntervalWidth_Con));
                            DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) - to_unsigned((SecondNanoseconds_Con/10000), NanosecondWidth_Con), NanosecondWidth_Con)); 
                        end if;
                        DriftAdjustment_Sign_DatReg <= '1';
                    elsif (((Sim_Gen = false) and (unsigned(DriftAdjustmentDelta_Second_DatReg) = 0) and (DriftAdjustmentDelta_Sign_DatReg = '0')) or -- smaller than one second
                           ((Sim_Gen = true) and (unsigned(DriftAdjustmentDelta_Second_DatReg) = 0) and (DriftAdjustmentDelta_Sign_DatReg = '0') and (unsigned(DriftAdjustmentDelta_Nanosecond_DatReg) < to_unsigned((SecondNanoseconds_Con/10000), NanosecondWidth_Con)))) then 
                        DriftCalcState_StaReg <= Normalize_Step1_St;
                        if (Sim_Gen = false) then 
                            DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(to_unsigned(SecondNanoseconds_Con, NanosecondWidth_Con) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con)); 
                        else
                            DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(to_unsigned((SecondNanoseconds_Con/10000), NanosecondWidth_Con) - unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), NanosecondWidth_Con));                                 
                        end if;
                        DriftAdjustment_Interval_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustmentDelta_Nanosecond_DatReg), AdjustmentIntervalWidth_Con));   
                        DriftAdjustment_Sign_DatReg <= '0';
                    else
                        DriftCalcState_StaReg <= WaitTimestamp_St; 
                        -- Error                                                                              
                        DriftAdjustment_Nanosecond_DatReg <= (others => '0'); -- no new drift
                        DriftAdjustment_Interval_DatReg <= (others => '0');
                        DriftAdjustment_Sign_DatReg <= '0';
                        DriftAdjustment_ValReg <= '1';  -- trigger calculation
                        DriftAdjustmentInvalid_ValReg <= '1';
                    end if;
                    -- Drift Normalization
                    Normalizer1_DatReg((2*AdjustmentIntervalWidth_Con-1) downto AdjustmentIntervalWidth_Con) <= (others => '0');
                    if (Sim_Gen = false) then 
                        Normalizer1_DatReg((AdjustmentIntervalWidth_Con-1) downto 0) <= std_logic_vector(to_unsigned(SecondNanoseconds_Con, AdjustmentIntervalWidth_Con)); 
                    else 
                        Normalizer1_DatReg((AdjustmentIntervalWidth_Con-1) downto 0) <= std_logic_vector(to_unsigned((SecondNanoseconds_Con/10000), AdjustmentIntervalWidth_Con)) ;
                    end if;
                    Step_CntReg <= AdjustmentIntervalWidth_Con-1;
                    Normalizer1_Result_DatReg <= (others => '0');
                    NormalizeActive1_ValReg <= '1';

                when Normalize_Step1_St => 
                    if (NormalizeActive1_ValReg = '1') then 
                        if (DriftAdjustment_Nanosecond_DatReg(((AdjustmentIntervalWidth_Con-1) - Step_CntReg)) = '1') then
                            Normalizer1_DatReg <= Normalizer1_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0';
                            Normalizer1_Result_DatReg <= std_logic_vector(resize(unsigned(Normalizer1_Result_DatReg) + unsigned(Normalizer1_DatReg), (2*AdjustmentIntervalWidth_Con)));
                        else
                            Normalizer1_DatReg <= Normalizer1_DatReg(((2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0';
                        end if;
                    else
                        DriftCalcState_StaReg <= Normalize_Step2_St;
                        NormalizeProduct_DatReg(((2*2*AdjustmentIntervalWidth_Con)-1) downto (2*AdjustmentIntervalWidth_Con)) <= (others => '0'); 
                        NormalizeProduct_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto 0) <= Normalizer1_Result_DatReg;
                        Normalizer2_DatReg(((2*2*AdjustmentIntervalWidth_Con)-1) downto (2*AdjustmentIntervalWidth_Con)) <= std_logic_vector(to_unsigned(to_integer(unsigned(DriftAdjustment_Interval_DatReg)), (2*AdjustmentIntervalWidth_Con)));
                        Normalizer2_DatReg(((2*AdjustmentIntervalWidth_Con)-1) downto 0) <= (others => '0');
                        Normalizer2_Result_DatReg <= (others => '0');
                        Step_CntReg <= (2*AdjustmentIntervalWidth_Con) - 1;
                        NormalizeActive2_ValReg <= '1';
                    end if;
                    if (Step_CntReg > 0) then
                        Step_CntReg <= Step_CntReg - 1;
                    else
                        NormalizeActive1_ValReg <= '0';
                    end if;
                
                when Normalize_Step2_St => 
                    if (NormalizeActive2_ValReg = '1') then 
                        if (unsigned(NormalizeProduct_DatReg(((2*2*AdjustmentIntervalWidth_Con)-2) downto 0) & '0') >= unsigned(Normalizer2_DatReg)) then
                            NormalizeProduct_DatReg <= std_logic_vector(resize(unsigned(NormalizeProduct_DatReg(((2*2*AdjustmentIntervalWidth_Con)-2)  downto 0) & '0') - unsigned(Normalizer2_DatReg), (2*2*AdjustmentIntervalWidth_Con)));
                            Normalizer2_Result_DatReg(Step_CntReg) <= '1';
                        else
                            NormalizeProduct_DatReg <= (NormalizeProduct_DatReg(((2*2*AdjustmentIntervalWidth_Con)-2)  downto 0) & '0');
                            Normalizer2_Result_DatReg(Step_CntReg) <= '0';
                        end if;
                    else
                        DriftCalcState_StaReg <= Normalize_Step3_St;
                    end if;
                    if (Step_CntReg > 0) then
                        Step_CntReg <= Step_CntReg - 1;
                    else
                        NormalizeActive2_ValReg <= '0';
                    end if;

                when Normalize_Step3_St => 
                    if (Sim_Gen = false) then
                        DriftAdjustment_Interval_DatReg <= std_logic_vector(to_unsigned(SecondNanoseconds_Con, AdjustmentIntervalWidth_Con));
                    else
                        DriftAdjustment_Interval_DatReg <= std_logic_vector(to_unsigned((SecondNanoseconds_Con/10000), AdjustmentIntervalWidth_Con));
                    end if;
                    DriftAdjustment_Nanosecond_DatReg <= Normalizer2_Result_DatReg((NanosecondWidth_Con-1) downto 0);
                    DriftAdjustment_ValReg <= '1';  -- trigger calculation
                    DriftCalcState_StaReg <= WaitTimestamp_St;
                        
                when others =>
                    DriftCalcState_StaReg <= WaitTimestamp_St;
            
            end case;
        end if;
    end process DriftCalc_Prc;
    
    -- Assign the PI factors if they are set dynamically. Otherwise, use the default values.
    AssignServoParameters_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            OffsetFactorP_DatReg <= OffsetFactorP_Con;
            OffsetFactorI_DatReg <= OffsetFactorI_Con;
            DriftFactorP_DatReg <= DriftFactorP_Con;
            DriftFactorI_DatReg <= DriftFactorI_Con;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            if (Servo_ValIn = '1') then
                OffsetFactorP_DatReg <= resize(unsigned(ServoOffsetFactorP_DatIn), FactorSize_Con);
                OffsetFactorI_DatReg <= resize(unsigned(ServoOffsetFactorI_DatIn), FactorSize_Con);
                DriftFactorP_DatReg <= resize(unsigned(ServoDriftFactorP_DatIn), FactorSize_Con);
                DriftFactorI_DatReg <= resize(unsigned(ServoDriftFactorI_DatIn), FactorSize_Con);
            end if;
        end if;
    end process AssignServoParameters_Prc;
    
    -- Calculate the PI servo offset correction
    PI_Offset_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            PI_OffsetState_StaReg <= Idle_St;
            
            PI_OffsetAdjustment_Second_DatReg <= (others => '0');
            PI_OffsetAdjustment_Nanosecond_DatReg <= (others=> '0');
            PI_OffsetAdjustment_Sign_DatReg <= '0';
            PI_OffsetAdjustment_Interval_DatReg <= (others => '0');
            PI_OffsetAdjustment_ValReg <= '0';
            PI_OffsetIntegral_DatReg <= (others => '0');
            PI_OffsetIntegralSign_DatReg <= '0';
            PI_OffsetMul_DatReg <= (others => '0');
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            PI_OffsetAdjustment_ValReg <= '0'; -- triggered for 1 clock cycle
            
            if (Enable_Ena = '1') then 
                case (PI_OffsetState_StaReg) is
                    when Idle_St => 
                        if ((OffsetAdjustment_ValReg = '1') and (OffsetAdjustment_ValOldReg = '0')) then 
                            if (OffsetAdjustmentInvalid_ValReg = '1') then -- error case during offset calculation
                                
                                PI_OffsetAdjustment_Second_DatReg <= (others => '0');
                                PI_OffsetAdjustment_Nanosecond_DatReg <= (others=> '0');
                                PI_OffsetAdjustment_Sign_DatReg <= '0';
                                PI_OffsetAdjustment_ValReg <= '1'; -- trigger correction
                            elsif (to_integer(unsigned(OffsetAdjustment_Second_DatReg)) /= 0) then -- too big offset will trigger a jump
                                PI_OffsetAdjustment_Second_DatReg <= OffsetAdjustment_Second_DatReg;
                                PI_OffsetAdjustment_Nanosecond_DatReg <= OffsetAdjustment_Nanosecond_DatReg;
                                PI_OffsetAdjustment_Sign_DatReg <= OffsetAdjustment_Sign_DatReg;
                                PI_OffsetAdjustment_Interval_DatReg <= OffsetAdjustment_Interval_DatReg;
                                PI_OffsetAdjustment_ValReg <= '1'; -- trigger correction
                                PI_OffsetIntegralSign_DatReg <= '0';
                                PI_OffsetIntegral_DatReg <= (others => '0');
                            else
                                PI_OffsetAdjustment_Second_DatReg <= OffsetAdjustment_Second_DatReg;
                                PI_OffsetAdjustment_Nanosecond_DatReg <= OffsetAdjustment_Nanosecond_DatReg;
                                PI_OffsetAdjustment_Sign_DatReg <= OffsetAdjustment_Sign_DatReg;
                                PI_OffsetAdjustment_Interval_DatReg <= OffsetAdjustment_Interval_DatReg;
                                PI_OffsetMul_DatReg <= std_logic_vector(resize(unsigned(OffsetAdjustment_Nanosecond_DatReg), FactorSize_Con) * OffsetFactorP_DatReg);
                                PI_OffsetState_StaReg <= P_St;
                                -- Add the calculated new offset to the PI offset integral 
                                if ((OffsetAdjustment_Sign_DatReg = '1') and (PI_OffsetIntegralSign_DatReg = '1')) then -- both negative
                                    if ((unsigned(OffsetAdjustment_Nanosecond_DatReg) + PI_OffsetIntegral_DatReg) >= IntegralMax_Con) then
                                        PI_OffsetIntegral_DatReg <= IntegralMax_Con;
                                    else
                                        PI_OffsetIntegral_DatReg <= resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) + PI_OffsetIntegral_DatReg, FactorSize_Con);
                                    end if;    
                                    PI_OffsetIntegralSign_DatReg <= '1';                           
                                elsif ((OffsetAdjustment_Sign_DatReg = '1') and (PI_OffsetIntegralSign_DatReg = '0')) then -- inversed
                                    if (PI_OffsetIntegral_DatReg >= unsigned(OffsetAdjustment_Nanosecond_DatReg)) then
                                        PI_OffsetIntegral_DatReg <= resize(PI_OffsetIntegral_DatReg - unsigned(OffsetAdjustment_Nanosecond_DatReg), FactorSize_Con);
                                        PI_OffsetIntegralSign_DatReg <= '0';
                                    else
                                        PI_OffsetIntegral_DatReg <= resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) - PI_OffsetIntegral_DatReg, FactorSize_Con);
                                        PI_OffsetIntegralSign_DatReg <= '1';
                                    end if;
                                elsif ((OffsetAdjustment_Sign_DatReg = '0') and (PI_OffsetIntegralSign_DatReg = '1')) then -- inversed
                                    if (unsigned(OffsetAdjustment_Nanosecond_DatReg) >= PI_OffsetIntegral_DatReg) then
                                        PI_OffsetIntegral_DatReg <= resize(unsigned(OffsetAdjustment_Nanosecond_DatReg) - PI_OffsetIntegral_DatReg, FactorSize_Con);  
                                        PI_OffsetIntegralSign_DatReg <= '0';                           
                                    else
                                        PI_OffsetIntegral_DatReg <= resize(PI_OffsetIntegral_DatReg - unsigned(OffsetAdjustment_Nanosecond_DatReg), FactorSize_Con); 
                                        PI_OffsetIntegralSign_DatReg <= '1';                           
                                    end if;                    
                                elsif ((OffsetAdjustment_Sign_DatReg = '0') and (PI_OffsetIntegralSign_DatReg = '0')) then -- both positive
                                    if ((PI_OffsetIntegral_DatReg + unsigned(OffsetAdjustment_Nanosecond_DatReg)) >= IntegralMax_Con) then
                                        PI_OffsetIntegral_DatReg <= IntegralMax_Con;
                                    else 
                                        PI_OffsetIntegral_DatReg <= resize(PI_OffsetIntegral_DatReg + unsigned(OffsetAdjustment_Nanosecond_DatReg), FactorSize_Con);
                                    end if;    
                                    PI_OffsetIntegralSign_DatReg <= '0';                           
                                end if;                           
                            end if;
                        end if;
                    
                    when P_St => 
                        PI_OffsetAdjustment_Nanosecond_DatReg <= PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16);
                        PI_OffsetMul_DatReg <= std_logic_vector(PI_OffsetIntegral_DatReg * OffsetFactorI_DatReg);
                        PI_OffsetState_StaReg <= I_St;
                    
                    when I_St => 
                        if ((PI_OffsetIntegralSign_DatReg = '1') and (PI_OffsetAdjustment_Sign_DatReg = '1')) then -- both negative
                            PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustment_Nanosecond_DatReg) + unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));    
                            PI_OffsetAdjustment_Sign_DatReg <= '1';
                        elsif ((PI_OffsetIntegralSign_DatReg = '1') and (PI_OffsetAdjustment_Sign_DatReg = '0')) then -- inversed
                            if (unsigned(PI_OffsetAdjustment_Nanosecond_DatReg) >= unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16))) then
                                PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustment_Nanosecond_DatReg) - unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));                                         
                                PI_OffsetAdjustment_Sign_DatReg <= '0';
                            else
                                PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) - unsigned(PI_OffsetAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));   
                                PI_OffsetAdjustment_Sign_DatReg <= '1';
                            end if;
                        elsif ((PI_OffsetIntegralSign_DatReg = '0') and (PI_OffsetAdjustment_Sign_DatReg = '1')) then -- inversed
                            if (unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) >= unsigned(PI_OffsetAdjustment_Nanosecond_DatReg)) then
                                PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) - unsigned(PI_OffsetAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));     
                                PI_OffsetAdjustment_Sign_DatReg <= '0';
                            else
                                PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustment_Nanosecond_DatReg) - unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));    
                                PI_OffsetAdjustment_Sign_DatReg <= '1';
                            end if;
                        elsif ((PI_OffsetIntegralSign_DatReg = '0') and (PI_OffsetAdjustment_Sign_DatReg = '0')) then -- both positive
                            PI_OffsetAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_OffsetAdjustment_Nanosecond_DatReg) + unsigned(PI_OffsetMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));    
                            PI_OffsetAdjustment_Sign_DatReg <= '0';
                        end if;
                        PI_OffsetAdjustment_ValReg <= '1';
                        PI_OffsetState_StaReg <= Idle_St;
                        
                    when others =>
                        PI_OffsetState_StaReg <= Idle_St;
                end case;
            else
                PI_OffsetIntegral_DatReg <= (others => '0');
                PI_OffsetIntegralSign_DatReg <= '0';
                PI_OffsetAdjustment_Nanosecond_DatReg <= (others=> '0');
                PI_OffsetAdjustment_Sign_DatReg <= '0';
                PI_OffsetAdjustment_ValReg <= '1';
                PI_OffsetState_StaReg <= Idle_St;
            end if;
        end if;
    end process PI_Offset_Prc;

    -- Calculate the PI servo drift correction
    PI_Drift_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            PI_DriftState_StaReg <= Idle_St;

            PI_DriftAdjustment_Nanosecond_DatReg <= (others=> '0');
            PI_DriftAdjustment_Sign_DatReg <= '0';
            PI_DriftAdjustment_Interval_DatReg <= (others => '0');
            PI_DriftAdjustment_ValReg <= '0';

            PI_DriftIntegral_DatReg <= (others => '0');
            PI_DriftIntegralSign_DatReg <= '0';
            PI_DriftMul_DatReg <= (others => '0');

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            PI_DriftAdjustment_ValReg <= '0';
            if (Enable_Ena = '1') then 
                case (PI_DriftState_StaReg) is 
                    
                    when Idle_St => 
                        if ((DriftAdjustment_ValReg = '1') and (DriftAdjustment_ValOldReg = '0')) then 
                            if (DriftAdjustmentInvalid_ValReg = '1') then -- error case during drift calculation
                                -- retain the old adjustments
                                PI_DriftState_StaReg <= Check_St;
                            else
                                PI_DriftAdjustment_Nanosecond_DatReg <= DriftAdjustment_Nanosecond_DatReg;
                                PI_DriftAdjustment_Sign_DatReg <= DriftAdjustment_Sign_DatReg;  
                                PI_DriftAdjustment_Interval_DatReg <= DriftAdjustment_Interval_DatReg;
                                PI_DriftMul_DatReg <= std_logic_vector(resize(unsigned(DriftAdjustment_Nanosecond_DatReg), FactorSize_Con) * DriftFactorP_DatReg);
                                PI_DriftState_StaReg <= P_St;
                                -- Add the calculated drift to the drift integral 
                                if ((DriftAdjustment_Sign_DatReg = '1') and (PI_DriftIntegralSign_DatReg = '1')) then -- both negative
                                    if ((PI_DriftIntegral_DatReg + unsigned(DriftAdjustment_Nanosecond_DatReg)) >= IntegralMax_Con) then
                                        PI_DriftIntegral_DatReg <= IntegralMax_Con;
                                    else 
                                        PI_DriftIntegral_DatReg <= resize(PI_DriftIntegral_DatReg + unsigned(DriftAdjustment_Nanosecond_DatReg), FactorSize_Con);
                                    end if;    
                                    PI_DriftIntegralSign_DatReg <= '1';                           
                                elsif ((DriftAdjustment_Sign_DatReg = '1') and (PI_DriftIntegralSign_DatReg = '0')) then -- inversed
                                    if (PI_DriftIntegral_DatReg >= unsigned(DriftAdjustment_Nanosecond_DatReg)) then
                                        PI_DriftIntegral_DatReg <= resize(PI_DriftIntegral_DatReg - unsigned(DriftAdjustment_Nanosecond_DatReg), FactorSize_Con);    
                                        PI_DriftIntegralSign_DatReg <= '0';                           
                                    else
                                        PI_DriftIntegral_DatReg <= resize(unsigned(DriftAdjustment_Nanosecond_DatReg) - PI_DriftIntegral_DatReg, FactorSize_Con);    
                                        PI_DriftIntegralSign_DatReg <= '1';                           
                                    end if;
                                elsif ((DriftAdjustment_Sign_DatReg = '0') and (PI_DriftIntegralSign_DatReg = '1')) then -- inversed
                                    if (unsigned(DriftAdjustment_Nanosecond_DatReg) >= PI_DriftIntegral_DatReg) then
                                        PI_DriftIntegral_DatReg <= resize(unsigned(DriftAdjustment_Nanosecond_DatReg) - PI_DriftIntegral_DatReg, FactorSize_Con);   
                                        PI_DriftIntegralSign_DatReg <= '0';                           
                                    else
                                        PI_DriftIntegral_DatReg <= resize(PI_DriftIntegral_DatReg - unsigned(DriftAdjustment_Nanosecond_DatReg), FactorSize_Con);   
                                        PI_DriftIntegralSign_DatReg <= '1';                           
                                    end if;                    
                                elsif ((DriftAdjustment_Sign_DatReg = '0') and (PI_DriftIntegralSign_DatReg = '0')) then -- both positive
                                    if ((PI_DriftIntegral_DatReg + unsigned(DriftAdjustment_Nanosecond_DatReg)) >= IntegralMax_Con) then
                                        PI_DriftIntegral_DatReg <= IntegralMax_Con;
                                    else 
                                        PI_DriftIntegral_DatReg <= resize(PI_DriftIntegral_DatReg + unsigned(DriftAdjustment_Nanosecond_DatReg), FactorSize_Con);
                                    end if;    
                                    PI_DriftIntegralSign_DatReg <= '0';                           
                                end if;                           
                            end if;
                        end if;

                    when P_St =>
                        PI_DriftAdjustment_Nanosecond_DatReg <= PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16);
                        PI_DriftMul_DatReg <= std_logic_vector(PI_DriftIntegral_DatReg * DriftFactorI_DatReg);
                        PI_DriftState_StaReg <= I_St;
                    
                    when I_St =>
                        if ((PI_DriftIntegralSign_DatReg = '1') and (PI_DriftAdjustment_Sign_DatReg = '1')) then -- both negative
                            PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftAdjustment_Nanosecond_DatReg) + unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));
                            PI_DriftAdjustment_Sign_DatReg <= '1';
                        elsif ((PI_DriftIntegralSign_DatReg = '1') and (PI_DriftAdjustment_Sign_DatReg = '0')) then -- inversed
                            if (unsigned(PI_DriftAdjustment_Nanosecond_DatReg) >= unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16))) then
                                PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftAdjustment_Nanosecond_DatReg) - unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));
                                PI_DriftAdjustment_Sign_DatReg <= '0';
                            else
                                PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) - unsigned(PI_DriftAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                PI_DriftAdjustment_Sign_DatReg <= '1';
                            end if;
                        elsif ((PI_DriftIntegralSign_DatReg = '0') and (PI_DriftAdjustment_Sign_DatReg = '1')) then -- inversed
                            if (unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) >= unsigned(PI_DriftAdjustment_Nanosecond_DatReg)) then
                                PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)) - unsigned(PI_DriftAdjustment_Nanosecond_DatReg), NanosecondWidth_Con));
                                PI_DriftAdjustment_Sign_DatReg <= '0';
                            else
                                PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftAdjustment_Nanosecond_DatReg) - unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));
                                PI_DriftAdjustment_Sign_DatReg <= '1';
                            end if;
                        elsif ((PI_DriftIntegralSign_DatReg = '0') and (PI_DriftAdjustment_Sign_DatReg = '0')) then -- both positive
                            PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(resize(unsigned(PI_DriftAdjustment_Nanosecond_DatReg) + unsigned(PI_DriftMul_DatReg(((NanosecondWidth_Con+16)-1) downto 16)), NanosecondWidth_Con));
                            PI_DriftAdjustment_Sign_DatReg <= '0';
                        end if;
                        PI_DriftState_StaReg <= Check_St;

                    when Check_St =>
                        if ((Sim_Gen = false) and (unsigned(PI_DriftAdjustment_Nanosecond_DatReg) > ClkCyclesInSecond_Con)) then
                            PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(to_unsigned(ClkCyclesInSecond_Con, NanosecondWidth_Con)); -- max possible
                            -- reset Servo
                            PI_DriftIntegral_DatReg <= (others => '0');
                            PI_DriftIntegralSign_DatReg <= '0';
                        elsif ((Sim_Gen = true) and (unsigned(PI_DriftAdjustment_Nanosecond_DatReg) > ((SecondNanoseconds_Con/10000)/ClockPeriod_Gen))) then
                            PI_DriftAdjustment_Nanosecond_DatReg <= std_logic_vector(to_unsigned(((SecondNanoseconds_Con/10000)/ClockPeriod_Gen), NanosecondWidth_Con)); -- max possible
                            -- reset Servo
                            PI_DriftIntegral_DatReg <= (others => '0');
                            PI_DriftIntegralSign_DatReg <= '0';
                        end if;
                        
                        PI_DriftAdjustment_ValReg <= '1';
                        PI_DriftState_StaReg <= Idle_St;

                    when others =>
                        PI_DriftState_StaReg <= Idle_St;
                
                end case;
            else
                -- reset Servo
                PI_DriftIntegral_DatReg <= (others => '0');
                PI_DriftIntegralSign_DatReg <= '0';
                PI_DriftAdjustment_Nanosecond_DatReg <= (others=> '0');
                PI_DriftAdjustment_Sign_DatReg <= '0';
                PI_DriftAdjustment_ValReg <= '1';
            end if;
        end if;
    end process PI_Drift_Prc;
    

    -- Access configuration and monitoring registers via an AXI4L slave
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress                            : std_logic_vector(31 downto 0) := (others => '0');    
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
                
            Axi_Init_Proc(PpsSlaveControl_Reg_Con, PpsSlaveControl_DatReg);
            Axi_Init_Proc(PpsSlaveStatus_Reg_Con, PpsSlaveStatus_DatReg);
            Axi_Init_Proc(PpsSlavePolarity_Reg_Con, PpsSlavePolarity_DatReg);
            Axi_Init_Proc(PpsSlaveVersion_Reg_Con, PpsSlaveVersion_DatReg);
            Axi_Init_Proc(PpsSlavePulseWidth_Reg_Con, PpsSlavePulseWidth_DatReg);
            Axi_Init_Proc(PpsSlaveCableDelay_Reg_Con, PpsSlaveCableDelay_DatReg);
            if (InputPolarity_Gen = true) then
                PpsSlavePolarity_DatReg(PpsSlavePolarity_PolarityBit_Con) <= '1';
            else
                PpsSlavePolarity_DatReg(PpsSlavePolarity_PolarityBit_Con) <= '0';
            end if;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
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
                        Axi_Read_Proc(PpsSlaveControl_Reg_Con, PpsSlaveControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsSlaveStatus_Reg_Con, PpsSlaveStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsSlavePolarity_Reg_Con, PpsSlavePolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsSlaveVersion_Reg_Con, PpsSlaveVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsSlavePulseWidth_Reg_Con, PpsSlavePulseWidth_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsSlaveCableDelay_Reg_Con, PpsSlaveCableDelay_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;    
                    end if;
                    
                when Write_St => 
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(PpsSlaveControl_Reg_Con, PpsSlaveControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsSlaveStatus_Reg_Con, PpsSlaveStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsSlavePolarity_Reg_Con, PpsSlavePolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsSlaveVersion_Reg_Con, PpsSlaveVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsSlavePulseWidth_Reg_Con, PpsSlavePulseWidth_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsSlaveCableDelay_Reg_Con, PpsSlaveCableDelay_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if; 
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or 
                        ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;    
                    end if;               
                    
                when others =>
                
            end case;  

            if (PpsSlaveControl_DatReg(PpsSlaveControl_EnableBit_Con) = '1') then
                if (PeriodError_DatReg = '1') then -- make it sticky
                    PpsSlaveStatus_DatReg(PpsSlaveStatus_PeriodErrorBit_Con) <= '1';
                end if;
                if (PulseWidthError_DatReg = '1') then -- make it sticky
                    PpsSlaveStatus_DatReg(PpsSlaveStatus_PulseWidthErrorBit_Con) <= '1';
                end if;
            else
                PpsSlaveStatus_DatReg(PpsSlaveStatus_PeriodErrorBit_Con) <= '0';
                PpsSlaveStatus_DatReg(PpsSlaveStatus_PulseWidthErrorBit_Con) <= '0';
            end if;
            
            PpsSlavePulseWidth_DatReg(9 downto 0) <= PulseWidth_DatReg;

        end if;
    end process Axi_Prc;

    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************

end architecture PpsSlave_Arch;