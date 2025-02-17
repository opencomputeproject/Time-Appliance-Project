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

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The MSI IRQ receives single interrupts of the FPGA cores and puts them into a         --
-- message for the Xilinx AXI-PCIe bridge core. Once a message is ready a request is set --
-- and it waits until the grant signal from the Xilinx Core.                             --
-- If there are several interrupts pending the messages are sent with the round-robin    --
-- principle. It supports up to 32 Interrupt Requests                                    --
-------------------------------------------------------------------------------------------
entity MsiIrq is
    generic (
        NumberOfInterrupts_Gen                      :       natural range 0 to 32:= 5;
        LevelInterrupt_Gen                          :       std_logic_vector(31 downto 0) := x"0000_0000"
    );          
    port (          
        -- System           
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
            
        -- Interrupt inputs                     
        IrqIn0_DatIn                                : in    std_logic := '0';
        IrqIn1_DatIn                                : in    std_logic := '0';
        IrqIn2_DatIn                                : in    std_logic := '0';
        IrqIn3_DatIn                                : in    std_logic := '0';
        IrqIn4_DatIn                                : in    std_logic := '0';
        IrqIn5_DatIn                                : in    std_logic := '0';
        IrqIn6_DatIn                                : in    std_logic := '0';
        IrqIn7_DatIn                                : in    std_logic := '0';
        IrqIn8_DatIn                                : in    std_logic := '0';
        IrqIn9_DatIn                                : in    std_logic := '0';
        IrqIn10_DatIn                               : in    std_logic := '0';
        IrqIn11_DatIn                               : in    std_logic := '0';
        IrqIn12_DatIn                               : in    std_logic := '0';
        IrqIn13_DatIn                               : in    std_logic := '0';
        IrqIn14_DatIn                               : in    std_logic := '0';
        IrqIn15_DatIn                               : in    std_logic := '0';
        IrqIn16_DatIn                               : in    std_logic := '0';
        IrqIn17_DatIn                               : in    std_logic := '0';
        IrqIn18_DatIn                               : in    std_logic := '0';
        IrqIn19_DatIn                               : in    std_logic := '0';
        IrqIn20_DatIn                               : in    std_logic := '0';
        IrqIn21_DatIn                               : in    std_logic := '0';
        IrqIn22_DatIn                               : in    std_logic := '0';
        IrqIn23_DatIn                               : in    std_logic := '0';
        IrqIn24_DatIn                               : in    std_logic := '0';
        IrqIn25_DatIn                               : in    std_logic := '0';
        IrqIn26_DatIn                               : in    std_logic := '0';
        IrqIn27_DatIn                               : in    std_logic := '0';
        IrqIn28_DatIn                               : in    std_logic := '0';
        IrqIn29_DatIn                               : in    std_logic := '0';
        IrqIn30_DatIn                               : in    std_logic := '0';
        IrqIn31_DatIn                               : in    std_logic := '0';
            
        -- MSI Interface            
        MsiIrqEnable_EnIn                           : in    std_logic;
            
        MsiGrant_ValIn                              : in    std_logic;
        MsiReq_ValOut                               : out   std_logic;
            
        MsiVectorWidth_DatIn                        : in    std_logic_vector(2 downto 0); -- unused
        MsiVectorNum_DatOut                         : out   std_logic_vector(4 downto 0)
    );
end entity MsiIrq;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture MsiIrq_Arch of MsiIrq is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type Msi_State_Type                             is (Idle_St,
                                                        SelectIrq_St, SendIrq_St, WaitGrant_St, 
                                                        End_St);

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Msi_State_StReg                          : Msi_State_Type;
            
    signal MsiReq_ValReg                            : std_logic := '0';
    signal MsiVectorNum_DatReg                      : std_logic_vector(4 downto 0) := (others => '0');
    
    signal IrqInMax_Dat                             : std_logic_vector((31) downto 0) := (others => '0'); -- max number of interrupts is 32
    signal IrqIn_Dat                                : std_logic_vector(NumberOfInterrupts_Gen-1 downto 0) := (others => '0');
    signal IrqIn_DatReg                             : std_logic_vector(NumberOfInterrupts_Gen-1 downto 0) := (others => '0');
    signal IrqIn_Dat_ff                             : std_logic_vector(NumberOfInterrupts_Gen-1 downto 0) := (others => '0');
    signal IrqDetected_Reg                          : std_logic_vector(NumberOfInterrupts_Gen-1 downto 0) := (others => '0');
            
    signal IrqNumber                                : natural range 0 to NumberOfInterrupts_Gen-1 := 0;
            
    attribute ASYNC_REG                             : string;
    attribute ASYNC_REG of IrqIn_DatReg             : signal is "TRUE";
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    MsiReq_ValOut <= MsiReq_ValReg;
    MsiVectorNum_DatOut <= MsiVectorNum_DatReg;
    
    -- assign the irq signals to vector of max size
    IrqInMax_Dat(31) <= IrqIn31_DatIn when NumberOfInterrupts_Gen = 32 else '0';
    IrqInMax_Dat(30) <= IrqIn30_DatIn when NumberOfInterrupts_Gen >= 31 else '0';
    IrqInMax_Dat(29) <= IrqIn29_DatIn when NumberOfInterrupts_Gen >= 30 else '0';
    IrqInMax_Dat(28) <= IrqIn28_DatIn when NumberOfInterrupts_Gen >= 29 else '0';
    IrqInMax_Dat(27) <= IrqIn27_DatIn when NumberOfInterrupts_Gen >= 28 else '0';
    IrqInMax_Dat(26) <= IrqIn26_DatIn when NumberOfInterrupts_Gen >= 27 else '0';
    IrqInMax_Dat(25) <= IrqIn25_DatIn when NumberOfInterrupts_Gen >= 26 else '0';
    IrqInMax_Dat(24) <= IrqIn24_DatIn when NumberOfInterrupts_Gen >= 25 else '0';
    IrqInMax_Dat(23) <= IrqIn23_DatIn when NumberOfInterrupts_Gen >= 24 else '0';
    IrqInMax_Dat(22) <= IrqIn22_DatIn when NumberOfInterrupts_Gen >= 23 else '0';
    IrqInMax_Dat(21) <= IrqIn21_DatIn when NumberOfInterrupts_Gen >= 22 else '0';
    IrqInMax_Dat(20) <= IrqIn20_DatIn when NumberOfInterrupts_Gen >= 21 else '0';
    IrqInMax_Dat(19) <= IrqIn19_DatIn when NumberOfInterrupts_Gen >= 20 else '0';
    IrqInMax_Dat(18) <= IrqIn18_DatIn when NumberOfInterrupts_Gen >= 19 else '0';
    IrqInMax_Dat(17) <= IrqIn17_DatIn when NumberOfInterrupts_Gen >= 18 else '0';
    IrqInMax_Dat(16) <= IrqIn16_DatIn when NumberOfInterrupts_Gen >= 17 else '0';
    IrqInMax_Dat(15) <= IrqIn15_DatIn when NumberOfInterrupts_Gen >= 16 else '0';
    IrqInMax_Dat(14) <= IrqIn14_DatIn when NumberOfInterrupts_Gen >= 15 else '0';
    IrqInMax_Dat(13) <= IrqIn13_DatIn when NumberOfInterrupts_Gen >= 14 else '0';
    IrqInMax_Dat(12) <= IrqIn12_DatIn when NumberOfInterrupts_Gen >= 13 else '0';
    IrqInMax_Dat(11) <= IrqIn11_DatIn when NumberOfInterrupts_Gen >= 12 else '0';
    IrqInMax_Dat(10) <= IrqIn10_DatIn when NumberOfInterrupts_Gen >= 11 else '0';
    IrqInMax_Dat(9) <= IrqIn9_DatIn when NumberOfInterrupts_Gen >= 10 else '0';
    IrqInMax_Dat(8) <= IrqIn8_DatIn when NumberOfInterrupts_Gen >= 9 else '0';
    IrqInMax_Dat(7) <= IrqIn7_DatIn when NumberOfInterrupts_Gen >= 8 else '0';
    IrqInMax_Dat(6) <= IrqIn6_DatIn when NumberOfInterrupts_Gen >= 7 else '0';
    IrqInMax_Dat(5) <= IrqIn5_DatIn when NumberOfInterrupts_Gen >= 6 else '0';
    IrqInMax_Dat(4) <= IrqIn4_DatIn when NumberOfInterrupts_Gen >= 5 else '0';
    IrqInMax_Dat(3) <= IrqIn3_DatIn when NumberOfInterrupts_Gen >= 4 else '0';
    IrqInMax_Dat(2) <= IrqIn2_DatIn when NumberOfInterrupts_Gen >= 3 else '0';
    IrqInMax_Dat(1) <= IrqIn1_DatIn when NumberOfInterrupts_Gen >= 2 else '0';
    IrqInMax_Dat(0) <= IrqIn0_DatIn when NumberOfInterrupts_Gen >= 1 else '0';
    
    -- scale the irq max vector down to proper size
    IrqIn_Dat <= IrqInMax_Dat((NumberOfInterrupts_Gen-1) downto 0);
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    
    -- Send the interrupt requests one by one to the output (AXI-MSI bridge) and wait until the request is granted.
    MsiGen_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            Msi_State_StReg <= Idle_St;
            
            IrqIn_DatReg <= (others => '0');
            IrqIn_Dat_ff <= (others => '0');
            IrqDetected_Reg <= (others => '0');
            
            MsiReq_ValReg <= '0';
            MsiVectorNum_DatReg <= (others => '0');
            
            IrqNumber <= 0;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            if (MsiIrqEnable_EnIn = '1') then
            
                IrqIn_DatReg <= IrqIn_Dat;
                IrqIn_Dat_ff <= IrqIn_DatReg;

                -- provide the next interrupt
                case (Msi_State_StReg) is
                    when Idle_St =>
                        if (unsigned(IrqDetected_Reg) /= 0) then
                            Msi_State_StReg <= SelectIrq_St;
                        else
                            -- no more IRQ pending restart from 0
                            IrqNumber <= 0;
                        end if;
                    
                    when SelectIrq_St  =>
                        if(IrqDetected_Reg(IrqNumber) /= '0')then
                            Msi_State_StReg <= SendIrq_St;
                        else
                            if (IrqNumber >= NumberOfInterrupts_Gen-1) then
                                IrqNumber <= 0;
                            else
                                IrqNumber <= IrqNumber +1;
                            end if; 
                        end if;
                    
                    when SendIrq_St  =>
                        MsiReq_ValReg <= '1';
                        MsiVectorNum_DatReg <= std_logic_vector(to_unsigned(IrqNumber, MsiVectorNum_DatReg'length));
                        
                        Msi_State_StReg <= WaitGrant_St;
                        
                    when WaitGrant_St =>
                        MsiReq_ValReg <= '0';
                        if(MsiGrant_ValIn = '1') then 
                            -- Clear IrqEdge if no edge in this cycle
                            IrqDetected_Reg(IrqNumber) <= '0';
                            Msi_State_StReg <= End_St;
                        end if; 

                    when End_St =>
                        if (IrqNumber >= NumberOfInterrupts_Gen-1) then
                            IrqNumber <= 0;
                        else
                            IrqNumber <= IrqNumber +1;
                        end if; 
                        Msi_State_StReg <= Idle_St;
    
                    when others =>
                        Msi_State_StReg <= Idle_St;
                        
                end case;
                
                -- scan for a new interrupt
                for i in 0 to NumberOfInterrupts_Gen-1 loop 
                    if(((IrqIn_Dat_ff(i) = '0') and (IrqIn_DatReg(i) = '1')) or ((IrqIn_Dat_ff(i) = '1') and (LevelInterrupt_Gen(i) = '1'))) then
                        IrqDetected_Reg(i) <= '1';
                    end if;
                end loop;
            
            else
                Msi_State_StReg <= Idle_St;
                IrqIn_DatReg <= (others => '0');
                IrqDetected_Reg <= (others => '0');
            end if;
        end if;
    end process MsiGen_Prc;
    
    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
        
end architecture MsiIrq_Arch;