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

library std;
use std.textio.all;

--*****************************************************************************************
-- Specific Libraries
--*****************************************************************************************
library TimecardLib;
use TimecardLib.Timecard_Package.all;

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The DummyAxiSlave reads and writes an instantiated RAM                                -- 
--                                                                                       --
-------------------------------------------------------------------------------------------
entity DummyAxiSlave is
    generic (
        ClockPeriod_Gen                             :       natural := 20; -- System clock 50MHz. Clock period in nanoseconds
        RamAddrWidth_Gen                            :       natural range 1 to 16 := 10
    );                  
    port (                  
        -- System                   
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
                    
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
end entity DummyAxiSlave;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture DummyAxiSlave_Arch of DummyAxiSlave is

    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Configuration Definitions
    --*************************************************************************************
    
    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type RamState_Type                              is (RamWait_St, RamDone_St);                 
    
    subtype RamEntry_Type                           is std_logic_vector(31 downto 0);
    type Ram_Type                                   is array(natural range <>) of std_logic_vector(31 downto 0);
                    
    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************
    
    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    
    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    -- Memory read signals 
    signal RamState_StaReg                          : RamState_Type;
    signal RamAddress_AdrReg                        : std_logic_vector((RamAddrWidth_Gen-1) downto 0);
    signal RamRead_DatReg                           : std_logic_vector(31 downto 0) := (others => '0');
    signal RamWrite_DatReg                          : std_logic_vector(31 downto 0) := (others => '0');
    signal RamWriteEn_EnaReg                        : std_logic;
    -- memory instantiation
    signal Memory_Ram                               : Ram_Type(((2**RamAddrWidth_Gen)-1) downto 0) := (others => (others => '0'));
    -- enforce usage of a Ramblock
    attribute ram_style                             : string;
    attribute ram_style of Memory_Ram               : signal is "block";
    
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
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    -- Axi process for reading and writing the register addresses
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
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
                       
            RamAddress_AdrReg <= (others => '0');
            RamState_StaReg <= RamWait_St;
            
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
                    RamState_StaReg <= RamWait_St;
                    
                when Read_St =>
                    if ((AxiReadAddrValid_ValIn = '1') and (AxiReadAddrReady_RdyReg = '1')) then
                        RamAddress_AdrReg <= "00" & AxiReadAddrAddress_AdrIn((RamAddrWidth_Gen-1) downto 2); -- 32-bit data per address 
                        RamState_StaReg <= RamWait_St;
                    else            
                        case RamState_StaReg is
                            when RamWait_St => 
                                RamState_StaReg <= RamDone_St;
                            when RamDone_St =>
                                AxiReadDataValid_ValReg <= '1';
                                AxiReadDataResponse_DatReg <= Axi_RespOk_Con;
                                AxiReadDataData_DatReg <= RamRead_DatReg;
                                Axi_AccessState_StaReg <= Resp_St;
                                RamState_StaReg <= RamWait_St;
                        end case;
                    end if;
                    
                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        RamAddress_AdrReg <= "00" & AxiWriteAddrAddress_AdrIn((RamAddrWidth_Gen-1) downto 2); -- 32-bit data per address
                        RamWrite_DatReg <= AxiWriteDataData_DatIn;
                        RamWriteEn_EnaReg <= '1'; 
                        RamState_StaReg <= RamWait_St;
                    else
                        case RamState_StaReg is
                            when RamWait_St => 
                                RamWriteEn_EnaReg <= '0';
                                RamState_StaReg <= RamDone_St;
                            when RamDone_St =>
                                AxiWriteRespValid_ValReg <= '1';
                                AxiWriteRespResponse_DatReg <= Axi_RespOk_Con;
                                Axi_AccessState_StaReg <= Resp_St;
                                RamState_StaReg <= RamWait_St;
                        end case;
                    end if;
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                       ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;

                when others =>
                    
            end case;
        end if;
    end process Axi_Prc;

    -- Separate the process to infer a block ram implementation of the memory
    Ram_Prc : process(SysClk_ClkIn) is
    begin
        if ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            if (RamWriteEn_EnaReg = '1') then
                Memory_Ram(to_integer(unsigned(RamAddress_AdrReg))) <= RamWrite_DatReg;
            end if;
            RamRead_DatReg <= Memory_Ram(to_integer(unsigned(RamAddress_AdrReg)));
        end if;    
    end process Ram_Prc;

    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
    

end architecture DummyAxiSlave_Arch;
