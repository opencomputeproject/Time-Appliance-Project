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
-- The PPS Generator generates a Pulse Per Second (PPS) aligned to the local clock's     --
-- new second. The local clock is provided as input. The core can be configured by an    --
-- AXI4Light-Slave Register interface. A high resolution clock is used for the pulse     --
-- generation to reduce the jitter.                                                      --
-------------------------------------------------------------------------------------------
entity PpsGenerator is
    generic (
        ClockPeriod_Gen                             :       natural := 20;  
        CableDelay_Gen                              :       boolean := false;
        OutputDelay_Gen                             :       natural := 0;       
        OutputPolarity_Gen                          :       boolean := true;        
        HighResFreqMultiply_Gen                     :       natural range 4 to 10 := 5;
                                
        Sim_Gen                                     :       boolean := false
    );          
    port (          
        -- System           
        SysClk_ClkIn                                : in    std_logic;
        SysClkNx_ClkIn                              : in    std_logic := '0';
        SysRstN_RstIn                               : in    std_logic;
                
        -- Time Input                       
        ClockTime_Second_DatIn                      : in   std_logic_vector((SecondWidth_Con-1) downto 0);
        ClockTime_Nanosecond_DatIn                  : in   std_logic_vector((NanosecondWidth_Con-1) downto 0);
        ClockTime_TimeJump_DatIn                    : in   std_logic;
        ClockTime_ValIn                             : in   std_logic;
                
        -- Pps Output                       
        Pps_EvtOut                                  : out   std_logic;
                
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
end entity PpsGenerator;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture PpsGenerator_Arch of PpsGenerator is
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

    constant OutputPulseWidthMillsecond_Con         : natural := 500;
    
    -- PPS Generator version
    constant PpsGenMajorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant PpsGenMinorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant PpsGenBuildVersion_Con                 : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant PpsGenVersion_Con                      : std_logic_vector(31 downto 0) := PpsGenMajorVersion_Con & PpsGenMinorVersion_Con & PpsGenBuildVersion_Con;
    
    -- AXI regs                                                       Addr       , Mask       , RW  , Reset
    constant PpsGenControl_Reg_Con                  : Axi_Reg_Type:= (x"00000000", x"00000001", Rw_E, x"00000000");
    constant PpsGenStatus_Reg_Con                   : Axi_Reg_Type:= (x"00000004", x"00000001", Wc_E, x"00000000");
    constant PpsGenPolarity_Reg_Con                 : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant PpsGenVersion_Reg_Con                  : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, PpsGenVersion_Con);
    constant PpsGenPulseWidth_Reg_Con               : Axi_Reg_Type:= (x"00000010", x"000003FF", Rw_E, x"00000000"); -- unused
    constant PpsGenCableDelay_Reg_Con               : Axi_Reg_Type:= (x"00000020", x"0000FFFF", Rw_E, x"00000000");

    constant PpsGenControl_EnableBit_Con            : natural := 0;
    constant PpsGenStatus_ErrorBit_Con              : natural := 0;    
    constant PpsGenPolarity_PolarityBit_Con         : natural := 0;

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Enable_Ena                               : std_logic;
    signal Polarity_Dat                             : std_logic;
    signal PulseWidth_Dat                           : std_logic_vector(9 downto 0);
    signal CableDelay_Dat                           : std_logic_vector(15 downto 0);
    
    -- Time Input           
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_TimeJump_DatReg                : std_logic;
    signal ClockTime_ValReg                         : std_logic;

    -- count the high resolution ticks
    signal PpsShiftSysClk_DatReg                    : std_logic_vector((HighResFreqMultiply_Gen-1) downto 0);
    signal PpsShiftSysClk1_DatReg                   : std_logic_vector((HighResFreqMultiply_Gen-1) downto 0) := (others => '0');
    signal PpsShiftSysClkNx_DatReg                  : std_logic_vector(((HighResFreqMultiply_Gen*2)-1) downto 0) := (others => '0');
    
    signal PpsError_Reg                             : std_logic;
    signal Pps_Reg                                  : std_logic;
    signal PulseWidthCounter_CntReg                 : integer;
    
    -- AXI signals and regs
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;
    signal AxiWriteAddrReady_RdyReg                 : std_logic;       
    signal AxiWriteDataReady_RdyReg                 : std_logic;   
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);   
    signal AxiReadAddrReady_RdyReg                  : std_logic;          
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);
                
    signal PpsGenControl_DatReg                     : std_logic_vector(31 downto 0);
    signal PpsGenPolarity_DatReg                    : std_logic_vector(31 downto 0);
    signal PpsGenStatus_DatReg                      : std_logic_vector(31 downto 0);
    signal PpsGenVersion_DatReg                     : std_logic_vector(31 downto 0);
    signal PpsGenPulseWidth_DatReg                  : std_logic_vector(31 downto 0); -- unused
    signal PpsGenCableDelay_DatReg                  : std_logic_vector(31 downto 0);
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    AxiWriteAddrReady_RdyOut                        <= AxiWriteAddrReady_RdyReg;        
    AxiWriteDataReady_RdyOut                        <= AxiWriteDataReady_RdyReg;   
    AxiWriteRespValid_ValOut                        <= AxiWriteRespValid_ValReg;
    AxiWriteRespResponse_DatOut                     <= AxiWriteRespResponse_DatReg;   
    AxiReadAddrReady_RdyOut                         <= AxiReadAddrReady_RdyReg;          
    AxiReadDataValid_ValOut                         <= AxiReadDataValid_ValReg;
    AxiReadDataResponse_DatOut                      <= AxiReadDataResponse_DatReg;
    AxiReadDataData_DatOut                          <= AxiReadDataData_DatReg;

    Polarity_Dat                                    <= PpsGenPolarity_DatReg(PpsGenPolarity_PolarityBit_Con);
    Enable_Ena                                      <= PpsGenControl_DatReg(PpsGenControl_EnableBit_Con);
    PulseWidth_Dat                                  <= PpsGenPulseWidth_DatReg(9 downto 0);
    CableDelay_Dat                                  <= PpsGenCableDelay_DatReg(15 downto 0);

    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    
   -- Pulse generation on the clock domain of the high frequency clock
    SysClkNxReg_Prc : process(SysClkNx_ClkIn) is
    begin
        if ((SysClkNx_ClkIn'event) and (SysClkNx_ClkIn = '1')) then
            PpsShiftSysClk1_DatReg <= PpsShiftSysClk_DatReg;
            if (PpsShiftSysClk_DatReg /= PpsShiftSysClk1_DatReg) then
                PpsShiftSysClkNx_DatReg <= PpsShiftSysClkNx_DatReg(((HighResFreqMultiply_Gen*2)-2) downto (HighResFreqMultiply_Gen-1)) & PpsShiftSysClk_DatReg; -- copy the high resolution clock periods
            else
                PpsShiftSysClkNx_DatReg <= PpsShiftSysClkNx_DatReg(((HighResFreqMultiply_Gen*2)-2) downto 0) & PpsShiftSysClkNx_DatReg(0); -- retain the last value
            end if;
            if (Polarity_Dat = '1') then
                Pps_EvtOut <= PpsShiftSysClkNx_DatReg((HighResFreqMultiply_Gen*2)-1);
            else
                Pps_EvtOut <= not PpsShiftSysClkNx_DatReg((HighResFreqMultiply_Gen*2)-1);         
            end if;
        end if;
    end process SysClkNxReg_Prc;

    -- The process sets the activation and deactivation of the PPS, based on the system
    -- clock. It also marks the pulse-activation in a shift register, which will
    -- be later used by the high-resolution clock to set a motre accurate activation time.
    -- The deactivation of the pulse is calculated by a free-running counter (i.e. not aligned 
    -- to the local time).
    Pps_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            PpsError_Reg <= '0';
            Pps_Reg <= '0';
            PpsShiftSysClk_DatReg <= (others => '0');
            
            ClockTime_Second_DatReg <= (others => '0');    
            ClockTime_Nanosecond_DatReg <= (others => '0');
            ClockTime_TimeJump_DatReg <= '0';  
            ClockTime_ValReg <= '0';           
            
            PulseWidthCounter_CntReg <= 0;
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
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
                if ((ClockTime_ValReg = '0') or (ClockTime_TimeJump_DatReg = '1')) then
                    -- do nothing, this may cause a loss of a PPS. If overflow happens, better than a wrong PPS          
                    PpsError_Reg <= '1';
                else
                    -- the pulse activation time is when a new second starts minus the total output delay
                    if ((Pps_Reg = '0') and
                        (((Sim_Gen = true) and ((to_integer(unsigned(ClockTime_Nanosecond_DatReg)) mod (SecondNanoseconds_Con/(1000*10))) >= ((SecondNanoseconds_Con/(1000*10)) - OutputDelaySum_Con))) or
                         ((Sim_Gen = false) and ((to_integer(unsigned(ClockTime_Nanosecond_DatReg))) >= (SecondNanoseconds_Con - OutputDelaySum_Con))))) then -- overflow in first half
                        PpsError_Reg <= '0'; -- clear the error on the next PPS
                        Pps_Reg <= '1'; -- this we need to do the edge detection
                        PpsShiftSysClk_DatReg <= (others => '0');
                        -- Mark in a shift-register how many high-resolution clock periods 'fit' between the current time and the compensated pulse-activation time.                         
                        for i in 0 to (HighResFreqMultiply_Gen-1) loop
                            if (Sim_Gen = true) then
                                if ((to_integer(unsigned(ClockTime_Nanosecond_DatReg)) mod (SecondNanoseconds_Con/(1000*10))) >= ((SecondNanoseconds_Con/(1000*10)) - OutputDelaySum_Con + (i*HighResClockPeriod_Con))) then
                                    PpsShiftSysClk_DatReg(i) <= '1';
                                end if;                         
                            else
                                if ((to_integer(unsigned(ClockTime_Nanosecond_DatReg))) >= (SecondNanoseconds_Con - OutputDelaySum_Con + (i*HighResClockPeriod_Con))) then
                                    PpsShiftSysClk_DatReg(i) <= '1';
                                end if;                         
                            end if;                         
                        end loop;
                        if (Sim_Gen = true) then
                            PulseWidthCounter_CntReg <= to_integer(unsigned(PulseWidth_Dat))*(SecondNanoseconds_Con/1000)/(1000*10);
                        else
                            PulseWidthCounter_CntReg <= to_integer(unsigned(PulseWidth_Dat))*(SecondNanoseconds_Con/1000);
                        end if;
                    -- the pulse deactivation time is when a free-running counter counts down to 0
                    else
                        if (Pps_Reg = '1') then
                            PpsShiftSysClk_DatReg <= (others => '1'); -- now set the level
                        end if;
                        if (PulseWidthCounter_CntReg > ClockPeriod_Gen) then -- pulse done (not aligned with the input clock)
                            PulseWidthCounter_CntReg <= PulseWidthCounter_CntReg - ClockPeriod_Gen;               
                        else
                            Pps_Reg <= '0';
                            PpsShiftSysClk_DatReg <= (others => '0');
                        end if;
                    end if;
                end if;
            else
                Pps_Reg <= '0';
                PpsShiftSysClk_DatReg <= (others => '0');
                if (Sim_Gen = true) then
                    PulseWidthCounter_CntReg <= to_integer(unsigned(PulseWidth_Dat))/(1000*10);
                else
                    PulseWidthCounter_CntReg <= to_integer(unsigned(PulseWidth_Dat));
                end if;
            end if;
        end if;
    end process Pps_Prc;
    
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
            
            Axi_Init_Proc(PpsGenControl_Reg_Con, PpsGenControl_DatReg);
            Axi_Init_Proc(PpsGenStatus_Reg_Con, PpsGenStatus_DatReg);
            Axi_Init_Proc(PpsGenPolarity_Reg_Con, PpsGenPolarity_DatReg);
            Axi_Init_Proc(PpsGenVersion_Reg_Con, PpsGenVersion_DatReg);
            Axi_Init_Proc(PpsGenPulseWidth_Reg_Con, PpsGenPulseWidth_DatReg); -- unused
            Axi_Init_Proc(PpsGenCableDelay_Reg_Con, PpsGenCableDelay_DatReg);
            
            if (OutputPolarity_Gen = true) then
                PpsGenPolarity_DatReg(PpsGenPolarity_PolarityBit_Con) <= '1';
            else
                PpsGenPolarity_DatReg(PpsGenPolarity_PolarityBit_Con) <= '0';
            end if;
            
            PpsGenPulseWidth_DatReg(9 downto 0) <= std_logic_vector(to_unsigned(OutputPulseWidthMillsecond_Con, 10)); -- overwrite with constant
           
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
                        Axi_Read_Proc(PpsGenControl_Reg_Con, PpsGenControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsGenStatus_Reg_Con, PpsGenStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsGenPolarity_Reg_Con, PpsGenPolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsGenVersion_Reg_Con, PpsGenVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(PpsGenPulseWidth_Reg_Con, PpsGenPulseWidth_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg); -- unused
                        if (CableDelay_Gen = true) then
                            Axi_Read_Proc(PpsGenCableDelay_Reg_Con, PpsGenCableDelay_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        end if;
                        Axi_AccessState_StaReg <= Resp_St;    
                    end if;
                    
                when Write_St => 
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(PpsGenControl_Reg_Con, PpsGenControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsGenStatus_Reg_Con, PpsGenStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsGenPolarity_Reg_Con, PpsGenPolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsGenVersion_Reg_Con, PpsGenVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(PpsGenPulseWidth_Reg_Con, PpsGenPulseWidth_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg); -- unused
                        if (CableDelay_Gen = true) then
                            Axi_Write_Proc(PpsGenCableDelay_Reg_Con, PpsGenCableDelay_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        end if;
                        Axi_AccessState_StaReg <= Resp_St;                                                        
                    end if; 
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or 
                        ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;    
                    end if;               
                    
                when others =>
                
            end case;  
            
            if (PpsGenControl_DatReg(PpsGenControl_EnableBit_Con) = '1') then
                if (PpsError_Reg = '1') then -- make it sticky
                    PpsGenStatus_DatReg(PpsGenStatus_ErrorBit_Con) <= '1';
                end if;
            else
                PpsGenStatus_DatReg(PpsGenStatus_ErrorBit_Con) <= '0';
            end if;
                            
            PpsGenPulseWidth_DatReg(9 downto 0) <= std_logic_vector(to_unsigned(OutputPulseWidthMillsecond_Con, 10)); -- overwrite with generic
                            
            if (CableDelay_Gen = false) then
                Axi_Init_Proc(PpsGenCableDelay_Reg_Con, PpsGenCableDelay_DatReg);
            end if;
            
        end if;
    end process Axi_Prc;


end architecture PpsGenerator_Arch;