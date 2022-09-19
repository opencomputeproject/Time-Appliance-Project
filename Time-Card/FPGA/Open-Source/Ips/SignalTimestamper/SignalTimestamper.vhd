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
-- The SignalTimestamper timestamps an event signal of configurable polarity.            --
-- Timestamps are taken on the configured edge of the signal and optional interrupts     -- 
-- are generated. The reference time for the timestamp is provided as input and the      --
-- delays of the timestamps are compensated. The Signal Timestamper is intended to be    --
-- connected to a CPU or any other AXI master that can read out the timestamps. The      --
-- settings are configured by an AXI4Light-Slave Register interface.                     -- 
-------------------------------------------------------------------------------------------
entity SignalTimestamper is
    generic (
        ClockPeriod_Gen                             :       natural := 20;   
        CableDelay_Gen                              :       boolean := false;
        InputDelay_Gen                              :       natural := 100;
        InputPolarity_Gen                           :       boolean := true;
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
        ClockTime_TimeJump_DatIn                    : in    std_logic; -- unused
        ClockTime_ValIn                             : in    std_logic;
            
        -- Timestamp Event Input            
        SignalTimestamper_EvtIn                     : in    std_logic;
                            
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
end entity SignalTimestamper;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture SignalTimestamper_Arch of SignalTimestamper is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    -- Timestamper Version 
    constant TimestamperMajorVersion_Con            : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant TimestamperMinorVersion_Con            : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant TimestamperBuildVersion_Con            : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant TimestamperVersion_Con                 : std_logic_vector(31 downto 0) := TimestamperMajorVersion_Con & TimestamperMinorVersion_Con & TimestamperBuildVersion_Con;
        
    -- AXI Registers        
    constant TimestamperControl_Reg_Con             : Axi_Reg_Type:= (x"00000000", x"00000001", Rw_E, x"00000000");
    constant TimestamperStatus_Reg_Con              : Axi_Reg_Type:= (x"00000004", x"00000003", Wc_E, x"00000000");
    constant TimestamperPolarity_Reg_Con            : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant TimestamperVersion_Reg_Con             : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, TimestamperVersion_Con);
    constant TimestamperCableDelay_Reg_Con          : Axi_Reg_Type:= (x"00000020", x"0000FFFF", Rw_E, x"00000000");
    constant TimestamperIrq_Reg_Con                 : Axi_Reg_Type:= (x"00000030", x"00000001", Wc_E, x"00000000");
    constant TimestamperIrqMask_Reg_Con             : Axi_Reg_Type:= (x"00000034", x"00000001", Rw_E, x"00000000");
    constant TimestamperEvtCount_Reg_Con            : Axi_Reg_Type:= (x"00000038", x"FFFFFFFF", Ro_E, x"00000000");
    constant TimestamperCount_Reg_Con               : Axi_Reg_Type:= (x"00000040", x"FFFFFFFF", Ro_E, x"00000000");
    constant TimestamperTimeValueL_Reg_Con          : Axi_Reg_Type:= (x"00000044", x"FFFFFFFF", Ro_E, x"00000000");
    constant TimestamperTimeValueH_Reg_Con          : Axi_Reg_Type:= (x"00000048", x"FFFFFFFF", Ro_E, x"00000000");
    constant TimestamperDataWidth_Reg_Con           : Axi_Reg_Type:= (x"0000004C", x"FFFFFFFF", Ro_E, x"00000000"); -- unused
    constant TimestamperData_Reg_Con                : Axi_Reg_Type:= (x"00000050", x"FFFFFFFF", Ro_E, x"00000000"); -- unused

    constant TimestamperControl_EnableBit_Con       : natural := 0;  
    constant TimestamperStatus_DropBit_Con          : natural := 0;  
    constant TimestamperPolarity_PolarityBit_Con    : natural := 0;
    constant TimestamperIrq_TimestampBit_Con        : natural := 0;
    constant TimestamperIrqMask_TimestampBit_Con    : natural := 0;

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Enable_Ena                               : std_logic;
    signal SignalTimestamper_Evt                    : std_logic;
    signal Timestamp_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal Timestamp_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal Timestamp_ValReg                         : std_logic;
    signal SignalCableDelay_Dat                     : std_logic_vector(15 downto 0);
    signal SignalPolarity_Dat                       : std_logic;
    signal RegisterDelay_DatReg                     : integer range 0 to (3*ClockPeriod_Gen);
    signal Count_CntReg                             : std_logic_vector(31 downto 0);
                                                    
    -- Time Input           
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_ValReg                         : std_logic;
    
    signal TimestampSysClkNx1_EvtReg                : std_logic := '0';
    signal TimestampSysClkNx2_EvtReg                : std_logic := '0';
    signal TimestampSysClkNx_EvtShiftReg            : std_logic_vector(HighResFreqMultiply_Gen*2-1 downto 0) := (others => '0');
    signal TimestampSysClk1_EvtReg                  : std_logic := '0';
    signal TimestampSysClk2_EvtReg                  : std_logic := '0';
    signal TimestampSysClk3_EvtReg                  : std_logic := '0';
    signal TimestampSysClk4_EvtReg                  : std_logic := '0';
    signal TimestampSysClk_EvtShiftReg              : std_logic_vector(HighResFreqMultiply_Gen*2-1 downto 0) := (others => '0');
                                                    
    -- Axi Signals                                  
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;
                                                    
    signal AxiWriteAddrReady_RdyReg                 : std_logic;       
    signal AxiWriteDataReady_RdyReg                 : std_logic;   
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);   
    signal AxiReadAddrReady_RdyReg                  : std_logic;          
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);
                
    signal TimestamperControl_DatReg                : std_logic_vector(31 downto 0);
    signal TimestamperStatus_DatReg                 : std_logic_vector(31 downto 0);
    signal TimestamperPolarity_DatReg               : std_logic_vector(31 downto 0);
    signal TimestamperVersion_DatReg                : std_logic_vector(31 downto 0);
    signal TimestamperCableDelay_DatReg             : std_logic_vector(31 downto 0);
    signal TimestamperIrq_DatReg                    : std_logic_vector(31 downto 0);
    signal TimestamperIrqMask_DatReg                : std_logic_vector(31 downto 0);
    signal TimestamperEvtCount_DatReg               : std_logic_vector(31 downto 0);
    signal TimestamperCount_DatReg                  : std_logic_vector(31 downto 0);
    signal TimestamperTimeValueL_DatReg             : std_logic_vector(31 downto 0);
    signal TimestamperTimeValueH_DatReg             : std_logic_vector(31 downto 0);
            
    signal TimestamperDataWidth_DatReg              : std_logic_vector(31 downto 0); -- unused
    signal TimestamperData_DatReg                   : std_logic_vector(31 downto 0); -- unused

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    Irq_EvtOut                                      <= TimestamperIrq_DatReg(TimestamperIrq_TimestampBit_Con) and TimestamperIrqMask_DatReg(TimestamperIrqMask_TimestampBit_Con);
                                            
    Enable_Ena                                      <= TimestamperControl_DatReg(TimestamperControl_EnableBit_Con);
    SignalCableDelay_Dat                            <= TimestamperCableDelay_DatReg(15 downto 0) when (CableDelay_Gen = true) else 
                                                       (others => '0');
    SignalPolarity_Dat                              <= TimestamperPolarity_DatReg(TimestamperPolarity_PolarityBit_Con);
                        
    SignalTimestamper_Evt                           <= SignalTimestamper_EvtIn when (SignalPolarity_Dat = '1') else
                                                       (not SignalTimestamper_EvtIn);
                        
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
            TimestampSysClkNx1_EvtReg <= SignalTimestamper_Evt;
            TimestampSysClkNx2_EvtReg <= TimestampSysClkNx1_EvtReg;
            TimestampSysClkNx_EvtShiftReg <= TimestampSysClkNx_EvtShiftReg((HighResFreqMultiply_Gen*2-2) downto 0) & TimestampSysClkNx2_EvtReg;
        end if;
    end process SysClkNxReg_Prc;
    
    -- Copy the event shift register of the high resolution clock domain to the system clock domain
    SysClkReg_Prc : process(SysClk_ClkIn) is
    begin
        if ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then 
            TimestampSysClk1_EvtReg <= SignalTimestamper_Evt;
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
    Timestamp_Prc: process(SysClk_ClkIn, SysRstN_RstIn) 
    begin
        if (SysRstN_RstIn = '0') then 
            Timestamp_ValReg <= '0';           
            Timestamp_Second_DatReg <= (others => '0');               
            Timestamp_Nanosecond_DatReg <= (others => '0');
            RegisterDelay_DatReg <= 0;
            Count_CntReg <= (others => '0');
            ClockTime_Second_DatReg <= (others => '0');
            ClockTime_Nanosecond_DatReg <= (others => '0');
            ClockTime_ValReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            -- single pulse
            Timestamp_ValReg <= '0';
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
            
            -- Compensate the timestamp delays. Ensure that the Nanosecond field does not underflow.
            if (TimestampSysClk3_EvtReg = '1' and TimestampSysClk4_EvtReg = '0') then 
                Count_CntReg <= std_logic_vector(unsigned(Count_CntReg) + 1);
                Timestamp_ValReg <= '1';
                if (ClockTime_ValReg = '0') then 
                    Timestamp_ValReg <= '0';           
                    Timestamp_Second_DatReg <= (others => '0');               
                    Timestamp_Nanosecond_DatReg <= (others => '0');
                else
                    if (to_integer(unsigned(ClockTime_Nanosecond_DatReg)) < InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(SignalCableDelay_Dat))) then -- smaller than 0
                        Timestamp_Nanosecond_DatReg <= std_logic_vector(to_unsigned((SecondNanoseconds_Con + 
                                                       to_integer(unsigned(ClockTime_Nanosecond_DatReg)) - 
                                                       (InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(SignalCableDelay_Dat)))), NanosecondWidth_Con));
                        Timestamp_Second_DatReg <= std_logic_vector(unsigned(ClockTime_Second_DatReg) - 1); 
                    else -- larger than/equal to 0
                        Timestamp_Nanosecond_DatReg <= std_logic_vector(to_unsigned((to_integer(unsigned(ClockTime_Nanosecond_DatReg)) - 
                                                       (InputDelay_Gen + RegisterDelay_DatReg + to_integer(unsigned(SignalCableDelay_Dat)))), NanosecondWidth_Con));
                        Timestamp_Second_DatReg <= ClockTime_Second_DatReg; 
                    end if;
                end if;        
            end if;
            
            if (Enable_Ena = '0') then 
                Timestamp_ValReg <= '0';           
                Timestamp_Second_DatReg <= (others => '0');               
                Timestamp_Nanosecond_DatReg <= (others => '0');
                Count_CntReg <= (others => '0');
            end if;
        end if;
    end process Timestamp_Prc;
    
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
            
            Axi_Init_Proc(TimestamperControl_Reg_Con, TimestamperControl_DatReg);
            Axi_Init_Proc(TimestamperStatus_Reg_Con, TimestamperStatus_DatReg); -- unused
            Axi_Init_Proc(TimestamperPolarity_Reg_Con, TimestamperPolarity_DatReg);
            if (InputPolarity_Gen = true) then
                TimestamperPolarity_DatReg(TimestamperPolarity_PolarityBit_Con) <= '1';
            else
                TimestamperPolarity_DatReg(TimestamperPolarity_PolarityBit_Con) <= '0';
            end if;
            Axi_Init_Proc(TimestamperCableDelay_Reg_Con, TimestamperCableDelay_DatReg);
            Axi_Init_Proc(TimestamperVersion_Reg_Con, TimestamperVersion_DatReg);
            Axi_Init_Proc(TimestamperIrq_Reg_Con, TimestamperIrq_DatReg);
            Axi_Init_Proc(TimestamperIrqMask_Reg_Con, TimestamperIrqMask_DatReg);
            Axi_Init_Proc(TimestamperEvtCount_Reg_Con, TimestamperEvtCount_DatReg);
            Axi_Init_Proc(TimestamperCount_Reg_Con, TimestamperCount_DatReg);
            Axi_Init_Proc(TimestamperTimeValueL_Reg_Con, TimestamperTimeValueL_DatReg);
            Axi_Init_Proc(TimestamperTimeValueH_Reg_Con, TimestamperTimeValueH_DatReg);

            Axi_Init_Proc(TimestamperData_Reg_Con, TimestamperData_DatReg); -- unused
            Axi_Init_Proc(TimestamperDataWidth_Reg_Con, TimestamperDataWidth_DatReg); -- unused
            
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
                        TempAddress := std_logic_vector(resize(unsigned(AxiReadAddrAddress_AdrIn),32));
                        AxiReadDataValid_ValReg <= '1';
                        AxiReadDataResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Read_Proc(TimestamperControl_Reg_Con, TimestamperControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperStatus_Reg_Con, TimestamperStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg); -- unused
                        Axi_Read_Proc(TimestamperPolarity_Reg_Con, TimestamperPolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        if (CableDelay_Gen = true) then
                            Axi_Read_Proc(TimestamperCableDelay_Reg_Con, TimestamperCableDelay_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        end if;
                        Axi_Read_Proc(TimestamperVersion_Reg_Con, TimestamperVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperIrq_Reg_Con, TimestamperIrq_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperIrqMask_Reg_Con, TimestamperIrqMask_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperEvtCount_Reg_Con, TimestamperEvtCount_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperCount_Reg_Con, TimestamperCount_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperTimeValueL_Reg_Con, TimestamperTimeValueL_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperTimeValueH_Reg_Con, TimestamperTimeValueH_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TimestamperDataWidth_Reg_Con, TimestamperDataWidth_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg); -- unused
                        Axi_Read_Proc(TimestamperData_Reg_Con, TimestamperData_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg); -- unused
                        Axi_AccessState_StaReg <= Resp_St;    
                    end if;
                    
                when Write_St => 
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(TimestamperControl_Reg_Con, TimestamperControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperStatus_Reg_Con, TimestamperStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg); -- unused
                        Axi_Write_Proc(TimestamperPolarity_Reg_Con, TimestamperPolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        if (CableDelay_Gen = true) then
                            Axi_Write_Proc(TimestamperCableDelay_Reg_Con, TimestamperCableDelay_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        end if;
                        Axi_Write_Proc(TimestamperVersion_Reg_Con, TimestamperVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperIrq_Reg_Con, TimestamperIrq_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperIrqMask_Reg_Con, TimestamperIrqMask_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperEvtCount_Reg_Con, TimestamperEvtCount_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperCount_Reg_Con, TimestamperCount_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperTimeValueL_Reg_Con, TimestamperTimeValueL_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperTimeValueH_Reg_Con, TimestamperTimeValueH_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TimestamperDataWidth_Reg_Con, TimestamperDataWidth_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg); -- unused
                        Axi_Write_Proc(TimestamperData_Reg_Con, TimestamperData_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg); -- unused
                        Axi_AccessState_StaReg <= Resp_St;                                                                                
                    end if;      
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or 
                       ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;    
                    end if;      
                    
                when others =>
                
            end case;       
            
            if (CableDelay_Gen = false) then
                Axi_Init_Proc(TimestamperCableDelay_Reg_Con, TimestamperCableDelay_DatReg);
            end if;
            
            if (Enable_Ena = '1') then
                -- counter counts all events 
                if (Timestamp_ValReg = '1') then
                    TimestamperEvtCount_DatReg <= std_logic_vector(unsigned(TimestamperEvtCount_DatReg) + 1);
                end if;
            else
                TimestamperIrq_DatReg(TimestamperIrq_TimestampBit_Con) <= '0'; -- when disabled we also clear that we had one pending
                TimestamperEvtCount_DatReg <= (others => '0');
                TimestamperCount_DatReg <= (others => '0');
                TimestamperStatus_DatReg(TimestamperStatus_DropBit_Con) <= '0';
            end if;
            
            if ((Enable_Ena = '1') and (TimestamperIrq_DatReg(TimestamperIrq_TimestampBit_Con) = '0') and 
                (Timestamp_ValReg = '1')) then
                -- counter counts the handled events
                TimestamperIrq_DatReg(TimestamperIrq_TimestampBit_Con) <= '1';
                TimestamperCount_DatReg <= std_logic_vector(Count_CntReg);
                TimestamperTimeValueL_DatReg <= Timestamp_Nanosecond_DatReg;
                TimestamperTimeValueH_DatReg <= Timestamp_Second_DatReg;
            elsif ((Enable_Ena = '1') and (TimestamperIrq_DatReg(TimestamperIrq_TimestampBit_Con) = '1') and 
                   (Timestamp_ValReg = '1')) then -- we still have a timestamp which was not handled, so we will drop that
                TimestamperStatus_DatReg(TimestamperStatus_DropBit_Con) <= '1'; -- make an event drop sticky
            end if;
            TimestamperDataWidth_DatReg <= std_logic_vector(to_unsigned(0, 32)); -- unused
            TimestamperData_DatReg <= std_logic_vector(to_unsigned(0, 32)); -- unused
            
        end if;
    end process Axi_Prc;
    
end architecture SignalTimestamper_Arch;

