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
entity SinglePulseIrq is

    port (
        -- System
        SysClk_ClkIn                    : in    std_logic;
        SysRstN_RstIn                   : in    std_logic;

        -- Interrupt input         
        IrqIn_DatIn                     : in    std_logic;

        -- Interrupt output         
        IrqOut_DatOut                   : out   std_logic
    );
end entity SinglePulseIrq;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture SinglePulseIrq_Arch of SinglePulseIrq is
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

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal IrqIn_DatReg                     : std_logic;
    signal IrqIn_DatOutReg                  : std_logic;
    
    --*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    IrqOut_DatOut <= IrqIn_DatOutReg;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    SignlePulsIrq_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            IrqIn_DatReg <= '0';
            IrqIn_DatOutReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            IrqIn_DatReg <= IrqIn_DatIn;
            IrqIn_DatOutReg <= '0';
     
            if (IrqIn_DatIn = '1' and IrqIn_DatReg = '0') then
                IrqIn_DatOutReg <= '1';
            end if;
        end if;
    end process SignlePulsIrq_Prc;
    
    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
        
end architecture SinglePulseIrq_Arch;