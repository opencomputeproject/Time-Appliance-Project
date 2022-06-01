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

--*****************************************************************************************
-- Specific Libraries
--*****************************************************************************************
library TimecardLib;
use TimecardLib.Timecard_Package.all;

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The Frequency Counter measures the frequency of an input signal over a period of time.--
-- The counter calculates non-fractional frequencies of range [0 Hz - 10'000'000 Hz]     --
-- and it is aligned to the local clock's new second.                                    --
-- The core can be configured by an AXI4Light-Slave Register interface.                  --
-------------------------------------------------------------------------------------------
entity FrequencyCounter is
    generic (
        Sim_Gen                                     :       boolean := false;
        OutputPolarity_Gen                          :       boolean := true

    );          
    port (          
        -- System           
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
            
        -- Time Input           
        ClockTime_Second_DatIn                      : in    std_logic_vector((SecondWidth_Con-1) downto 0);
        ClockTime_Nanosecond_DatIn                  : in    std_logic_vector((NanosecondWidth_Con-1) downto 0);
        ClockTime_TimeJump_DatIn                    : in    std_logic;
        ClockTime_ValIn                             : in    std_logic;
            
        -- Frequency input          
        Frequency_EvtIn                             : in    std_logic;
            
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
end entity FrequencyCounter;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture FrequencyCounter_Arch of FrequencyCounter is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    -- Frequency Counter Version
    constant FreqCntMajorVersion_Con                : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant FreqCntMinorVersion_Con                : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant FreqCntBuildVersion_Con                : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant FreqCntVersion_Con                     : std_logic_vector(31 downto 0) := FreqCntMajorVersion_Con & FreqCntMinorVersion_Con & FreqCntBuildVersion_Con;
        
    -- AXI registers    
    constant FreqCntControl_Reg_Con                 : Axi_Reg_Type:= (x"00000000", x"0000FF01", Rw_E, x"00000100");
    constant FreqCntFrequency_Reg_Con               : Axi_Reg_Type:= (x"00000004", x"E0FFFFFF", Ro_E, x"00000000");
    constant FreqCntPolarity_Reg_Con                : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant FreqCntVersion_Reg_Con                 : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, FreqCntVersion_Con);

    constant FreqCntControl_EnableBit_Con           : natural := 0; 
    constant FreqCntPolarity_PolarityBit_Con        : natural := 0;
    
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Enable_Ena                               : std_logic;
    signal MeasurePeriod_Dat                        : std_logic_vector(7 downto 0);
    signal FrequencyCounter_CntReg                  : unsigned(63 downto 0);
    signal FrequencyPeriodCounter_CntReg            : unsigned(7 downto 0);
    signal FrequencyTempPeriod_DatReg               : std_logic_vector(7 downto 0);
            
    signal FrequencyCount_DatReg                    : std_logic_vector(63 downto 0);
    signal FrequencyPeriod_DatReg                   : std_logic_vector(7 downto 0);
    signal Frequency_ValReg                         : std_logic;
    signal Frequency_ValOldReg                      : std_logic;
                
    signal FrequencyExtend_DatReg                   : std_logic_vector(63 downto 0);
    signal FrequencyCountExtend_DatReg              : std_logic_vector(127 downto 0);
    signal FrequencyPeriodExtend_DatReg             : std_logic_vector(127 downto 0);
    signal CalcFrequency_ValReg                     : std_logic;
    signal CalcStep_CntReg                          : natural range 0 to 63;
    signal CalcFrequencyDone_ValReg                 : std_logic;
    
    signal Polarity_Dat                             : std_logic;

    -- Time Input           
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_TimeJump_DatReg                : std_logic;
    signal ClockTime_ValReg                         : std_logic;    
                
    signal FrequencySysClk1_EvtReg                  : std_logic := '0';
    signal FrequencySysClk2_EvtReg                  : std_logic := '0';
    signal FrequencySysClk3_EvtReg                  : std_logic := '0';
                
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
            
    signal FreqCntControl_DatReg                    : std_logic_vector(31 downto 0);
    signal FreqCntFrequency_DatReg                  : std_logic_vector(31 downto 0);
    signal FreqCntPolarity_DatReg                   : std_logic_vector(31 downto 0);
    signal FreqCntVersion_DatReg                    : std_logic_vector(31 downto 0);
    
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    Enable_Ena                                      <= FreqCntControl_DatReg(FreqCntControl_EnableBit_Con);
    MeasurePeriod_Dat                               <= FreqCntControl_DatReg(15 downto 8);
    Polarity_Dat                                    <= FreqCntPolarity_DatReg(FreqCntPolarity_PolarityBit_Con);

    
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
    
    -- Metastability registers of the input signal
    Syc_Prc : process(SysClk_ClkIn) is
    begin
        if ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            FrequencySysClk1_EvtReg <= Frequency_EvtIn;
            FrequencySysClk2_EvtReg <= FrequencySysClk1_EvtReg;
            FrequencySysClk3_EvtReg <= FrequencySysClk2_EvtReg;
        end if;
    end process Syc_Prc;
    
    -- When the core is enabled by configuration and the input time is valid
    -- the measurement of the frequency will start at the beginning of the next second.
    -- At the rising edge of the input signal, the frequency counter increases 
    -- At the beginning of a new second the period counter (i.e. the number of seconds over which the frequency is measured) is updated.
    -- When a measurement period has completed, a flag is raised and the measurement starts over.
    Count_Prc :  process(SysClk_ClkIn, SysRstN_RstIn) is
    variable FrequencyCounter_CntVar    : unsigned(63 downto 0);
    begin
        if (SysRstN_RstIn = '0') then
            ClockTime_Second_DatReg <= (others => '0');   
            ClockTime_Nanosecond_DatReg <= (others => '0');   
            ClockTime_TimeJump_DatReg <= '0';
            ClockTime_ValReg <= '0'; 
            
            FrequencyCounter_CntReg <= to_unsigned(0, 64);
            FrequencyPeriodCounter_CntReg <= to_unsigned(0, 8);
            FrequencyTempPeriod_DatReg <= (others => '0');
 
            FrequencyCount_DatReg <= (others => '0');
            FrequencyPeriod_DatReg <= (others => '0');
            Frequency_ValReg <= '0';

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClockTime_Second_DatReg <= ClockTime_Second_DatIn;
            ClockTime_Nanosecond_DatReg <= ClockTime_Nanosecond_DatIn;
            ClockTime_TimeJump_DatReg <= ClockTime_TimeJump_DatIn;
            ClockTime_ValReg <= ClockTime_ValIn;
            
            Frequency_ValReg <= '0';
            
            if (((FrequencySysClk3_EvtReg = '0') and (FrequencySysClk2_EvtReg = '1') and (Polarity_Dat = '1')) or 
                ((FrequencySysClk3_EvtReg = '1') and (FrequencySysClk2_EvtReg = '0') and (Polarity_Dat = '0')))then
                FrequencyCounter_CntVar := FrequencyCounter_CntReg + 1;
            else
                FrequencyCounter_CntVar := FrequencyCounter_CntReg;
            end if;
            
            if ((ClockTime_ValIn = '0') or (ClockTime_TimeJump_DatIn = '1')) then
                FrequencyCounter_CntVar := to_unsigned(0, 64);
                FrequencyPeriodCounter_CntReg <= to_unsigned(0, 8);
            else
                -- at the beginning of a new second, assign the configuration, update the period counter 
                if (((Sim_Gen = true) and 
                     ((to_integer(unsigned(ClockTime_Nanosecond_DatReg)) mod 1000000) > (to_integer(unsigned(ClockTime_Nanosecond_DatIn)) mod 1000000))) or 
                    ((Sim_Gen = false) and 
                     (ClockTime_Second_DatReg /= ClockTime_Second_DatIn))) then
                    if ((FrequencyPeriodCounter_CntReg = 0) or (Enable_Ena = '0')) then -- not valid
                        FrequencyCounter_CntVar := to_unsigned(0, 64);
                        FrequencyPeriodCounter_CntReg <= unsigned(MeasurePeriod_Dat);
                        FrequencyTempPeriod_DatReg <= MeasurePeriod_Dat; -- this is needed for the division
                    elsif (FrequencyPeriodCounter_CntReg = 1) then
                        if (Enable_Ena = '1') then
                            if (Sim_Gen = true) then
                                FrequencyCounter_CntVar := resize((FrequencyCounter_CntVar * 1000), 64);
                            end if;
                            FrequencyCount_DatReg <= std_logic_vector(FrequencyCounter_CntVar);
                            FrequencyPeriod_DatReg <= FrequencyTempPeriod_DatReg; -- this is the one that we had for the current measurement
                            Frequency_ValReg <= '1';
                        end if;
                        FrequencyCounter_CntVar := to_unsigned(0, 64);
                        FrequencyPeriodCounter_CntReg <= unsigned(MeasurePeriod_Dat);
                        FrequencyTempPeriod_DatReg <= MeasurePeriod_Dat; -- this is needed for the division
                    else
                        FrequencyPeriodCounter_CntReg <= FrequencyPeriodCounter_CntReg - 1;
                    end if;
                end if;
            end if;
            
            FrequencyCounter_CntReg <= FrequencyCounter_CntVar;
        end if;
    end process Count_Prc;
    
    -- AXI slave for configuring and supervising the core
    -- Enable the core and provide the measurement period
    -- Divide the frequency counter by the measurement period and store the result to the register
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

            Axi_Init_Proc(FreqCntControl_Reg_Con, FreqCntControl_DatReg);
            Axi_Init_Proc(FreqCntFrequency_Reg_Con, FreqCntFrequency_DatReg);
            Axi_Init_Proc(FreqCntPolarity_Reg_Con, FreqCntPolarity_DatReg);
            Axi_Init_Proc(FreqCntVersion_Reg_Con, FreqCntVersion_DatReg);
            
            if (OutputPolarity_Gen = true) then
                FreqCntPolarity_DatReg(FreqCntPolarity_PolarityBit_Con) <= '1';
            else
                FreqCntPolarity_DatReg(FreqCntPolarity_PolarityBit_Con) <= '0';
            end if;
            CalcFrequency_ValReg <= '0';
            CalcStep_CntReg <= 0;
            Frequency_ValOldReg <= '0';
            FrequencyCountExtend_DatReg <= (others => '0');
            FrequencyPeriodExtend_DatReg <= (others => '0');
            FrequencyExtend_DatReg <= (others => '0');
            CalcFrequencyDone_ValReg <= '0';

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            Frequency_ValOldReg <= Frequency_ValReg;
            CalcFrequencyDone_ValReg <= '0';
            
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
                        Axi_Read_Proc(FreqCntControl_Reg_Con, FreqCntControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(FreqCntFrequency_Reg_Con, FreqCntFrequency_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(FreqCntPolarity_Reg_Con, FreqCntPolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(FreqCntVersion_Reg_Con, FreqCntVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(FreqCntControl_Reg_Con, FreqCntControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(FreqCntFrequency_Reg_Con, FreqCntFrequency_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(FreqCntPolarity_Reg_Con, FreqCntPolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(FreqCntVersion_Reg_Con, FreqCntVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                       ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;
                when others =>
            end case;

            if ((Frequency_ValReg = '1') and (Frequency_ValOldReg = '0')) then 
                FrequencyCountExtend_DatReg(127 downto 64) <= (others => '0');
                FrequencyCountExtend_DatReg(63 downto 0) <= FrequencyCount_DatReg;
                FrequencyPeriodExtend_DatReg(127 downto 64) <= x"00000000000000" & FrequencyPeriod_DatReg;
                FrequencyPeriodExtend_DatReg(63 downto 0) <= (others => '0');
                FrequencyExtend_DatReg <= (others => '0');
                CalcStep_CntReg <= 63;
                if (unsigned(FrequencyPeriod_DatReg) = 0) then -- no division by 0
                    FreqCntFrequency_DatReg(31) <= '0';
                    FreqCntFrequency_DatReg(30) <= '1';
                    FreqCntFrequency_DatReg(29) <= '0';
                    FreqCntFrequency_DatReg(23 downto 0) <= (others => '0');
                else
                    -- only calculate if ok so we know that in case of an error we cleared the reg
                    CalcFrequency_ValReg <= '1';
                end if;    
            elsif (CalcFrequency_ValReg = '1') then 
                if (unsigned(FrequencyCountExtend_DatReg(126 downto 0) & '0') >= unsigned(FrequencyPeriodExtend_DatReg)) then
                    FrequencyCountExtend_DatReg <= std_logic_vector(resize(unsigned(FrequencyCountExtend_DatReg(126 downto 0) & '0') - unsigned(FrequencyPeriodExtend_DatReg), 128));
                    FrequencyExtend_DatReg(CalcStep_CntReg) <= '1';
                else
                    FrequencyCountExtend_DatReg <= (FrequencyCountExtend_DatReg(126 downto 0) & '0');
                    FrequencyExtend_DatReg(CalcStep_CntReg) <= '0';
                end if;
                if (CalcStep_CntReg > 0) then
                    CalcStep_CntReg <= CalcStep_CntReg - 1;
                else
                    CalcFrequency_ValReg <= '0';
                    CalcFrequencyDone_ValReg <= '1';
                end if;
            end if;
            
            if (CalcFrequencyDone_ValReg = '1') then 
                if (unsigned(FrequencyExtend_DatReg) > 10000000) then
                    FreqCntFrequency_DatReg(31) <= '0';
                    FreqCntFrequency_DatReg(30) <= '1'; -- this is also an error
                    FreqCntFrequency_DatReg(29) <= '1';
                    FreqCntFrequency_DatReg(23 downto 0) <= (others => '0');
                else
                    FreqCntFrequency_DatReg(31) <= '1';
                    FreqCntFrequency_DatReg(30) <= '0';
                    FreqCntFrequency_DatReg(29) <= '0';
                    FreqCntFrequency_DatReg(23 downto 0) <= FrequencyExtend_DatReg(23 downto 0);
                end if;
            end if;

            if (Enable_Ena = '0') then
                Axi_Init_Proc(FreqCntFrequency_Reg_Con, FreqCntFrequency_DatReg);
            end if;
            
        end if;
    end process Axi_Prc;
    
end architecture FrequencyCounter_Arch;