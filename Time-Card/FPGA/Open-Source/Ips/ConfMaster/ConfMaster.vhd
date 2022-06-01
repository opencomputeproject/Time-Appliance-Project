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
-- The ConfMaster initializes a ROM with the contents of file ConfigFile_Gen. The        -- 
-- contents are commands for accessing the AXI registers of the other cores.             --
-- A basic configuration of the design's cores can be applied.                           --
-- When all the commands have been processed, the ConfigDone_ValOut is activated.        --
-------------------------------------------------------------------------------------------
entity ConfMaster is
    generic (
        AxiTimeout_Gen                              :       natural := 0; -- 0 means no timeout (wait forever), else timeout in nanoseconds
        ConfigFile_Gen                              :       string := "NA";
        ClockPeriod_Gen                             :       natural := 20 -- System clock 50MHz. Clock period in nanoseconds
    );              
    port (              
        -- System               
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
                        
        -- Configuration Output             
        ConfigDone_ValOut                           : out   std_logic;
                
        -- Axi              
        AxiWriteAddrValid_ValOut                    : out   std_logic;
        AxiWriteAddrReady_RdyIn                     : in    std_logic;
        AxiWriteAddrAddress_AdrOut                  : out   std_logic_vector(31 downto 0);
        AxiWriteAddrProt_DatOut                     : out   std_logic_vector(2 downto 0);
                        
        AxiWriteDataValid_ValOut                    : out   std_logic;
        AxiWriteDataReady_RdyIn                     : in    std_logic;
        AxiWriteDataData_DatOut                     : out   std_logic_vector(31 downto 0);
        AxiWriteDataStrobe_DatOut                   : out   std_logic_vector(3 downto 0);
                        
        AxiWriteRespValid_ValIn                     : in    std_logic;
        AxiWriteRespReady_RdyOut                    : out   std_logic;
        AxiWriteRespResponse_DatIn                  : in    std_logic_vector(1 downto 0);
                
        AxiReadAddrValid_ValOut                     : out   std_logic;
        AxiReadAddrReady_RdyIn                      : in    std_logic;
        AxiReadAddrAddress_AdrOut                   : out   std_logic_vector(31 downto 0);
        AxiReadAddrProt_DatOut                      : out   std_logic_vector(2 downto 0);
                        
        AxiReadDataValid_ValIn                      : in    std_logic;
        AxiReadDataReady_RdyOut                     : out   std_logic;
        AxiReadDataResponse_DatIn                   : in    std_logic_vector(1 downto 0);
        AxiReadDataData_DatIn                       : in    std_logic_vector(31 downto 0)      
    );
end entity ConfMaster;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture ConfMaster_Arch of ConfMaster is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Configuration Definitions
    --*************************************************************************************
    constant MaxEntries_Con                         : natural := 2**12; -- change this if you need more than 2**12 entries in the list
    
    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    subtype Rom_Entry_Type                          is std_logic_vector(127 downto 0);
    type Rom_Type                                   is array(natural range <>) of Rom_Entry_Type;
                
    type ConfigList_Type is record           
        ConfigListData                              : Rom_Type((MaxEntries_Con-1) downto 0);
        ConfigListSize                              : natural range 0 to MaxEntries_Con;
    end record ConfigList_Type;                  
            
    type ConfigCommand_Type                         is (Unknown_E,
                                                        Skip_E,
                                                        Wait_E,
                                                        Read_E,
                                                        Write_E);
                
    type ConfigState_Type                           is (Idle_St,
                                                        WaitConfig_St,
                                                        FetchConfig_St,
                                                        Skip_St,
                                                        Wait_St,
                                                        StartReadWrite_St,
                                                        WaitToRead_St,
                                                        WaitToWrite_St,
                                                        End_St);
    
    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************
    -- Define an impure function for parsing the text file ConfigFile_Gen and assigning its contents to the MemoryConfigList
    -- Only the valid lines are assigned to the output, the empty/commented lines are skipped
    -- The expected ConfigFile_Gen format:
    -- - each line is a command (skip, wait, read, write)  
    -- - the format of a line in ascii characters is:
    --      - if the first ascii character is NUL/CR/LF/HT or comment "--", "//", or the line has less than 35 characters, then skip the line
    --      - 1 - 9: 8 hex digits (command type) followed by a space(' ') character. The command type enumeration is 1:Skip, 2:Wait, 3: Read, 4:Write
    --      - 10-18: 8 hex digits (base address) followed by a space(' ') character. Applicable for Read/Write commands. Otherwise, 0
    --      - 19-27: 8 hex digits (reg address) followed by a space(' ') character. Applicable for Read/Write commands. Otherwise, 0
    --      - 28-35: 8 hex digits (Data if Write or Time in ns if Wait).
    --      - >=36 : skip the additional characters of the line
    -- - if the line has invalid format, then the line is skipped
    impure function LoadConfigList_Func(ConfigFile : string) return ConfigList_Type is
        variable Rom                                : ConfigList_Type := ((others => (others => '0')),0);
        file InputFile                              : text;
        variable TempLine                           : line;
        variable TempChar                           : character;
        variable NotEol                             : boolean;
        variable TempLineString                     : string(1 to 35);
        variable TempLineLength                     : natural;
        variable TempLineIndex                      : natural;
        variable TempValue1                         : unsigned(31 downto 0);
        variable TempValue2                         : unsigned(31 downto 0);
        variable TempValue3                         : unsigned(31 downto 0);
        variable TempValue4                         : unsigned(31 downto 0);
        variable TempInteger                        : natural range 0 to 17;
        variable SkipLine                           : boolean;
    begin
        Rom := ((others => (others => '0')),0);
        file_open(InputFile, ConfigFile, READ_MODE);
        TempLineIndex := 0;
        while ((TempLineIndex < (MaxEntries_Con-1)) and -- ensure not out of bounds
               (not endfile(InputFile))) loop
            readline(InputFile,TempLine);
            TempLineLength := 1;
            SkipLine := false;
            TempValue1 := (others => '0');
            TempValue2 := (others => '0');
            TempValue3 := (others => '0');
            TempValue4 := (others => '0');
            for i in 1 to 35 loop
                read(TempLine, TempChar, NotEol);
                if (NotEol = true) then
                    TempLineString(i) := TempChar;
                    TempLineLength := i;
                else
                    TempLineString(i) := NUL;
                end if;
            end loop;    
            if ((TempLineLength < 35) or  -- at least 35 characters are expected per line
                (TempLineString(1) = ' ') or (TempLineString(1) = NUL) or (TempLineString(1) = CR) or (TempLineString(1) = LF) or (TempLineString(1) = HT) or -- the new line does not start with some special characters 
                (TempLineString(1 to 2)= "//" or TempLineString(1 to 2)= "--" )) then -- no comment lines
                SkipLine := true;
            else    
                report "======================================" severity note; 
                report "ConfigIndex: " & integer'image(TempLineIndex) severity note; 
                report "ConfigString: " & TempLineString(1 to TempLineLength) severity note; 
                report "StringLength: " & integer'image(TempLineLength) severity note;
                for i in 1 to 35 loop
                    case (TempLineString(i)) is
                        when '0' => TempInteger := 0;
                        when '1' => TempInteger := 1;
                        when '2' => TempInteger := 2;
                        when '3' => TempInteger := 3;
                        when '4' => TempInteger := 4;
                        when '5' => TempInteger := 5;
                        when '6' => TempInteger := 6;
                        when '7' => TempInteger := 7;
                        when '8' => TempInteger := 8;
                        when '9' => TempInteger := 9;
                        when 'A' => TempInteger := 10;
                        when 'B' => TempInteger := 11;
                        when 'C' => TempInteger := 12;
                        when 'D' => TempInteger := 13;
                        when 'E' => TempInteger := 14;
                        when 'F' => TempInteger := 15;
                        when 'a' => TempInteger := 10;
                        when 'b' => TempInteger := 11;
                        when 'c' => TempInteger := 12;
                        when 'd' => TempInteger := 13;
                        when 'e' => TempInteger := 14;
                        when 'f' => TempInteger := 15;
                        when ' ' => TempInteger := 16;
                        when others => TempInteger := 17;
                            report "character_to_integer: illegal state" 
                            severity failure; 
                    end case;
                    if (i <= 8) then 
                        if (TempInteger < 16) then -- only hex characters
                            TempValue1 := resize((TempValue1*16),32) + TempInteger;
                        else
                            SkipLine := true;    
                        end if;
                    elsif (i = 9) then  
                        if (TempInteger /= 16) then -- no space character
                            SkipLine := true;    
                        end if;
                    elsif (i <= 17) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue2 := resize((TempValue2*16),32) + TempInteger;
                        else
                            SkipLine := true;    
                        end if;
                    elsif (i = 18) then  
                        if (TempInteger /= 16) then -- no space character
                            SkipLine := true;    
                        end if;
                    elsif (i <= 26) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue3 := resize((TempValue3*16),32) + TempInteger;
                        else
                            SkipLine := true;    
                        end if;
                    elsif (i = 27) then  
                        if (TempInteger /= 16) then -- no space character
                            SkipLine := true;    
                        end if;
                    elsif (i <= 35) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue4 := resize((TempValue4*16),32) + TempInteger;
                        else
                            SkipLine := true;    
                        end if;
                    end if;    
                end loop;    
                if (SkipLine = false) then 
                    Rom.ConfigListData(TempLineIndex)(31 downto 0) := std_logic_vector(TempValue1);
                    Rom.ConfigListData(TempLineIndex)(63 downto 32) := std_logic_vector(TempValue2);
                    Rom.ConfigListData(TempLineIndex)(95 downto 64) := std_logic_vector(TempValue3);
                    Rom.ConfigListData(TempLineIndex)(127 downto 96) := std_logic_vector(TempValue4);
                    TempLineIndex := TempLineIndex + 1;
                    Rom.ConfigListSize := TempLineIndex;
                end if;    
            end if;
        end loop;
        file_close(InputFile);
        return Rom;
    end function LoadConfigList_Func;
    
    --*************************************************************************************
    -- Constant Definitions (only after file handling)
    --*************************************************************************************
    constant MemoryConfigList_Con                   : ConfigList_Type := LoadConfigList_Func(ConfigFile_Gen);
    constant RomAddrWidth_Con                       : natural := integer(ceil(log2(real((MemoryConfigList_Con.ConfigListSize)))));

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    -- ROM
    signal RomAddress_AdrReg                        : std_logic_vector((RomAddrWidth_Con-1) downto 0);
    signal RomRead_DatReg                           : std_logic_vector(127 downto 0) := (others => '0');
    signal RomData_Rom                              : Rom_Type(((2**RomAddrWidth_Con)-1) downto 0) := MemoryConfigList_Con.ConfigListData(((2**RomAddrWidth_Con)-1) downto 0);
    -- to enforce usage of a Ramblock
    attribute ram_style                             : string;
    attribute ram_style of RomData_Rom              : signal is "block";

    -- control ROM read
    signal ConfigIndex_CntReg                       : natural range 0 to MemoryConfigList_Con.ConfigListSize;
    signal ConfigState_StaReg                       : ConfigState_Type;
    signal ConfigCommand_DatReg                     : ConfigCommand_Type;
    signal ConfigBaseAddr_DatReg                    : std_logic_Vector(31 downto 0);
    signal ConfigRegAddr_DatReg                     : std_logic_Vector(31 downto 0);
    signal ConfigData_DatReg                        : std_logic_Vector(31 downto 0);
    
    -- Axi info
    signal AxiTimeout_CntReg                        : natural range 0 to AxiTimeout_Gen;
    signal AxiReadData_DatReg                       : std_logic_vector(31 downto 0); -- unused
    signal AxiResponse_DatReg                       : std_logic_vector(2 downto 0);  -- unused
    
    -- Axi
    signal AxiWriteAddrValid_ValReg                 : std_logic;
    signal AxiWriteAddrAddress_AdrReg               : std_logic_vector(31 downto 0);
    signal AxiWriteAddrProt_DatReg                  : std_logic_vector(2 downto 0);
            
    signal AxiWriteDataValid_ValReg                 : std_logic;
    signal AxiWriteDataData_DatReg                  : std_logic_vector(31 downto 0);
    signal AxiWriteDataStrobe_DatReg                : std_logic_vector(3 downto 0);
            
    signal AxiWriteRespReady_RdyReg                 : std_logic;
            
    signal AxiReadAddrValid_ValReg                  : std_logic;
    signal AxiReadAddrAddress_AdrReg                : std_logic_vector(31 downto 0);
    signal AxiReadAddrProt_DatReg                   : std_logic_vector(2 downto 0);
            
    signal AxiReadDataReady_RdyReg                  : std_logic;
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    AxiWriteAddrValid_ValOut <= AxiWriteAddrValid_ValReg;
    AxiWriteAddrAddress_AdrOut <= AxiWriteAddrAddress_AdrReg;
    AxiWriteAddrProt_DatOut <= AxiWriteAddrProt_DatReg;

    AxiWriteDataValid_ValOut <= AxiWriteDataValid_ValReg;
    AxiWriteDataData_DatOut <= AxiWriteDataData_DatReg;
    AxiWriteDataStrobe_DatOut <= AxiWriteDataStrobe_DatReg;

    AxiWriteRespReady_RdyOut <= AxiWriteRespReady_RdyReg;

    AxiReadAddrValid_ValOut <= AxiReadAddrValid_ValReg;
    AxiReadAddrAddress_AdrOut <= AxiReadAddrAddress_AdrReg;
    AxiReadAddrProt_DatOut <= AxiReadAddrProt_DatReg;

    AxiReadDataReady_RdyOut <= AxiReadDataReady_RdyReg;
    
    -- this is done this way to have no warning of truncation if the ConfigIndex reaches exactly 2**RomAddrWidth_Con
    -- e.g. we have 4 entries, the address is 0 to 3 then, but after all of them are handled ConfigIndex_CntReg will be 4
    --         since it counts the number entries handled as well as being the index before increment that and
    --         entry has been handled
    RomAddress_AdrReg <= std_logic_vector(to_unsigned(ConfigIndex_CntReg, 32)((RomAddrWidth_Con-1) downto 0));
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    
    -- Axi master for accessing the design's cores registers or waiting for time in ns
    -- The commands are stored in a Rom. Each address stores 128 bits of data:
    -- Bits  31 - 0 : Command type (0/1/>4 = Skip, 2 = Wait, 3 = Read, 4 = Write)
    -- Bits  63 - 32: Base Address (for read/write commands, else 0)
    -- Bits  95 - 64: Reg Address (for read/write commands, else 0)
    -- Bits 127 - 96: Data (for write command) or time (in ns, for wait command)
    Config_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            ConfigState_StaReg <= Idle_St;
            ConfigIndex_CntReg <= 0;
            ConfigCommand_DatReg <= Unknown_E;
            ConfigBaseAddr_DatReg <= (others => '0'); 
            ConfigRegAddr_DatReg <= (others => '0'); 
            ConfigData_DatReg <= (others => '0');    
            
            AxiReadData_DatReg <= (others => '0');
            AxiResponse_DatReg <= (others => '0');
            
            AxiWriteAddrValid_ValReg <= '0';
            AxiWriteAddrAddress_AdrReg <= (others => '0');
            AxiWriteAddrProt_DatReg <= (others => '0');
            
            AxiWriteDataValid_ValReg <= '0';
            AxiWriteDataData_DatReg <= (others => '0');
            AxiWriteDataStrobe_DatReg <= (others => '0');
            
            AxiWriteRespReady_RdyReg <= '0';
            
            AxiReadAddrValid_ValReg <= '0';
            AxiReadAddrAddress_AdrReg <= (others => '0');
            AxiReadAddrProt_DatReg <= (others => '0');
            
            AxiReadDataReady_RdyReg <= '0';
            AxiTimeout_CntReg <= 0;
            
            ConfigDone_ValOut <= '0';
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ConfigDone_ValOut <= '0';
            if ((AxiWriteAddrValid_ValReg = '1') and (AxiWriteAddrReady_RdyIn = '1')) then
                AxiWriteAddrValid_ValReg <= '0';
            end if;

            if ((AxiWriteDataValid_ValReg = '1') and (AxiWriteDataReady_RdyIn = '1')) then
                AxiWriteDataValid_ValReg <= '0';
            end if;

            if ((AxiWriteRespValid_ValIn = '1') and (AxiWriteRespReady_RdyReg = '1')) then
                AxiWriteRespReady_RdyReg <= '0';
            end if;

            if ((AxiReadAddrValid_ValReg = '1') and (AxiReadAddrReady_RdyIn = '1')) then
                AxiReadAddrValid_ValReg <= '0';
            end if;

            if ((AxiReadDataValid_ValIn = '1') and (AxiReadDataReady_RdyReg = '1')) then
                AxiReadDataReady_RdyReg <= '0';
            end if;


            case (ConfigState_StaReg) is
                when Idle_St =>
                    if (ConfigIndex_CntReg < MemoryConfigList_Con.ConfigListSize) then 
                        ConfigState_StaReg <= WaitConfig_St; -- make sure that data is valid independent of the state we were
                    else
                        ConfigState_StaReg <= End_St;
                    end if;
                    
                when WaitConfig_St => 
                    ConfigState_StaReg <= FetchConfig_St;
                    
                when FetchConfig_St =>
                    case (to_integer(unsigned(RomRead_DatReg(31 downto 0)))) is 
                        when 1 => 
                            ConfigCommand_DatReg <= Skip_E;
                            ConfigState_StaReg <= Skip_St;
                        when 2 => 
                            ConfigCommand_DatReg <= Wait_E;
                            ConfigState_StaReg <= Wait_St;
                        when 3 => 
                            ConfigCommand_DatReg <= Read_E;
                            ConfigState_StaReg <= StartReadWrite_St;
                        when 4 => 
                            ConfigCommand_DatReg <= Write_E;
                            ConfigState_StaReg <= StartReadWrite_St;
                        when others=> 
                            ConfigCommand_DatReg <= Unknown_E;
                            ConfigState_StaReg <= Skip_St;
                    end case;
                    ConfigBaseAddr_DatReg <= RomRead_DatReg(63 downto 32); 
                    ConfigRegAddr_DatReg <= RomRead_DatReg(95 downto 64);
                    ConfigData_DatReg <= RomRead_DatReg(127 downto 96);
                    ConfigIndex_CntReg <= ConfigIndex_CntReg + 1;

                when Skip_St => 
                    ConfigState_StaReg <= Idle_St;
                
                when Wait_St => 
                    if (unsigned(ConfigData_DatReg) >= ClockPeriod_Gen) then
                        ConfigData_DatReg <= std_logic_vector(unsigned(ConfigData_DatReg) - ClockPeriod_Gen);
                    else
                        ConfigState_StaReg <= Idle_St;
                    end if;
                    
                when StartReadWrite_St => 
                    if (ConfigCommand_DatReg = Read_E) then
                        AxiReadAddrValid_ValReg <= '1';
                        AxiReadAddrAddress_AdrReg <= std_logic_vector(unsigned(ConfigBaseAddr_DatReg) + unsigned(ConfigRegAddr_DatReg));
                        AxiReadAddrProt_DatReg <= "000";
                        AxiReadDataReady_RdyReg <= '1';
                        ConfigState_StaReg <= WaitToRead_St;
                    else
                        AxiWriteAddrValid_ValReg <= '1';
                        AxiWriteAddrAddress_AdrReg <= std_logic_vector(unsigned(ConfigBaseAddr_DatReg) + unsigned(ConfigRegAddr_DatReg));
                        AxiWriteAddrProt_DatReg <= "000";
                        AxiWriteDataValid_ValReg <= '1';
                        AxiWriteDataStrobe_DatReg <= "1111";
                        AxiWriteDataData_DatReg <= ConfigData_DatReg;
                        AxiWriteRespReady_RdyReg <= '1';
                        ConfigState_StaReg <= WaitToWrite_St;
                    end if;
                    AxiTimeout_CntReg <= 0;

                when WaitToRead_St =>
                    if ((AxiTimeout_Gen > 0) and (AxiTimeout_CntReg < (AxiTimeout_Gen-1))) then
                        AxiTimeout_CntReg <= AxiTimeout_CntReg + 1;
                    end if;

                    if ((AxiReadDataValid_ValIn = '1') and (AxiReadDataReady_RdyReg = '1')) then
                        AxiReadData_DatReg <= AxiReadDataData_DatIn;
                        AxiResponse_DatReg(2) <= '0'; -- no timeout
                        AxiResponse_DatReg(1 downto 0) <= AxiReadDataResponse_DatIn;
                        ConfigState_StaReg <= Idle_St;
                    elsif ((AxiTimeout_Gen > 0) and (AxiTimeout_CntReg >= (AxiTimeout_Gen-1))) then
                        AxiReadData_DatReg <= (others => '0');
                        AxiResponse_DatReg(2) <= '1'; -- timeout
                        AxiResponse_DatReg(1 downto 0) <= "00";
                        ConfigState_StaReg <= Idle_St;
                        -- this is a violation of the bus spec but better than blocking the bus forever
                        AxiReadAddrValid_ValReg <= '0';
                        AxiReadAddrAddress_AdrReg <= (others => '0');
                        AxiReadAddrProt_DatReg <= (others => '0');
                        AxiReadDataReady_RdyReg <= '0';
                    end if;
                
                when WaitToWrite_St =>
                    if ((AxiTimeout_Gen > 0) and (AxiTimeout_CntReg < (AxiTimeout_Gen-1))) then
                        AxiTimeout_CntReg <= AxiTimeout_CntReg + 1;
                    end if;

                    if ((AxiWriteRespValid_ValIn = '1') and (AxiWriteRespReady_RdyReg = '1')) then
                        AxiReadData_DatReg <= (others => '0');
                        AxiResponse_DatReg(2) <= '0'; -- no timeout
                        AxiResponse_DatReg(1 downto 0) <= AxiWriteRespResponse_DatIn;
                        ConfigState_StaReg <= Idle_St;
                    elsif ((AxiTimeout_Gen > 0) and (AxiTimeout_CntReg >= (AxiTimeout_Gen-1))) then
                        AxiReadData_DatReg <= (others => '0');
                        AxiResponse_DatReg(2) <= '1'; -- timeout
                        AxiResponse_DatReg(1 downto 0) <= "00";
                        ConfigState_StaReg <= Idle_St;
                        -- this is a violation of the bus spec but better than blocking the bus forever
                        AxiWriteAddrValid_ValReg <= '0';
                        AxiWriteAddrAddress_AdrReg <= (others => '0');
                        AxiWriteAddrProt_DatReg <= (others => '0');
                        AxiWriteDataValid_ValReg <= '0';
                        AxiWriteDataData_DatReg <= (others => '0');
                        AxiWriteDataStrobe_DatReg <= (others => '0');
                        AxiWriteRespReady_RdyReg <= '0';
                    end if;
                
                when End_St => 
                    ConfigState_StaReg <= End_St;
                    ConfigDone_ValOut <= '1';
                    
                when others =>
                    ConfigState_StaReg <= Idle_St;
                    
            end case;
        end if;
    end process Config_Prc;

    -- Separate the process to infer a block ram implementation of the Core List memory
    Rom_Prc : process(SysClk_ClkIn) is
    begin
        if ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            RomRead_DatReg <= RomData_Rom(to_integer(unsigned(RomAddress_AdrReg)));
        end if;    
    end process Rom_Prc;

    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************

end architecture ConfMaster_Arch;