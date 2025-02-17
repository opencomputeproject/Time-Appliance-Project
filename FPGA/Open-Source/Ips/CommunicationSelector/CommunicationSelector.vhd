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
-- Entity Declaration
--*****************************************************************************************
-- Configure UART or I2C communication based on a selection input.                       --
-------------------------------------------------------------------------------------------
entity CommunicationSelector is
    port (
        SelIn_DatIn                     : in    std_logic;         -- '0' UART; '1' I2C

        -- IO Pins 
        TxScl_DatIn                     : in    std_logic;
        TxScl_DatOut                    : out   std_logic;
        TxSclT_EnaOut                   : out   std_logic;
        RxSda_DatIn                     : in    std_logic;
        RxSda_DatOut                    : out   std_logic;
        RxSdaT_EnaOut                   : out   std_logic;
    
        Irq_DatOut                      : out   std_logic;

        -- UART Interface to IP
        UartTx_DatIn                    : in    std_logic;
        UartRx_DatOut                   : out   std_logic;

        UartIrq_DatIn                   : in    std_logic;

        -- I2C Interface to IP
        I2cSclIn_DatOut                 : out   std_logic;
        I2cSclOut_DatIn                 : in    std_logic;
        I2cSclT_EnaIn                   : in    std_logic;
 
        I2cSdaIn_DatOut                 : out   std_logic;
        I2cSdaOut_DatIn                 : in    std_logic;
        I2cSdaT_EnaIn                   : in    std_logic;
        
        I2cIrq_DatIn                    : in    std_logic
    );
end entity CommunicationSelector;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture CommunicationSelector_Arch of CommunicationSelector is
    --*************************************************************************************
    -- Component Definitions
    --*************************************************************************************

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
    signal TxSclOut_Dat                 : std_logic;
    signal TxSclIn_Dat                  : std_logic;
    signal TxSclT_Ena                   : std_logic;
    
    signal RxSdaOut_Dat                 : std_logic;
    signal RxSdaIn_Dat                  : std_logic;
    signal RxSdaT_Ena                   : std_logic;
    
--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    ------------------------------------------
    -- To IO Pins
    ------------------------------------------
    TxScl_DatOut <= TxSclOut_Dat;
    TxSclIn_Dat <= TxScl_DatIn;
    TxSclT_EnaOut <= TxSclT_Ena;
    
    RxSda_DatOut <= RxSdaOut_Dat;
    RxSdaIn_Dat <= RxSda_DatIn;
    RxSdaT_EnaOut <= RxSdaT_Ena;
    
    ------------------------------------------
    -- MUX
    ------------------------------------------
    -- Irq Mapping
    Irq_DatOut          <= UartIrq_DatIn when (SelIn_DatIn = '0') else I2cIrq_DatIn;
    
    -- Tx and SCL  Mapping
    TxSclOut_Dat        <= UartTx_DatIn when (SelIn_DatIn = '0') else I2cSclOut_DatIn;
    I2cSclIn_DatOut     <= '0' when (SelIn_DatIn = '0') else TxSclIn_Dat;
    -- Tristate = 0 --> I = I; IO = I; O = I
    TxSclT_Ena          <= '0' when (SelIn_DatIn = '0') else I2cSclT_EnaIn;
   
    -- Rx and SDA Mapping
    RxSdaOut_Dat        <= '0' when (SelIn_DatIn = '0') else I2cSdaOut_DatIn;
    I2cSdaIn_DatOut     <= '0' when (SelIn_DatIn = '0') else RxSdaIn_Dat;
    -- Tristate = 1 --> I = X; IO = Z; O = IO
    RxSdaT_Ena          <= '1' when (SelIn_DatIn = '0') else I2cSdaT_EnaIn;
    
    UartRx_DatOut       <= RxSdaIn_Dat when (SelIn_DatIn = '0') else '0';
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************

    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
    
end architecture CommunicationSelector_Arch;