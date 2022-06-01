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
library TimecardLib;
use TimecardLib.Timecard_Package.all;


--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The Clock Detector detects the available clock sources and selects the clocks to be   --
-- used. The selection is done with different clock selector and clock enable outputs.   --
-- The selection is according to a priority scheme and it can be overwritten via         --
-- registers of the AXI slave interface                                                  --
-------------------------------------------------------------------------------------------
entity ClockDetector is
    generic( 
        ClockSelect_Gen                             :       std_logic_vector(3 downto 0):="0000";
        PpsSelect_Gen                               :       std_logic_vector(1 downto 0):="00"
    );
    port (
        -- System
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;

        -- Clock Inputs
        Mhz10ClkSma_ClkIn                           : in    std_logic;
        Mhz10ClkMac_ClkIn                           : in    std_logic;
        Mhz10ClkDcxo1_ClkIn                         : in    std_logic;
        Mhz10ClkDcxo2_ClkIn                         : in    std_logic;

        -- Selected Clock output
        ClkMux1Select_EnOut                         : out   std_logic;
        ClkMux2Select_EnOut                         : out   std_logic;
        ClkMux3Select_EnOut                         : out   std_logic;
        ClkWiz2Select_EnOut                         : out   std_logic;

        ClockRstN_RstOut                            : out   std_logic;

        -- Config interface to PPS source select
        PpsSourceSelect_DatOut                      : out std_logic_vector(1 downto 0);
        PpsSourceAvailable_DatIn                    : in  std_logic_vector(3 downto 0);

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
end entity ClockDetector;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture ClockDetector_Arch of ClockDetector is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    constant NumberOfClocks_Con                     : natural := 4;
    
    -- PPS Generator version
    constant ClkDetMajorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant ClkDetMinorVersion_Con                 : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(1, 8));
    constant ClkDetBuildVersion_Con                 : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(0, 16));
    constant ClkDetVersion_Con                      : std_logic_vector(31 downto 0) := ClkDetMajorVersion_Con & ClkDetMinorVersion_Con & ClkDetBuildVersion_Con;

    -- AXI regs                                                       Addr       , Mask       , RW  , Reset
    constant ClkDetSourceSelected_Reg_Con           : Axi_Reg_Type:= (x"00000000", x"000000FF", Ro_E, x"00000000");
    constant ClkDetSourceSelect_Reg_Con             : Axi_Reg_Type:= (x"00000008", x"000000FF", Rw_E, (x"000000" & ClockSelect_Gen & "00" & PpsSelect_Gen));
    constant ClkDetVersion_Reg_Con                  : Axi_Reg_Type:= (x"00000010", x"FFFFFFFF", Ro_E, ClkDetVersion_Con);


    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type Common_ByteU_Type                          is array(natural range <>) of unsigned(7 downto 0);
    type Common_Natural_Type                        is array(natural range <>) of natural range 0 to 10000;

    type ClockSelection_State_Type                  is (Idle_St,
                                                        SelectClk_St, CheckClk_St,
                                                        End_St);

    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************
    signal MhzXClk_ClkIn                            : std_logic_vector(NumberOfClocks_Con-1 downto 0);

    signal ClockCounter_DatReg                      : Common_ByteU_Type(NumberOfClocks_Con-1 downto 0) := (others => (others =>'0'));
    signal ClockAliveTimeOut_DatReg                 : Common_Natural_Type(NumberOfClocks_Con-1 downto 0) := (others => 0);
    signal MhzSlowClk_Clk                           : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');
    signal MhzSlowClk_Clk_FF                        : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');
    signal MhzSlowClk_Clk_FFF                       : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');

    signal ClockAvailable_Dat                       : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');

    signal ClkSelected_Dat                          : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');
    signal ClkSelected_DatReg                       : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');
    signal ClkManualSelect_DatReg                   : std_logic_vector(NumberOfClocks_Con-1 downto 0) := (others => '0');

    signal ClockRst_ShiftReg                        : std_logic_vector(7 downto 0) := (others => '0');

    signal ClockSelection_StateStReg                : ClockSelection_State_Type;

    -- Manual Clock selection
    signal ClkManualSelect_Dat                      : std_logic_vector(NumberOfClocks_Con-1 downto 0);

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

    signal ClkDetSourceSelected_DatReg              : std_logic_vector(31 downto 0);
    signal ClkDetSourceSelect_DatReg                : std_logic_vector(31 downto 0);
    signal ClkDetVersion_DatReg                     : std_logic_vector(31 downto 0);

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    -- Stretch reset
    ClockRstN_RstOut <= '1' when ClockRst_ShiftReg = x"00" else '0';

    ClkManualSelect_Dat <= ClkDetSourceSelect_DatReg(7 downto 4);       -- manual clock selection
    PpsSourceSelect_DatOut <= ClkDetSourceSelect_DatReg(1 downto 0);    --forward the source PPS source selection to the output

    ------------------------------------------------------------
    -- Selected Clock
    --------------------------Mux1----Mux2----Mux3----Wiz2------
    -- Mhz10ClkSma_ClkIn        0       x       0       0
    -- Mhz10ClkMac_ClkIn        1       x       0       0
    -- Mhz10ClkDcxo1_ClkIn      x       0       1       0
    -- Mhz10ClkDcxo2_ClkIn      x       1       1       0
    -- Ext                      x       x       x       1

    -- PLL Select Logic
    ClkMux1Select_EnOut <= '1' when ClkSelected_Dat(1) = '1' else '0';
    ClkMux2Select_EnOut <= '1' when ClkSelected_Dat(3) = '1' else '0';
    ClkMux3Select_EnOut <= '1' when ClkSelected_Dat(2) = '1' or ClkSelected_Dat(3) = '1' else '0';
    ClkWiz2Select_EnOut <= '1' when ClkSelected_Dat = "0000" else '0';

    MhzXClk_ClkIn(0) <= Mhz10ClkSma_ClkIn;
    MhzXClk_ClkIn(1) <= Mhz10ClkMac_ClkIn;
    MhzXClk_ClkIn(2) <= Mhz10ClkDcxo1_ClkIn;
    MhzXClk_ClkIn(3) <= Mhz10ClkDcxo2_ClkIn;

    -- For each clock input create a slow clock (slower frequency by factor 128)
    SlowClk_Generate : for i in 0 to NumberOfClocks_Con-1 generate
        MhzSlowClk_Clk(i) <= ClockCounter_DatReg(i)(7);
    end generate SlowClk_Generate;

    -- AXI assignements
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

    -- 8-bit tick counter for each clock
    ClockCounter_Gen : for i in 0 to NumberOfClocks_Con-1 generate
        Counter_Prc : process(MhzXClk_ClkIn(i)) is
        begin
            if ((MhzXClk_ClkIn(i)'event) and (MhzXClk_ClkIn(i) = '1')) then
                ClockCounter_DatReg(i) <= ClockCounter_DatReg(i) + 1;
            end if;
        end process Counter_Prc;
    end generate ClockCounter_Gen;

    -- For each clock input, check its availability, based on the toggling of its slow clock
    ClockDetect_Gen : for i in 0 to NumberOfClocks_Con-1 generate
        Counter_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
        begin
            if (SysRstN_RstIn = '0') then
                MhzSlowClk_Clk_FF(i) <= '0';
                MhzSlowClk_Clk_FFF(i) <= '0';
                ClockAliveTimeOut_DatReg(i) <= 0;
                ClockAvailable_Dat(i) <= '0';
            elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
                MhzSlowClk_Clk_FF(i) <= MhzSlowClk_Clk(i);
                MhzSlowClk_Clk_FFF(i) <= MhzSlowClk_Clk_FF(i);

                if (MhzSlowClk_Clk_FFF(i) /=  MhzSlowClk_Clk_FF(i)) then
                    ClockAliveTimeOut_DatReg(i) <= 10000;
                    ClockAvailable_Dat(i) <= '1';
                elsif (ClockAliveTimeOut_DatReg(i) > 0) then
                    ClockAliveTimeOut_DatReg(i) <= ClockAliveTimeOut_DatReg(i) - 1;
                else
                    ClockAvailable_Dat(i) <= '0';
                end if;

            end if;
        end process Counter_Prc;

    end generate ClockDetect_Gen;

    -- Select the clock based on the availability, the default priority and, optionally, on a manual selection
    ClockSelect_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            ClkSelected_Dat <= (others => '0');
            ClkSelected_DatReg <= (others => '0');
            ClkManualSelect_DatReg <= (others => '0');

            ClockRst_ShiftReg <= (others => '0');

            ClockSelection_StateStReg <= Idle_St;
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClkSelected_DatReg <= ClkSelected_Dat;
            --Reset after new selection
            if(ClkSelected_DatReg /= ClkSelected_Dat) then
                ClockRst_ShiftReg <= ClockRst_ShiftReg(6 downto 0) & '1';
            else
                ClockRst_ShiftReg <= ClockRst_ShiftReg(6 downto 0) & '0';
            end if;

            case (ClockSelection_StateStReg) is
                when Idle_St =>
                    -- Automatic Selection
                    if(unsigned(ClkManualSelect_Dat) = 0)then
                        ClockSelection_StateStReg <= SelectClk_St;
                    else
                    -- Manual Selection
                        ClkManualSelect_DatReg <= ClkManualSelect_Dat;
                        ClockSelection_StateStReg <= CheckClk_St;
                    end if;

                when SelectClk_St =>
                    for i in 0 to NumberOfClocks_Con-1 loop
                        if (ClockAvailable_Dat(i) = '1') then
                            ClkSelected_Dat <= (others => '0');
                            ClkSelected_Dat(i) <= '1';
                            exit;
                        end if;
                    end loop;
                    ClockSelection_StateStReg <= Idle_St;

                when CheckClk_St  =>
                    for i in 0 to NumberOfClocks_Con-1 loop
                        if ((ClkManualSelect_DatReg(i) = '1') and (ClockAvailable_Dat(i) = '1')) then
                            ClkSelected_Dat <= (others => '0');
                            ClkSelected_Dat(i) <= '1';
                            ClockSelection_StateStReg <= Idle_St;
                            exit;
                        elsif ((ClkManualSelect_DatReg(i) = '1') and (ClockAvailable_Dat(i) = '0')) then
                            ClockSelection_StateStReg <= SelectClk_St;
                            exit;
                        else
                            ClockSelection_StateStReg <= Idle_St;
                        end if;
                    end loop;

                when others =>
                    ClockSelection_StateStReg <= Idle_St;

            end case;


        end if;
    end process ClockSelect_Prc;

    -- Access configuration and monitoring registers via an AXI4L slave
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress                : std_logic_vector(31 downto 0) := (others => '0');
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

            Axi_Init_Proc(ClkDetSourceSelected_Reg_Con, ClkDetSourceSelected_DatReg);
            Axi_Init_Proc(ClkDetSourceSelect_Reg_Con, ClkDetSourceSelect_DatReg);
            Axi_Init_Proc(ClkDetVersion_Reg_Con, ClkDetVersion_DatReg);

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
                        Axi_Read_Proc(ClkDetSourceSelected_Reg_Con, ClkDetSourceSelected_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClkDetSourceSelect_Reg_Con, ClkDetSourceSelect_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(ClkDetVersion_Reg_Con, ClkDetVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);

                        Axi_AccessState_StaReg <= Resp_St;
                    end if;

                when Write_St =>
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(ClkDetSourceSelected_Reg_Con, ClkDetSourceSelected_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClkDetSourceSelect_Reg_Con, ClkDetSourceSelect_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(ClkDetVersion_Reg_Con, ClkDetVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);

                        Axi_AccessState_StaReg <= Resp_St;
                    end if;

                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or
                        ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;
                    end if;

                when others =>

            end case;

            ClkDetSourceSelected_DatReg(7 downto 4) <= ClkSelected_Dat; -- report the selected clock
            ClkDetSourceSelected_DatReg(3 downto 0) <= PpsSourceAvailable_DatIn; -- receive the available PPS sources externally
        end if;
    end process Axi_Prc;


    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************

end architecture ClockDetector_Arch;