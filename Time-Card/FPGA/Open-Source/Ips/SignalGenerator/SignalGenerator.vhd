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
-- The Signal Generator is a full hardware (FPGA) only implementation that allows to     --
-- generate pulse width modulated (PWM) signals of configurable polarity aligned with    --
-- the local clock. The Signal Generator takes a start time, a pulse width and period as --
-- well as a repeat count as input and generates the signal accordingly. The settings    --
-- are configurable by an AXI4Light-Slave Register interface.                            --
-------------------------------------------------------------------------------------------
entity SignalGenerator is
    generic (
        ClockPeriod_Gen                             :       natural := 20;
        CableDelay_Gen                              :       boolean := false;
        OutputDelay_Gen                             :       integer := 100;
        OutputPolarity_Gen                          :       boolean := true;
        HighResFreqMultiply_Gen                     :       natural range 4 to 10 := 5
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
            
        -- Signal Output            
        SignalGenerator_EvtOut                      : out   std_logic;
                    
        -- Interrupt Output         
        Irq_EvtOut                                  : out   std_logic;
            
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
end entity SignalGenerator;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture SignalGenerator_Arch of SignalGenerator is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    -- High resolution constants
    constant HighResClockPeriod_Con                 : natural := (ClockPeriod_Gen/HighResFreqMultiply_Gen);
    constant RegOutputDelay_Con                     : natural := ((3 * ClockPeriod_Gen) + HighResClockPeriod_Con);
    -- The total output delay consists of 
    --     - the configurable output delay compensation(generic input), due to output registers
    --     - the cable delay compensation, provided by AXI reg, if enabled by the generic input
    --     - the internal register delay compensation for the clock domain crossing
    constant OutputDelaySum_Con                     : integer := (OutputDelay_Gen + (ClockPeriod_Gen/2) + RegOutputDelay_Con);
    
    -- Signal Generator version
    constant SigGenMajorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant SigGenMinorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant SigGenBuildVersion_Con                 : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant SigGenVersion_Con                      : std_logic_vector(31 downto 0) := SigGenMajorVersion_Con & SigGenMinorVersion_Con & SigGenBuildVersion_Con;
        
    -- AXI registers    
    constant SigGenControl_Reg_Con                  : Axi_Reg_Type:= (x"00000000", x"00000003", Rw_E, x"00000000");
    constant SigGenStatus_Reg_Con                   : Axi_Reg_Type:= (x"00000004", x"00000003", Wc_E, x"00000000");
    constant SigGenPolarity_Reg_Con                 : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant SigGenVersion_Reg_Con                  : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, SigGenVersion_Con);
    constant SigGenCableDelay_Reg_Con               : Axi_Reg_Type:= (x"00000020", x"0000FFFF", Rw_E, x"00000000");
    constant SigGenIrq_Reg_Con                      : Axi_Reg_Type:= (x"00000030", x"00000001", Wc_E, x"00000000");
    constant SigGenIrqMask_Reg_Con                  : Axi_Reg_Type:= (x"00000034", x"00000001", Rw_E, x"00000000");
    constant SigGenStartTimeValueL_Reg_Con          : Axi_Reg_Type:= (x"00000040", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenStartTimeValueH_Reg_Con          : Axi_Reg_Type:= (x"00000044", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenPulseWidthValueL_Reg_Con         : Axi_Reg_Type:= (x"00000048", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenPulseWidthValueH_Reg_Con         : Axi_Reg_Type:= (x"0000004C", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenPeriodValueL_Reg_Con             : Axi_Reg_Type:= (x"00000050", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenPeriodValueH_Reg_Con             : Axi_Reg_Type:= (x"00000054", x"FFFFFFFF", Rw_E, x"00000000");
    constant SigGenRepeatCount_Reg_Con              : Axi_Reg_Type:= (x"00000058", x"FFFFFFFF", Rw_E, x"00000000");
        
    constant SigGenControl_EnableBit_Con            : natural := 0;
    constant SigGenControl_SignalValBit_Con         : natural := 1;
    constant SigGenStatus_Error_Con                 : natural := 0;
    constant SigGenStatus_TimeJumpBit_Con           : natural := 1;
    constant SigGenPolarity_PolarityBit_Con         : natural := 0;
    constant SigGenIrq_RefInvalidBit_Con            : natural := 0;
    constant SigGenIrqMask_RefInvalidBit_Con        : natural := 0;
    
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type SigGenState_Type                             is (Idle_St, CheckTime_St, Generate_St);
    
    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    -- control and data signals
    signal Enable_Ena                               : std_logic;
    signal Signal_Val                               : std_logic;
    signal Polarity_Dat                             : std_logic;
    signal Error_EvtReg                             : std_logic;
    signal StartTime_Second_Dat                     : std_logic_vector((SecondWidth_Con-1) downto 0);   
    signal StartTime_Nanosecond_Dat                 : std_logic_vector((NanosecondWidth_Con-1) downto 0);   
    signal PulseWidth_Second_Dat                    : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal PulseWidth_Nanosecond_Dat                : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal Period_Second_Dat                        : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal Period_Nanosecond_Dat                    : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal RepeatCount_Dat                          : std_logic_vector(31 downto 0);
    signal CableDelay_Dat                           : std_logic_vector(15 downto 0);
                
    -- Time Input           
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_TimeJump_DatReg                : std_logic;
    signal ClockTime_ValReg                         : std_logic;

    -- count the high resolution ticks
    signal SignalShiftSysClk_DatReg                 : std_logic_vector((HighResFreqMultiply_Gen-1) downto 0);
    signal SignalShiftSysClk1_DatReg                : std_logic_vector((HighResFreqMultiply_Gen-1) downto 0);
    signal SignalShiftSysClkNx_DatReg               : std_logic_vector((2*HighResFreqMultiply_Gen-1) downto 0);
    
    signal Polarity_DatReg                          : std_logic;
    signal StartTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);   
    signal StartTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);   
    signal StopTime_Second_DatReg                   : std_logic_vector((SecondWidth_Con-1) downto 0);   
    signal StopTime_Nanosecond_DatReg               : std_logic_vector((NanosecondWidth_Con-1) downto 0);   
    signal PulseWidth_Second_DatReg                 : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal PulseWidth_Nanosecond_DatReg             : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal Period_Second_DatReg                     : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal Period_Nanosecond_DatReg                 : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal RepeatCount_DatReg                       : std_logic_vector(31 downto 0);
                
    -- number of generated pulses 
    signal PulseCount_CntReg                        : std_logic_vector(31 downto 0); 
    -- signal activated
    signal SignalActive_DatReg                      : std_logic;
                
    signal SigGenState_StaReg                       : SigGenState_Type;
                
    -- Axi signals
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;
    signal AxiWriteAddrReady_RdyReg                 : std_logic;
    signal AxiWriteDataReady_RdyReg                 : std_logic;
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);
    signal AxiReadAddrReady_RdyReg                  : std_logic;
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);
    
    signal SigGenControl_DatReg                     : std_logic_vector(31 downto 0);
    signal SigGenStatus_DatReg                      : std_logic_vector(31 downto 0);
    signal SigGenPolarity_DatReg                    : std_logic_vector(31 downto 0);
    signal SigGenVersion_DatReg                     : std_logic_vector(31 downto 0);
    signal SigGenCableDelay_DatReg                  : std_logic_vector(31 downto 0);
    signal SigGenIrq_DatReg                         : std_logic_vector(31 downto 0);
    signal SigGenIrqMask_DatReg                     : std_logic_vector(31 downto 0);
    signal SigGenStartTimeValueL_DatReg             : std_logic_vector(31 downto 0);
    signal SigGenStartTimeValueH_DatReg             : std_logic_vector(31 downto 0);
    signal SigGenPulseWidthValueL_DatReg            : std_logic_vector(31 downto 0);
    signal SigGenPulseWidthValueH_DatReg            : std_logic_vector(31 downto 0);
    signal SigGenPeriodValueL_DatReg                : std_logic_vector(31 downto 0);
    signal SigGenPeriodValueH_DatReg                : std_logic_vector(31 downto 0);
    signal SigGenRepeatCount_DatReg                 : std_logic_vector(31 downto 0);
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    StartTime_Nanosecond_Dat                        <= SigGenStartTimeValueL_DatReg;
    StartTime_Second_Dat                            <= SigGenStartTimeValueH_DatReg;
    PulseWidth_Nanosecond_Dat                       <= SigGenPulseWidthValueL_DatReg;
    PulseWidth_Second_Dat                           <= SigGenPulseWidthValueH_DatReg;
    Period_Nanosecond_Dat                           <= SigGenPeriodValueL_DatReg;
    Period_Second_Dat                               <= SigGenPeriodValueH_DatReg;
    RepeatCount_Dat                                 <= SigGenRepeatCount_DatReg;
    Signal_Val                                      <= SigGenControl_DatReg(SigGenControl_SignalValBit_Con);
    Polarity_Dat                                    <= SigGenPolarity_DatReg(SigGenPolarity_PolarityBit_Con);
    CableDelay_Dat                                  <= SigGenCableDelay_DatReg(15 downto 0) when CableDelay_Gen = true else 
                                                       (others => '0');
    Irq_EvtOut                                      <= SigGenIrq_DatReg(SigGenIrq_RefInvalidBit_Con) and SigGenIrqMask_DatReg(SigGenIrqMask_RefInvalidBit_Con);

    AxiWriteAddrReady_RdyOut                        <= AxiWriteAddrReady_RdyReg;
    AxiWriteDataReady_RdyOut                        <= AxiWriteDataReady_RdyReg;
    AxiWriteRespValid_ValOut                        <= AxiWriteRespValid_ValReg;
    AxiWriteRespResponse_DatOut                     <= AxiWriteRespResponse_DatReg;
    AxiReadAddrReady_RdyOut                         <= AxiReadAddrReady_RdyReg;
    AxiReadDataValid_ValOut                         <= AxiReadDataValid_ValReg;
    AxiReadDataResponse_DatOut                      <= AxiReadDataResponse_DatReg;
    AxiReadDataData_DatOut                          <= AxiReadDataData_DatReg;

    Enable_Ena                                      <= SigGenControl_DatReg(SigGenControl_EnableBit_Con);

    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    -- The shift register that indicates how many high resolution clock periods could 'fit' between the start/stop time and the current time is passed to the high resolution clock domain
    -- Generate the signal based on the compensated delays of the shift register
    SysClkNxReg_Prc : process(SysClkNx_ClkIn) is
    begin
        if ((SysClkNx_ClkIn'event) and (SysClkNx_ClkIn = '1')) then
            SignalShiftSysClk1_DatReg <= SignalShiftSysClk_DatReg;
            if (SignalShiftSysClk_DatReg /= SignalShiftSysClk1_DatReg) then
                SignalShiftSysClkNx_DatReg <= SignalShiftSysClkNx_DatReg(((HighResFreqMultiply_Gen*2)-2) downto (HighResFreqMultiply_Gen-1)) & SignalShiftSysClk_DatReg; -- copy the high resolution clock periods
            else
                SignalShiftSysClkNx_DatReg <= SignalShiftSysClkNx_DatReg(((HighResFreqMultiply_Gen*2)-2) downto 0) & SignalShiftSysClkNx_DatReg(0); -- retain the last value
            end if;
            if (Polarity_DatReg = '1') then
                SignalGenerator_EvtOut <= SignalShiftSysClkNx_DatReg((HighResFreqMultiply_Gen*2)-1);
            else
                SignalGenerator_EvtOut <= not SignalShiftSysClkNx_DatReg((HighResFreqMultiply_Gen*2)-1);
            end if;
        end if;
    end process SysClkNxReg_Prc;
    
    -- When the Signal Generator is enabled and the new configuration registers are set, the StartTime and StopTime are calculated, as: 
    -- StartTime<= StartTime-Delays and StopTime=StartTime+PulseWidth. 
    -- If the StartTime is reached (equal or bigger) the pulse is asserted to the configured polarity and a new start time is calculated by adding to the current start time the signal period. 
    -- If the stop time is reached (equal or bigger) the pulse is asserted to the inverse of the configured polarity, the new stop time is calculated by adding the period and a pulse counter gets incremented. 
    -- This start/stop procedure is repeated until the pulse count is reached (continuously repetition if the pulse count is '0'). 
    -- The pulse generation is disabled if a) the repetition limit is reached b)a time jump happens, c)the initial start time is set to the past (no signal generation) d)the input clock is not active
    -- Additionally, in order to increase the accuracy of the generation, when the start/stop time is reached, a shift register indicates how many high resolution clock periods could 'fit' between the start/stop time and the current time.
    SignalGenerator_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin   
        if (SysRstN_RstIn = '0') then
            SigGenState_StaReg <= Idle_St;
            
            ClockTime_Second_DatReg <= (others => '0');    
            ClockTime_Nanosecond_DatReg <= (others => '0');
            ClockTime_TimeJump_DatReg <= '0';  
            ClockTime_ValReg <= '0';           
            
            Error_EvtReg <= '0';
            SignalActive_DatReg <= '0';
            Polarity_DatReg <= '1';
            PulseCount_CntReg <= (others => '0');
            StartTime_Second_DatReg <= (others => '0');    
            StartTime_Nanosecond_DatReg <= (others => '0');
            PulseWidth_Second_DatReg <= (others => '0');   
            PulseWidth_Nanosecond_DatReg <= (others => '0');
            Period_Second_DatReg <= (others => '0');       
            Period_Nanosecond_DatReg <= (others => '0');   
            RepeatCount_DatReg <= (others => '0');         
            SignalShiftSysClk_DatReg <= (others => '0');
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            Error_EvtReg <= '0';
            
            ClockTime_TimeJump_DatReg <= ClockTime_TimeJump_DatIn;
            ClockTime_ValReg <= ClockTime_ValIn;       
            if (CableDelay_Gen = false) then 
                ClockTime_Second_DatReg <= ClockTime_Second_DatIn;
                ClockTime_Nanosecond_DatReg <= ClockTime_Nanosecond_DatIn;
            else
                if ((unsigned(ClockTime_Nanosecond_DatIn) + unsigned(CableDelay_Dat)) < SecondNanoseconds_Con) then 
                    ClockTime_Second_DatReg <= ClockTime_Second_DatIn;
                    ClockTime_Nanosecond_DatReg <= std_logic_vector(unsigned(ClockTime_Nanosecond_DatIn) + unsigned(CableDelay_Dat));
                else
                    ClockTime_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatIn) + 1);
                    ClockTime_Nanosecond_DatReg <= std_logic_vector((unsigned(ClockTime_Nanosecond_DatIn) + unsigned(CableDelay_Dat)) - SecondNanoseconds_Con);
                end if;
            end if;
            
            if (Enable_Ena = '1') then 
                if (Signal_Val = '1') then -- update the configuration immediately
                    if ((unsigned(StartTime_Nanosecond_Dat)) >= (to_unsigned(OutputDelaySum_Con, NanosecondWidth_Con))) then     
                        StartTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StartTime_Nanosecond_Dat) - to_unsigned(OutputDelaySum_Con, NanosecondWidth_Con));
                        StartTime_Second_DatReg <= StartTime_Second_Dat;
                    else
                        StartTime_Nanosecond_DatReg <= std_logic_vector((unsigned(StartTime_Nanosecond_Dat) + SecondNanoseconds_Con) - to_unsigned(OutputDelaySum_Con, NanosecondWidth_Con));
                        StartTime_Second_DatReg <= std_logic_vector(unsigned(StartTime_Second_Dat) - 1);
                    end if;
                    
                    Polarity_DatReg <= Polarity_Dat;
                    PulseCount_CntReg <= (others => '0');
                    PulseWidth_Second_DatReg <= PulseWidth_Second_Dat;   
                    PulseWidth_Nanosecond_DatReg <= PulseWidth_Nanosecond_Dat;
                    Period_Second_DatReg <= Period_Second_Dat;       
                    Period_Nanosecond_DatReg <= Period_Nanosecond_Dat;   
                    RepeatCount_DatReg <= RepeatCount_Dat;         
                    SignalActive_DatReg <= '0';
                    SignalShiftSysClk_DatReg <= (others => '0');
                    
                    SigGenState_StaReg <= CheckTime_St;
                    
                else
                    case SigGenState_StaReg is 
                        when Idle_St => -- wait for a new configuration
                            Polarity_DatReg <= '1';
                            PulseCount_CntReg <= (others => '0');
                            StartTime_Second_DatReg <= (others => '0');    
                            StartTime_Nanosecond_DatReg <= (others => '0');
                            PulseWidth_Second_DatReg <= (others => '0');   
                            PulseWidth_Nanosecond_DatReg <= (others => '0');
                            Period_Second_DatReg <= (others => '0');   
                            Period_Nanosecond_DatReg <= (others => '0');   
                            RepeatCount_DatReg <= (others => '0');     
                            SignalActive_DatReg <= '0';
                            SignalShiftSysClk_DatReg <= (others => '0');
                            
                            SigGenState_StaReg <= Idle_St; 
                        
                        when CheckTime_St => -- validate the new configuration
                            if ((ClockTime_TimeJump_DatReg = '0') and (ClockTime_ValReg = '1') and
                                ((unsigned(StartTime_Second_DatReg) > unsigned(ClockTime_Second_DatReg)) or 
                                 ((StartTime_Second_DatReg = ClockTime_Second_DatReg) and (unsigned(StartTime_Nanosecond_DatReg) > unsigned(ClockTime_Nanosecond_DatReg))))) then 
                               
                               if ((unsigned(StartTime_Nanosecond_DatReg) + unsigned(PulseWidth_Nanosecond_DatReg)) < SecondNanoseconds_Con) then 
                                    StopTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StartTime_Nanosecond_DatReg) + unsigned(PulseWidth_Nanosecond_DatReg));
                                    StopTime_Second_DatReg <= std_logic_vector(unsigned(StartTime_Second_DatReg) + unsigned(PulseWidth_Second_DatReg));
                                else
                                    StopTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StartTime_Nanosecond_DatReg) + unsigned(PulseWidth_Nanosecond_DatReg) - SecondNanoseconds_Con);
                                    StopTime_Second_DatReg <= std_logic_vector(unsigned(StartTime_Second_DatReg) + unsigned(PulseWidth_Second_DatReg) + 1);
                                end if;
                                
                                SigGenState_StaReg <= Generate_St; 
                                
                            else
                                Error_EvtReg <= '1';
                                SignalShiftSysClk_DatReg <= (others => '0');
                                SigGenState_StaReg <= Idle_St;
                            end if;    
                            
                        when Generate_St => -- generate the configured signal
                            if ((ClockTime_TimeJump_DatReg = '0') and (ClockTime_ValReg = '1')) then  
                                if ((unsigned(RepeatCount_DatReg) = 0) or (unsigned(PulseCount_CntReg) < unsigned(RepeatCount_DatReg))) then 
                                    -- if current time >= start time
                                    if ((unsigned(ClockTime_Second_DatReg) > unsigned(StartTime_Second_DatReg)) or 
                                        ((ClockTime_Second_DatReg = StartTime_Second_DatReg) and (unsigned(ClockTime_Nanosecond_DatReg) >= unsigned(StartTime_Nanosecond_DatReg)))) then 
                                        -- calculate next start point
                                       if ((unsigned(StartTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg)) < SecondNanoseconds_Con) then 
                                            StartTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StartTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg));
                                            StartTime_Second_DatReg <= std_logic_vector(unsigned(StartTime_Second_DatReg) + unsigned(Period_Second_DatReg));
                                        else
                                            StartTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StartTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg) - SecondNanoseconds_Con);
                                            StartTime_Second_DatReg <= std_logic_vector(unsigned(StartTime_Second_DatReg) + unsigned(Period_Second_DatReg) + 1);
                                        end if;
                                        
                                        SignalActive_DatReg <= '1';
                                        
                                        if (SignalActive_DatReg /= '1') then 
                                            SignalShiftSysClk_DatReg <= (others => '0');
                                            for i in 0 to (HighResFreqMultiply_Gen-1) loop
                                                -- if we are at the point where we need to provide the shift register pattern
                                                if ((unsigned(StartTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)) < SecondNanoseconds_Con) then 
                                                    if ((unsigned(ClockTime_Second_DatReg) > unsigned(StartTime_Second_DatReg)) or 
                                                        ((unsigned(ClockTime_Second_DatReg) = unsigned(StartTime_Second_DatReg)) and 
                                                         (unsigned(ClockTime_Nanosecond_DatReg) >= ((unsigned(StartTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)))))) then 
                                                        SignalShiftSysClk_DatReg(i) <= '1';
                                                    end if;
                                                else
                                                    if ((unsigned(ClockTime_Second_DatReg) > (unsigned(StartTime_Second_DatReg) + 1)) or 
                                                        ((unsigned(ClockTime_Second_DatReg) = (unsigned(StartTime_Second_DatReg) + 1)) and 
                                                         (unsigned(ClockTime_Nanosecond_DatReg) >= ((unsigned(StartTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)) - SecondNanoseconds_Con))) ) then 
                                                        SignalShiftSysClk_DatReg(i) <= '1';
                                                    end if;
                                                end if;
                                                        
                                            end loop;    
                                        else
                                            SignalShiftSysClk_DatReg <= (others => '0');
                                        end if;
                                    -- if current time >= stop time
                                    elsif ((unsigned(ClockTime_Second_DatReg) > unsigned(StopTime_Second_DatReg)) or 
                                           ((ClockTime_Second_DatReg = StopTime_Second_DatReg) and (unsigned(ClockTime_Nanosecond_DatReg) >= unsigned(StopTime_Nanosecond_DatReg)))) then 
                                        
                                        if (unsigned(RepeatCount_DatReg) /= 0) then
                                            PulseCount_CntReg <= std_logic_vector(unsigned(PulseCount_CntReg) + 1);
                                        end if;
                                        
                                        -- calculate next stop point
                                       if ((unsigned(StopTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg)) < SecondNanoseconds_Con) then 
                                            StopTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StopTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg));
                                            StopTime_Second_DatReg <= std_logic_vector(unsigned(StopTime_Second_DatReg) + unsigned(Period_Second_DatReg));
                                        else
                                            StopTime_Nanosecond_DatReg <= std_logic_vector(unsigned(StopTime_Nanosecond_DatReg) + unsigned(Period_Nanosecond_DatReg) - SecondNanoseconds_Con);
                                            StopTime_Second_DatReg <= std_logic_vector(unsigned(StopTime_Second_DatReg) + unsigned(Period_Second_DatReg) + 1);
                                        end if;
                                        
                                        SignalActive_DatReg <= '0';
                                        
                                        if (SignalActive_DatReg = '1') then 
                                            SignalShiftSysClk_DatReg <= (others=>'1');
                                            for i in 0 to (HighResFreqMultiply_Gen-1) loop
                                                -- if we are at the point where we need to provide the shift register pattern
                                                if ((unsigned(StopTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)) < SecondNanoseconds_Con) then 
                                                    if ((unsigned(ClockTime_Second_DatReg) > unsigned(StopTime_Second_DatReg)) or 
                                                        ((unsigned(ClockTime_Second_DatReg) = unsigned(StopTime_Second_DatReg)) and 
                                                         (unsigned(ClockTime_Nanosecond_DatReg) >= ((unsigned(StopTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)))))) then 
                                                        SignalShiftSysClk_DatReg(i) <= '0';
                                                    end if;
                                                else
                                                    if ((unsigned(ClockTime_Second_DatReg) > (unsigned(StopTime_Second_DatReg) + 1)) or 
                                                        ((unsigned(ClockTime_Second_DatReg) = (unsigned(StopTime_Second_DatReg) + 1)) and 
                                                         (unsigned(ClockTime_Nanosecond_DatReg) >= ((unsigned(StopTime_Nanosecond_DatReg) + to_unsigned((i*HighResClockPeriod_Con), NanosecondWidth_Con)) - SecondNanoseconds_Con))) ) then 
                                                        SignalShiftSysClk_DatReg(i) <= '0';
                                                    end if;
                                                end if;
                                            end loop;    
                                        else
                                            SignalShiftSysClk_DatReg <= (others => '0');
                                        end if;
                                    end if;
                                    
                                else
                                    SignalShiftSysClk_DatReg <= (others => '0');
                                    SigGenState_StaReg <= Idle_St;
                                end if;
                            else
                                Error_EvtReg <= '1';
                                SignalShiftSysClk_DatReg <= (others => '0');
                                SigGenState_StaReg <= Idle_St;
                            end if;
                        
                        when others =>
                            SigGenState_StaReg <= Idle_St;
                        
                    end case;
                end if;
            else
                SignalActive_DatReg <= '0';
                SignalShiftSysClk_DatReg <= (others => '0');
                SigGenState_StaReg <= Idle_St;
            end if;
        end if;
    end process SignalGenerator_Prc;
    
    -- AXI register access
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

            Axi_Init_Proc(SigGenControl_Reg_Con, SigGenControl_DatReg);
            Axi_Init_Proc(SigGenStatus_Reg_Con, SigGenStatus_DatReg);
            Axi_Init_Proc(SigGenPolarity_Reg_Con, SigGenPolarity_DatReg);
            if (OutputPolarity_Gen = true) then
                SigGenPolarity_DatReg(SigGenPolarity_PolarityBit_Con) <= '1';
            else
                SigGenPolarity_DatReg(SigGenPolarity_PolarityBit_Con) <= '0';
            end if;
            Axi_Init_Proc(SigGenCableDelay_Reg_Con, SigGenCableDelay_DatReg);
            Axi_Init_Proc(SigGenVersion_Reg_Con, SigGenVersion_DatReg);
            Axi_Init_Proc(SigGenIrq_Reg_Con, SigGenIrq_DatReg);
            Axi_Init_Proc(SigGenIrqMask_Reg_Con, SigGenIrqMask_DatReg);
            Axi_Init_Proc(SigGenStartTimeValueL_Reg_Con, SigGenStartTimeValueL_DatReg);
            Axi_Init_Proc(SigGenStartTimeValueH_Reg_Con, SigGenStartTimeValueH_DatReg);
            Axi_Init_Proc(SigGenPulseWidthValueL_Reg_Con, SigGenPulseWidthValueL_DatReg);
            Axi_Init_Proc(SigGenPulseWidthValueH_Reg_Con, SigGenPulseWidthValueH_DatReg);
            Axi_Init_Proc(SigGenPeriodValueL_Reg_Con, SigGenPeriodValueL_DatReg);
            Axi_Init_Proc(SigGenPeriodValueH_Reg_Con, SigGenPeriodValueH_DatReg);
            Axi_Init_Proc(SigGenRepeatCount_Reg_Con, SigGenRepeatCount_DatReg);

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
                        Axi_Read_Proc(SigGenControl_Reg_Con, SigGenControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenStatus_Reg_Con, SigGenStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenPolarity_Reg_Con, SigGenPolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        if (CableDelay_Gen = true) then
                            Axi_Read_Proc(SigGenCableDelay_Reg_Con, SigGenCableDelay_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        end if;
                        Axi_Read_Proc(SigGenVersion_Reg_Con, SigGenVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenIrq_Reg_Con, SigGenIrq_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenIrqMask_Reg_Con, SigGenIrqMask_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenStartTimeValueL_Reg_Con, SigGenStartTimeValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenStartTimeValueH_Reg_Con, SigGenStartTimeValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenPulseWidthValueL_Reg_Con, SigGenPulseWidthValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenPulseWidthValueH_Reg_Con, SigGenPulseWidthValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenPeriodValueL_Reg_Con, SigGenPeriodValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenPeriodValueH_Reg_Con, SigGenPeriodValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(SigGenRepeatCount_Reg_Con, SigGenRepeatCount_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                    
                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(SigGenControl_Reg_Con, SigGenControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenStatus_Reg_Con, SigGenStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenPolarity_Reg_Con, SigGenPolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        if (CableDelay_Gen = true) then
                            Axi_Write_Proc(SigGenCableDelay_Reg_Con, SigGenCableDelay_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        end if;
                        Axi_Write_Proc(SigGenVersion_Reg_Con, SigGenVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenIrq_Reg_Con, SigGenIrq_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenIrqMask_Reg_Con, SigGenIrqMask_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenStartTimeValueL_Reg_Con, SigGenStartTimeValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenStartTimeValueH_Reg_Con, SigGenStartTimeValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenPulseWidthValueL_Reg_Con, SigGenPulseWidthValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenPulseWidthValueH_Reg_Con, SigGenPulseWidthValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenPeriodValueL_Reg_Con, SigGenPeriodValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenPeriodValueH_Reg_Con, SigGenPeriodValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(SigGenRepeatCount_Reg_Con, SigGenRepeatCount_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                        ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;
                    
                when others =>
                
            end case;

            if (SigGenControl_DatReg(SigGenControl_EnableBit_Con) = '1') then
                if (Error_EvtReg = '1') then
                    SigGenStatus_DatReg(SigGenStatus_Error_Con) <= '1';
                end if;
                if ((ClockTime_TimeJump_DatIn = '1') or (ClockTime_ValIn = '0')) then
                    SigGenStatus_DatReg(SigGenStatus_TimeJumpBit_Con) <= '1';
                end if;
            else
                SigGenStatus_DatReg(SigGenStatus_Error_Con) <= '0';
                SigGenStatus_DatReg(SigGenStatus_TimeJumpBit_Con) <= '0';
            end if;

            if (SigGenControl_DatReg(SigGenControl_SignalValBit_Con) = '1') then
                SigGenControl_DatReg(SigGenControl_SignalValBit_Con) <= '0';
            end if;

            if (CableDelay_Gen = false) then
                Axi_Init_Proc(SigGenCableDelay_Reg_Con, SigGenCableDelay_DatReg);
            end if;

            if ((SigGenControl_DatReg(SigGenControl_EnableBit_Con) = '1') and
                (SigGenIrq_DatReg(SigGenIrq_RefInvalidBit_Con) = '0') and (SigGenIrqMask_DatReg(SigGenIrqMask_RefInvalidBit_Con) = '1') and
                ((ClockTime_TimeJump_DatIn = '1') or (ClockTime_ValReg /= ClockTime_ValIn) or (Error_EvtReg = '1'))) then
                SigGenIrq_DatReg(SigGenIrq_RefInvalidBit_Con) <= '1';
            end if;

        end if;
    end process Axi_Prc;


end architecture SignalGenerator_Arch;