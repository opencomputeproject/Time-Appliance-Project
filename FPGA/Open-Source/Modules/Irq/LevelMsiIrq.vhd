--*****************************************************************************************
-- Project:     TimeSync
--
-- Author:      Thomas Schaub, NetTimeLogic GmbH
--
-- License:     Copyright (c) 2020, NetTimeLogic GmbH, Switzerland, <contact@nettimelogic.com>
--              All rights reserved.
--                
--              THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
--              ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
--              WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
--              DISCLAIMED. IN NO EVENT SHALL NetTimeLogic GmbH BE LIABLE FOR ANY
--              DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
--              (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
--              LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
--              ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
--              (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
--              SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
entity LevelMsiIrq is
    generic (
        RepeateInterruptNoOfClks_Gen    :       natural range 0 to 10000000 := 5000000;
        SinglePulseIrq_Gen              :       boolean := false
    );
    port (
        -- System
        SysClk_ClkIn                    : in    std_logic;
        SysRstN_RstIn                   : in    std_logic;

        -- Interrupt input         
        IrqIn_DatIn                     : in    std_logic;

        -- Interrupt output         
        IrqOut_DatOut                   : out   std_logic
    );
end entity LevelMsiIrq;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture LevelMsiIrq_Arch of LevelMsiIrq is
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
    type Level_State_Type                   is (Idle_St,
                                                LevelCounter_St, RegenerateEdge_St);

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal Level_State_StReg                : Level_State_Type;
    
    signal LevelCouter                      : natural range 0 to 10000000 := 5000000;
    
    signal IrqIn_DatReg                     : std_logic;
    
    --*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    IrqOut_DatOut <= IrqIn_DatReg;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    MsiGen_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is

    begin
        if (SysRstN_RstIn = '0') then
            Level_State_StReg <= Idle_St;

            IrqIn_DatReg <= '0';
            LevelCouter <= 0;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
        
            if (SinglePulseIrq_Gen = true) then
                IrqIn_DatReg <= '0';
            else
                IrqIn_DatReg <= IrqIn_DatIn;
            end if;
        
            case (Level_State_StReg) is
                when Idle_St =>
                    if (IrqIn_DatIn = '1') then
                        LevelCouter <= 0;
                        IrqIn_DatReg <= '1';
                        Level_State_StReg <= LevelCounter_St;
                    end if;
                
                when LevelCounter_St  =>
                    if (LevelCouter < RepeateInterruptNoOfClks_Gen) then
                        LevelCouter <= LevelCouter + 1;
                    else 
                        IrqIn_DatReg <= '0';
                        Level_State_StReg <= RegenerateEdge_St;
                    end if;
                
                when RegenerateEdge_St  =>
                    IrqIn_DatReg <= '0';
                    Level_State_StReg <= Idle_St;

                when others =>
                    Level_State_StReg <= Idle_St;
                    
            end case;
                
        end if;
    end process MsiGen_Prc;
    
    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
        
end architecture LevelMsiIrq_Arch;