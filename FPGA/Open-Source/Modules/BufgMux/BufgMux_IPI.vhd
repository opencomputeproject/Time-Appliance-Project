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

--*****************************************************************************************
-- Specific Libraries
--*****************************************************************************************


--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
entity BufgMux_IPI is
    port (
        -- System
        ClkIn0_ClkIn                    : in    std_logic;
        ClkIn1_ClkIn                    : in    std_logic;
    
        SelecteClk1_EnIn                : in    std_logic;
        
        ClkOut_ClkOut                   : out   std_logic

    );
end entity BufgMux_IPI;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture BufgMux_Arch of BufgMux_IPI is
    --*************************************************************************************
    -- Component Definitions
    --*************************************************************************************
    component BUFGMUX
    generic (
        CLK_SEL_TYPE                :       string
    );
    port (
        o                           : out   std_logic;
        i0                          : in    std_logic; 
        i1                          : in    std_logic; 
        s                           : in    std_logic 
    );
    end component BUFGMUX;   

begin

    BufgMux_Inst : BUFGMUX
    generic map(
        CLK_SEL_TYPE    => "ASYNC"
    )
    port map(
        o               => ClkOut_ClkOut,
        i0              => ClkIn0_ClkIn,
        i1              => ClkIn1_ClkIn,
        s               => SelecteClk1_EnIn
    );
end BufgMux_Arch;
