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
-- The CoreList initializes a ROM with the core contents of CoreListFile_Gen and         -- 
-- provides them to the CPU via an AXI slave. When an invalid core type nr is read       --
-- (0x00000000), the CoreListReadCompleted_DatOut is activated                           --
-------------------------------------------------------------------------------------------                                                         
entity CoreList is
    generic (
        CoreListFile_Gen                            :       string := "NA";
        ClockPeriod_Gen                             :       natural := 20 -- System clock 50MHz. Clock period in nanoseconds
    );                  
    port (                  
        -- System                   
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
                    
        -- Core List Read                   
        CoreListReadCompleted_DatOut                : out   std_logic;
                            
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
end entity CoreList;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture CoreList_Arch of CoreList is

    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Configuration Definitions
    --*************************************************************************************
    constant MaxEntries_Con                         : natural := 2**12; -- change this if you need more than 2**12 entries in the list
    
    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant AddrWidth_Con                          : natural := integer(ceil(log2(real(MaxEntries_Con))));
    constant TextWordWidth_Con                      : natural := 9; -- in ascii characters, the text length should be 4 byte aligned   
    constant TextCharWidth_Con                      : natural := (4*TextWordWidth_Con); -- in ascii characters, the text length should be 4 byte aligned, up to 36 characters
    constant LineCharWidth_Con                      : natural := ((8*7)+7+TextCharWidth_Con); -- line length in ascii characters, including the 7 8-digit values, the 7 'space' delimiters and the text  
        
    constant BytesPerLine_Con                       : natural := ((4*7)+TextCharWidth_Con); -- line length in bytes
    constant WordsPerLine_Con                       : natural := natural(ceil(real(BytesPerLine_Con/4))); -- line length in 4-byte words
        
    -- Core List version    
    constant CoreListMajorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant CoreListMinorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant CoreListBuildVersion_Con               : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant CoreListVersion_Con                    : std_logic_vector(31 downto 0) := CoreListMajorVersion_Con & CoreListMinorVersion_Con & CoreListBuildVersion_Con;

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type RomReadState_Type                          is (ReadWait_St, ReadDone_St);                 
    
    subtype RomEntry_Type                           is std_logic_vector(31 downto 0);
    type Rom_Type                                   is array(natural range <>) of std_logic_vector(31 downto 0);
                    
    type CoreList_Type is record                       
        CoreListData                                : Rom_Type(((MaxEntries_Con*WordsPerLine_Con)-1) downto 0);
        CoreListSize                                : natural range 0 to MaxEntries_Con;
    end record CoreList_Type;

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************
    
    -- Define an impure function for parsing the text file CoreListFile_Gen and assigning its contents to the MemoryCoreList
    -- Only the valid lines are assigned to the output, the empty/commented lines are skipped
    -- One additional NULL entry is added as valid entry at the end of the list
    -- The expected CoreListFile_Gen format:
    -- - each core is described in a text line (up to 99 ascii characters per line)  
    -- - the format of a line in ascii characters is:
    --      - if the first ascii character is NUL/CR/LF/HT or comment "--", "//", then skip the line
    --      - 1 - 9: 8 hex digits (CoreTypeNr)    followed by 1 'space' character. Otherwise, report failure 
    --      - 10-18: 8 hex digits (CoreNstanceNr) followed by 1 'space' character. Otherwise, report failure
    --      - 19-27: 8 hex digits (VersionNr)     followed by 1 'space' character. Otherwise, report failure
    --      - 28-36: 8 hex digits (AddrRangeLow)  followed by 1 'space' character. Otherwise, report failure
    --      - 37-45: 8 hex digits (AddrRangeHigh) followed by 1 'space' character. Otherwise, report failure
    --      - 46-54: 8 hex digits (InterruptNr) followed by 1 'space' character. Otherwise, report failure
    --      - 55-62: 8 hex digits (Sensitivity). Otherwise, report failure    
    --      - 63   : if the character is NUL/CR/LF/HT, the line is accepted up to char 62 and there is no magic word for the core. Otherwise, either the next character is 'space' or report failure
    --      - 64-99: up to 36 ascii characters of text, until a special char NUL/CR/LF/HT is encountered or the size limit is reached. If more characters exist, they are ignored.
    impure function LoadCoreList_Func(CoreListFile : string) return CoreList_Type is
        variable CoreList                           : CoreList_Type := ((others => (others => '0')),0);
        file InputFile                              : text;
        variable TempLine                           : line;
        variable TempChar                           : character;
        variable NotEol                             : boolean;
        variable TempLineString                     : string(1 to LineCharWidth_Con);
        variable TempLineLength                     : natural;
        variable TempLineIndex                      : natural;
        variable TempValue1                         : unsigned(31 downto 0);
        variable TempValue2                         : unsigned(31 downto 0);
        variable TempValue3                         : unsigned(31 downto 0);
        variable TempValue4                         : unsigned(31 downto 0);
        variable TempValue5                         : unsigned(31 downto 0);
        variable TempValue6                         : unsigned(31 downto 0);
        variable TempValue7                         : unsigned(31 downto 0);
        variable TempText                           : std_logic_vector(((TextCharWidth_Con*8)-1) downto 0);
        variable TempInteger                        : natural range 0 to 17;
        variable SkipLine                           : boolean;
    begin
        CoreList := ((others => (others => '0')), 0);
        file_open(InputFile, CoreListFile, READ_MODE);
        TempLineIndex := 0;
        TempLineLength := 1;
        while ((TempLineIndex < (MaxEntries_Con-1)) and -- ensure not out of bounds
               (not endfile(InputFile))) loop
            readline(InputFile,TempLine);
            SkipLine := false;
            TempValue1 := (others => '0');
            TempValue2 := (others => '0');
            TempValue3 := (others => '0');
            TempValue4 := (others => '0');
            TempValue5 := (others => '0');
            TempValue6 := (others => '0');
            TempValue7 := (others => '0');
            TempText := (others => '0');
            TempLineString := (others => NUL);

            TempLineLength := TempLine'length;
            if (TempLineLength >= LineCharWidth_Con) then
                TempLineLength := LineCharWidth_Con;
            end if;
            for i in 1 to TempLineLength loop
                read(TempLine, TempChar);
                TempLineString(i) := TempChar;
            end loop;
            if ((TempLineString(1) = ' ') or (TempLineString(1) = NUL) or (TempLineString(1) = CR) or (TempLineString(1) = LF) or (TempLineString(1) = HT) or -- the new line starts with some special character 
                (TempLineString(1 to 2)= "//") or (TempLineString(1 to 2)= "--" )) then -- no comment lines
                SkipLine := true;
                report "Skip empty/commented line" severity note;
            elsif (TempLineLength < (LineCharWidth_Con-TextCharWidth_Con-1)) then  -- at least certain characters are expected per line, the string text is optional
                assert false report "Too short line found at the Core List" severity failure;
                SkipLine := true;
            else    
                report "======================================" severity note; 
                report "Corelist Index: " & integer'image(TempLineIndex) severity note; 
                report "Corelist Line: " & TempLineString(1 to TempLineLength) severity note; 
                report "CoreList Line Length: " & integer'image(TempLineLength) severity note; 
                TempText := (others => '0');
                for i in 1 to TempLineLength loop
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
                        when others => 
                            TempInteger := 17;
                            if i<63 then 
                                assert false report "Unexpected char at the Core List" severity failure;
                                SkipLine := true;
                            end if;    
                    end case;
                    if (i <= 8) then 
                        if (TempInteger < 16) then -- only hex characters
                            TempValue1 := resize((TempValue1*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Type Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 9) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Type Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 17) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue2 := resize((TempValue2*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Instance Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 18) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Instance Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 26) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue3 := resize((TempValue3*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Version Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 27) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Version Number" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 35) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue4 := resize((TempValue4*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Address Range Low" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 36) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Address Range Low" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 44) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue5 := resize((TempValue5*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Address Range High" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 45) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Address Range High" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 53) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue6 := resize((TempValue6*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Interrupt Nr" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 54) then  
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Interrput Nr" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i <= 62) then    
                        if (TempInteger < 16) then -- only hex characters
                            TempValue7 := resize((TempValue7*16),32) + TempInteger;
                        else
                            assert false report "Unexpected char at the Core Interrupt Sensitivity" severity failure;
                            SkipLine := true;
                        end if;
                    elsif (i = 63) then    
                        if (TempInteger /= 16) then -- no space character
                            assert false report "Missing space char after the Core Interrput Sensitivity" severity failure;
                            SkipLine := true;
                        end if;
                    else
                        -- Store the first ASCII word at the lowest address and the last ASCII word at the highest address
                        TempText((((i-64)*8)+7) downto ((i-64)*8)) := std_logic_vector(to_unsigned(character'pos(TempLineString(i)),8));
                    end if;    
                end loop;    
                if (SkipLine = false) then  
                    CoreList.CoreListData(TempLineIndex*WordsPerLine_Con) := std_logic_vector(TempValue1);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 1) := std_logic_vector(TempValue2);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 2) := std_logic_vector(TempValue3);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 3) := std_logic_vector(TempValue4);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 4) := std_logic_vector(TempValue5);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 5) := std_logic_vector(TempValue6);
                    CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 6) := std_logic_vector(TempValue7);
                    for k in 0 to (TextWordWidth_Con-1) loop
                        -- invert the byte order in the word. The leftest ascii char is registered as the MSB of the word
                        CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 7 + k)(7 downto 0) := std_logic_vector(TempText(32*k+31 downto 32*k+24));
                        CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 7 + k)(15 downto 8) := std_logic_vector(TempText(32*k+23 downto 32*k+16));
                        CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 7 + k)(23 downto 16) := std_logic_vector(TempText(32*k+15 downto 32*k+8));
                        CoreList.CoreListData((TempLineIndex*WordsPerLine_Con) + 7 + k)(31 downto 24) := std_logic_vector(TempText(32*k+7 downto 32*k));
                    end loop;
                    report "Completed reading line " & integer'image(TempLineIndex) & " correctly.";
                    TempLineIndex := TempLineIndex + 1;
                else
                    report "Skip line" ;
                end if;    
            end if;
        end loop;
        file_close(InputFile);
        -- add a NULL entry to indicate the end of the list
        CoreList.CoreListData((((TempLineIndex+1)*WordsPerLine_Con)-1) downto (TempLineIndex*WordsPerLine_Con)) := (others => (others => '0'));
        TempLineIndex := TempLineIndex + 1;
        CoreList.CoreListSize := TempLineIndex;
        return CoreList;
    end function LoadCorelist_Func;
        
    --*************************************************************************************
    -- Constant Definitions (only after file handling)
    --*************************************************************************************
    constant MemoryCoreList_Con                     : CoreList_Type := LoadCoreList_Func(CoreListFile_Gen);
    -- 64 bytes per CoreList line are allocated: 7*4 bytes for values and the rest for text(up to 36 ascii characters)
    constant CoreListBytes_Con                      : natural := MemoryCoreList_Con.CoreListSize * BytesPerLine_Con;           
    
    constant RomAddrWidth_Con                       : natural := integer(ceil(log2(real((MemoryCoreList_Con.CoreListSize * WordsPerLine_Con)))));
    
    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal CoreListReadCompleted_DatReg             : std_logic;
    
    -- Memory read signals 
    signal RomReadState_StaReg                      : RomReadState_Type;
    signal RomAddress_AdrReg                        : std_logic_vector((RomAddrWidth_Con-1) downto 0);
    signal RomRead_DatReg                           : std_logic_vector(31 downto 0) := (others => '0');
    signal RomData_Rom                              : Rom_Type(((2**RomAddrWidth_Con)-1) downto 0) := MemoryCoreList_Con.CoreListData(((2**RomAddrWidth_Con)-1) downto 0);
    -- to enforce usage of a Ramblock
    attribute ram_style                             : string;
    attribute ram_style of RomData_Rom              : signal is "block";
    
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
    signal AxiReadDone_ValReg                       : std_logic;
    
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
    
    CoreListReadCompleted_DatOut                    <= CoreListReadCompleted_DatReg;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************

    -- Axi slave for accessing the Core List registers 
    -- The format of the core list ROM which is provided via AXI is 
    -- - each core is described by a set of 64 8-bit registers (512 bits):
    -- - the format of each core in bits is:
    --      - ByteAddr[03-00]: 31 - 0  CoreTypeNr(4B) 
    --      - ByteAddr[07-04]: 63 - 32 CoreInstNr(4B)
    --      - ByteAddr[0B-08]: 95 - 64 Version(4B)
    --      - ByteAddr[0F-0C]: 127- 96 AddressRangeLow(4B)
    --      - ByteAddr[13-10]: 159-128 AddressRangeHigh(4B)
    --      - ByteAddr[17-14]: 191-160 InterruptMask(4B) 
    --      - ByteAddr[1B-18]: 223-192 Sensitivity(4B) 
    --      - ByteAddr[3F-1C]: 511-224 Magic word(max 36B of  ascii chars) 
    -- Only word-aligned addresses are accessing the ROM 
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress                : std_logic_vector(31 downto 0) := (others => '0');
    begin
        if (SysRstN_RstIn = '0') then
            CoreListReadCompleted_DatReg <= '0';
            
            AxiWriteAddrReady_RdyReg <= '0';
            AxiWriteDataReady_RdyReg <= '0';

            AxiWriteRespValid_ValReg <= '0';
            AxiWriteRespResponse_DatReg <= (others => '0');

            AxiReadAddrReady_RdyReg <= '0';

            AxiReadDataValid_ValReg <= '0';
            AxiReadDataResponse_DatReg <= (others => '0');
            AxiReadDataData_DatReg <= (others => '0');

            Axi_AccessState_StaReg <= Axi_AccessState_Type_Rst_Con;
            
            AxiReadDone_ValReg <= '0';
            
            RomAddress_AdrReg <= (others => '0');
            RomReadState_StaReg <= ReadWait_St;
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            -- just a pulse
            AxiReadDone_ValReg <= '0';
            
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
                    RomReadState_StaReg <= ReadWait_St;
                    
                when Read_St =>
                    if ((AxiReadAddrValid_ValIn = '1') and (AxiReadAddrReady_RdyReg = '1')) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiReadAddrAddress_AdrIn),32));
                        if (unsigned(TempAddress) >= (CoreListBytes_Con-1)) then -- larger than the CoreList size (Lines*BytesPerLine)
                            AxiReadDataValid_ValReg <= '1';
                            AxiReadDataResponse_DatReg <= Axi_RespOk_Con;
                            AxiReadDataData_DatReg <= (others => '0');
                            Axi_AccessState_StaReg <= Resp_St;
                        else 
                            RomAddress_AdrReg <= AxiReadAddrAddress_AdrIn(((2+RomAddrWidth_Con)-1) downto 2); -- divide the AXI address by 4, to get the ROM address
                            RomReadState_StaReg <= ReadWait_St;
                        end if;
                    else            
                        case RomReadState_StaReg is
                            when ReadWait_St => 
                                RomReadState_StaReg <= ReadDone_St;
                                
                            when ReadDone_St =>
                                AxiReadDataValid_ValReg <= '1';
                                AxiReadDataResponse_DatReg <= Axi_RespOk_Con;
                                AxiReadDataData_DatReg <= RomRead_DatReg;
                                Axi_AccessState_StaReg <= Resp_St;
                                AxiReadDone_ValReg <= '1';
                                RomReadState_StaReg <= ReadWait_St;
                                
                            when others =>
                                RomReadState_StaReg <= ReadWait_St;
                        end case;
                    end if;
                    
                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_AccessState_StaReg <= Resp_St;
                    end if;
                    
                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                       ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;
                    -- if a Core Type '0' is read, assume that the whole list has been read
                    if ((unsigned(AxiReadDataData_DatReg) = 0) and 
                        (unsigned(RomAddress_AdrReg(5 downto 0)) = 0) and 
                        (AxiReadDone_ValReg = '1')) then 
                        CoreListReadCompleted_DatReg <= '1'; -- sticky bit
                    end if;
                    
                when others =>
                    Axi_AccessState_StaReg <= Idle_St;
                    
            end case;
        end if;
    end process Axi_Prc;

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
    

end architecture CoreList_Arch;
