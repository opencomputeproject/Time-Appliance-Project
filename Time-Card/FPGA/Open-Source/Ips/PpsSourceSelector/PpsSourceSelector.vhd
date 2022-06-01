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
-- The PPS Source Selector detects the available PPS sources and selects the PPS source --
-- according to a priority scheme and a configuration.                                  --
-- The configuration and monitoring of the core is optional and it is done by an        --
-- external interface (e.g. an AXI slave interface of Clock Detector). If the           --
-- configuration is not provided, then the PPS selection is done by a priority scheme   --
-- of the available PPS inputs.                                                         --
------------------------------------------------------------------------------------------
entity PpsSourceSelector is
    generic (
        ClockClkPeriodNanosecond_Gen    :       natural := 20;
        PpsAvailableThreshold_Gen       :       natural range 0 to 30 := 3
    );
    port (
        -- System
        SysClk_ClkIn                    : in    std_logic;
        SysRstN_RstIn                   : in    std_logic;

        -- Selection
        PpsSourceSelect_DatIn           : in    std_logic_vector(1 downto 0);
        
        -- PPS Available    
        PpsSourceAvailable_DatOut       : out   std_logic_vector(3 downto 0);

        -- PPS Inputs 
        SmaPps_EvtIn                    : in    std_logic;
        MacPps_EvtIn                    : in    std_logic;
        GnssPps_EvtIn                   : in    std_logic;
        
        -- PPS Outputs
        SlavePps_EvtOut                 : out   std_logic;
        MacPps_EvtOut                   : out   std_logic
    );
end entity PpsSourceSelector;

--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture PpsSourceSelector_Arch of PpsSourceSelector is
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
    
    signal PpsSourceAvailable_DatReg    : std_logic_vector(2 downto 0) := (others => '0');
    
    signal SmaPps_EvtReg                : std_logic;
    signal SmaPps_EvtFF                 : std_logic;
    signal MacPps_EvtReg                : std_logic;
    signal MacPps_EvtFF                 : std_logic;
    signal GnssPps_EvtReg               : std_logic;
    signal GnssPps_EvtFF                : std_logic;
    
    signal SmaPpsPulse_CntReg           : natural range 0 to 30;
    signal SmaPpsPeriod_CntReg          : natural;
    
    signal MacPpsPulse_CntReg           : natural range 0 to 30;
    signal MacPpsPeriod_CntReg          : natural;
    
    signal GnssPpsPulse_CntReg          : natural range 0 to 30;
    signal GnssPpsPeriod_CntReg         : natural;
    
    signal PpsSlaveSourceSelect_DatReg  : std_logic_vector(1 downto 0) := (others => '0');
    signal MacSourceSelect_DatReg       : std_logic_vector(1 downto 0) := (others => '0');

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    PpsSourceAvailable_DatOut <= "0" & PpsSourceAvailable_DatReg;

    with PpsSlaveSourceSelect_DatReg select SlavePps_EvtOut <=  SmaPps_EvtIn when "00",
                                                                MacPps_EvtIn when "01",
                                                                GnssPps_EvtIn when "10",
                                                                GnssPps_EvtIn when others;

    with MacSourceSelect_DatReg select MacPps_EvtOut        <=  SmaPps_EvtIn when "00",
                                                                GnssPps_EvtIn when "10",
                                                                GnssPps_EvtIn when others;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************
    
    -- Check that each PPS input has a frequency of ~1 Hz for at least PpsAvailableThreshold_Gen seconds in sequence
    Supervision_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            PpsSourceAvailable_DatReg <= (others => '0');
            
            SmaPps_EvtReg <= '0';
            SmaPps_EvtFF <= '0';
            MacPps_EvtReg <= '0';
            MacPps_EvtFF <= '0';
            GnssPps_EvtReg <= '0';
            GnssPps_EvtFF <= '0';
            
            SmaPpsPulse_CntReg <= 0;
            SmaPpsPeriod_CntReg <= 0;
            
            MacPpsPulse_CntReg <= 0;
            MacPpsPeriod_CntReg <= 0;
            
            GnssPpsPulse_CntReg <= 0;
            GnssPpsPeriod_CntReg <= 0;

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            SmaPps_EvtReg <= SmaPps_EvtIn;
            SmaPps_EvtFF <= SmaPps_EvtReg;
            MacPps_EvtReg <= MacPps_EvtIn;
            MacPps_EvtFF <= MacPps_EvtReg;
            GnssPps_EvtReg <= GnssPps_EvtIn;
            GnssPps_EvtFF <= GnssPps_EvtReg;
            
            if (SmaPpsPulse_CntReg = 0) then
                PpsSourceAvailable_DatReg(0) <= '0';
            elsif (SmaPpsPulse_CntReg >= PpsAvailableThreshold_Gen) then
                PpsSourceAvailable_DatReg(0) <= '1';
            end if;    

            if (MacPpsPulse_CntReg = 0) then
                PpsSourceAvailable_DatReg(1) <= '0';
            elsif (MacPpsPulse_CntReg >= PpsAvailableThreshold_Gen) then
                PpsSourceAvailable_DatReg(1) <= '1';
            end if;    

            if (GnssPpsPulse_CntReg = 0) then
                PpsSourceAvailable_DatReg(2) <= '0';
            elsif (GnssPpsPulse_CntReg >= PpsAvailableThreshold_Gen) then
                PpsSourceAvailable_DatReg(2) <= '1';
            end if;    

            -- SMA PPS
            if ((SmaPps_EvtReg = '1') and (SmaPps_EvtFF = '0')) then -- rising
                SmaPpsPeriod_CntReg <= 0;
                
                if (SmaPpsPeriod_CntReg < 900000000) then       -- too short -10%
                    if (SmaPpsPulse_CntReg > 0) then
                        SmaPpsPulse_CntReg <= SmaPpsPulse_CntReg - 1;
                    end if;
                elsif (SmaPpsPeriod_CntReg >= 1100000000) then  -- too long + 10%
                    if (SmaPpsPulse_CntReg > 0) then
                        SmaPpsPulse_CntReg <= SmaPpsPulse_CntReg - 1;
                    end if;
                else
                    if (SmaPpsPulse_CntReg < PpsAvailableThreshold_Gen) then
                        SmaPpsPulse_CntReg <= SmaPpsPulse_CntReg + 1;
                    end if;
                end if;
            else
                if (SmaPpsPeriod_CntReg < 1100000000) then
                    SmaPpsPeriod_CntReg <= SmaPpsPeriod_CntReg + ClockClkPeriodNanosecond_Gen;
                else
                    SmaPpsPeriod_CntReg <= 0;
                    if (SmaPpsPulse_CntReg > 0) then
                        SmaPpsPulse_CntReg <= SmaPpsPulse_CntReg - 1;
                    end if;
                end if;
            end if;
            
            -- MAC PPS
            if ((MacPps_EvtReg = '1') and (MacPps_EvtFF = '0')) then -- rising
                MacPpsPeriod_CntReg <= 0;
                
                if (MacPpsPeriod_CntReg < 900000000) then       -- too short -10%
                    if (MacPpsPulse_CntReg > 0) then
                        MacPpsPulse_CntReg <= MacPpsPulse_CntReg - 1;
                    end if;
                elsif (MacPpsPeriod_CntReg >= 1100000000) then  -- too long + 10%
                    if (MacPpsPulse_CntReg > 0) then
                        MacPpsPulse_CntReg <= MacPpsPulse_CntReg - 1;
                    end if;
                else
                    if (MacPpsPulse_CntReg < PpsAvailableThreshold_Gen) then
                        MacPpsPulse_CntReg <= MacPpsPulse_CntReg + 1;
                    end if;
                end if;
            else
                if (MacPpsPeriod_CntReg < 1100000000) then
                    MacPpsPeriod_CntReg <= MacPpsPeriod_CntReg + ClockClkPeriodNanosecond_Gen;
                else
                    MacPpsPeriod_CntReg <= 0;
                    if (MacPpsPulse_CntReg > 0) then
                        MacPpsPulse_CntReg <= MacPpsPulse_CntReg - 1;
                    end if;
                end if;
            end if;
            
            -- Gnss PPS
            if ((GnssPps_EvtReg = '1') and (GnssPps_EvtFF = '0')) then -- rising
                GnssPpsPeriod_CntReg <= 0;
                
                if (GnssPpsPeriod_CntReg < 900000000) then       -- too short -10%
                    if (GnssPpsPulse_CntReg > 0) then
                        GnssPpsPulse_CntReg <= GnssPpsPulse_CntReg - 1;
                    end if;
                elsif (GnssPpsPeriod_CntReg >= 1100000000) then  -- too long + 10%
                    if (GnssPpsPulse_CntReg > 0) then
                        GnssPpsPulse_CntReg <= GnssPpsPulse_CntReg - 1;
                    end if;
                else
                    if (GnssPpsPulse_CntReg < PpsAvailableThreshold_Gen) then
                        GnssPpsPulse_CntReg <= GnssPpsPulse_CntReg + 1;
                    end if;
                end if;
            else
                if (GnssPpsPeriod_CntReg < 1100000000) then
                    GnssPpsPeriod_CntReg <= GnssPpsPeriod_CntReg + ClockClkPeriodNanosecond_Gen;
                else
                    GnssPpsPeriod_CntReg <= 0;
                    if (GnssPpsPulse_CntReg > 0) then
                        GnssPpsPulse_CntReg <= GnssPpsPulse_CntReg - 1;
                    end if;
                end if;
            end if;
            
        end if;
        
    end process Supervision_Prc;
    
    -- Select the Slave PPS and MAC PPS sources according to configuration
    Select_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            PpsSlaveSourceSelect_DatReg <= (others => '0');
            MacSourceSelect_DatReg <= (others => '0');

        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            case (PpsSourceSelect_DatIn) is
                -- Auto select
                when "00" =>
                    -- 1. SMA
                    if (PpsSourceAvailable_DatReg(0) = '1') then
                        if (SmaPps_EvtIn = '0') then
                            PpsSlaveSourceSelect_DatReg <= "00";   -- SMA
                            MacSourceSelect_DatReg <= "00";        -- SMA
                        end if;
                    -- 2. MAC
                    elsif (PpsSourceAvailable_DatReg(1) = '1') then
                        if (MacPps_EvtIn = '0') then
                            PpsSlaveSourceSelect_DatReg <= "01";    -- MAC
                            if (GnssPps_EvtIn = '0') then
                                MacSourceSelect_DatReg <= "10";     -- GNSS
                            end if;
                        end if;
                    -- 3. GNSS
                    elsif (PpsSourceAvailable_DatReg(2) = '1') then
                        if (GnssPps_EvtIn = '0') then
                            PpsSlaveSourceSelect_DatReg <= "10";    -- GNSS
                            MacSourceSelect_DatReg <= "10";         -- GNSS
                        end if;
                    else
                        if (GnssPps_EvtIn = '0') then
                            PpsSlaveSourceSelect_DatReg <= "10";    -- GNSS
                            MacSourceSelect_DatReg <= "10";         -- GNSS
                        end if;
                    end if;
                    
                -- Force SMA
                when "01" =>
                    if (SmaPps_EvtIn = '0') then
                            PpsSlaveSourceSelect_DatReg <= "00";    -- SMA
                            MacSourceSelect_DatReg <= "00";         -- SMA
                    end if;
                -- Force MAC
                when "10" =>
                    if (MacPps_EvtIn = '0') then
                        PpsSlaveSourceSelect_DatReg <= "01";        -- MAC
                        if (GnssPps_EvtIn = '0') then        
                            MacSourceSelect_DatReg <= "10";         -- GNSS
                        end if;
                    end if;
                -- Force GNSS
                when "11" =>
                    if (GnssPps_EvtIn = '0') then
                        PpsSlaveSourceSelect_DatReg <= "10";        -- GNSS
                        MacSourceSelect_DatReg <= "10";             -- GNSS
                    end if;
                
                when others =>
                    if (GnssPps_EvtIn = '0') then
                        PpsSlaveSourceSelect_DatReg <= "10";        -- GNSS
                        MacSourceSelect_DatReg <= "10";             -- GNSS
                    end if;
                    
            end case;
            
        end if;
    end process Select_Prc;
    
    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
        
end architecture PpsSourceSelector_Arch;