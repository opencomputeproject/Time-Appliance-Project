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

--*****************************************************************************************
-- Specific Libraries
--*****************************************************************************************
library TimecardLib;
use TimecardLib.Timecard_Package.all;

--*****************************************************************************************
-- Entity Declaration
--*****************************************************************************************
-- The Time-Of-Day (ToD) slave provides adjustment to a Clock of the "seconds" field.    --
-- From a GNSS receiver, it receives, via a UART interface, messages that include the    --
-- ToD. The messages are detected and decoded in order to extract the time infrmation.   --
-- The extracted time is converted to Unix Epoch format (32bit seconds and 32bit         --
-- nanonseconds)and then the TAI is calculated, by adding the received UTC offset. The   --
-- calculated time is compared with the time received from the connected Clock. If the   --
-- "seconds" field is not the equal, then a time adjustment is applied to the Clock at   --
-- at the change of second.                                                              --
-- This core is expected to be connected to an Adjustable Clock together with a PPS      --
-- slave. The TOD slave forces the Clock to jump to the correct "second", while the PPS  --
-- slave fine-tunes the Clock by providing offset and drift adjustments.                 --
-------------------------------------------------------------------------------------------
entity TodSlave is
    generic (
        ClockPeriod_Gen                             :       natural := 20;
        UartDefaultBaudRate_Gen                     :       natural := 2; -- UBX allowed baudrate: 2=>4800 3=>9600 4=>19200 5=>38400 6=>57600 7=>115200 8=>230400 9=>460800
        UartPolarity_Gen                            :       boolean := true;
        ReceiveCurrentTime_Gen                      :       boolean := true; -- The GNSS receiver provides the time of the current second or the next second. Ublox receivers provide the current second.
        
        Sim_Gen                                     :       boolean := false
    );
    port (
        -- System
        SysClk_ClkIn                                : in    std_logic;
        SysRstN_RstIn                               : in    std_logic;
                    
        -- Time Input
        ClockTime_Second_DatIn                      : in    std_logic_vector((SecondWidth_Con-1) downto 0);
        ClockTime_Nanosecond_DatIn                  : in    std_logic_vector((NanosecondWidth_Con-1) downto 0);
        ClockTime_TimeJump_DatIn                    : in    std_logic;
        ClockTime_ValIn                             : in    std_logic;

        -- Tod Input
        RxUart_DatIn                                : in    std_logic;

        -- Time Adjustment Output                    
        TimeAdjustment_Second_DatOut                : out   std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustment_Nanosecond_DatOut            : out   std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
        TimeAdjustment_ValOut                       : out   std_logic;

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
end entity TodSlave;


--*****************************************************************************************
-- Architecture Declaration
--*****************************************************************************************
architecture TodSlave_Arch of TodSlave is
    --*************************************************************************************
    -- Procedure Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Function Definitions
    --*************************************************************************************

    --*************************************************************************************
    -- Type Definitions
    --*************************************************************************************
    type ClksPerUartBit_Array_Type                  is array(natural range <>) of natural;
    
    type UartRxState_Type                           is (Idle_St,
                                                        Start_St,
                                                        Data_St,
                                                        Stop_St);
    
    type DetectUbxMsgState_Type                     is (Idle_St,
                                                        -- UBX Header
                                                        UbxHeader_Sync_St,
                                                        UbxHeader_Class_St,
                                                        UbxHeader_MonId_St,
                                                        UbxHeader_NavId_St,
                                                        -- UBX Payload MON-HW
                                                        UbxMonHw_CheckPayload_St,
                                                        -- UBX Payload NAV-SAT
                                                        UbxNavSat_CheckPayload_St,
                                                        -- UBX Payload NAV-Status
                                                        UbxNavStatus_CheckPayload_St,
                                                        -- UBX Payload NAV-TIMELS
                                                        UbxNavTimeLs_CheckPayload_St,
                                                        -- UBX Payload NAV-TIMEUTC
                                                        UbxNavTimeUtc_CheckPayload_St,
                                                        -- UBX Checksum
                                                        UbxChecksum_ChecksumA_St,
                                                        UbxChecksum_ChecksumB_St);

    type DetectTsipMsgState_Type                    is (Idle_St,
                                                        TsipPacket_Id_St,
                                                        TsipPacket_SubId_St,
                                                        TsipLength1_St,
                                                        TsipLength2_St,
                                                        TsipMode_St,
                                                        TsipTiming_Data_St,
                                                        TsipPosition_Data_St,
                                                        TsipSatellite_Data_St,
                                                        TsipAlarms_Data_St,
                                                        TsipReceiver_Data_St,
                                                        TsipChecksum_St,
                                                        TsipEoF1_St,
                                                        TsipEoF2_St);

    type TaiConversionState_Type                    is (Idle_St,
                                                        ConvertYears_St,
                                                        ConvertMonths_St,
                                                        ConvertDays_St,
                                                        ConvertHours_St,
                                                        ConvertMinutes_St,
                                                        CalcTai_St,
                                                        TimeAdjust_St);

    type SecondsPerMonthArray_Type                  is array(1 to 12) of natural; 
    
    type UtcOffsetInfo_Type is record
        Leap59                                      : std_logic;
        Leap61                                      : std_logic;
        LeapAnnouncement                            : std_logic;
        LeapChangeValid                             : std_logic;
        SrcOfCurLeapSecond                          : std_logic_vector(7 downto 0);
        TimeToLeapSecond                            : std_logic_vector(31 downto 0);
        TimeToLeapSecondValid                       : std_logic;
        CurrentUtcOffset                            : std_logic_vector(7 downto 0);
        CurrentUtcOffsetValid                       : std_logic;
        CurrentTaiGnssOffset                        : std_logic_vector(7 downto 0);
        CurrentTaiGnssOffsetValid                   : std_logic;    
    end record;

    type ToD_Type is record
        Year                                        : std_logic_vector(11 downto 0); -- 1999-2099
        Month                                       : std_logic_vector(3 downto 0); -- 1-12
        Day                                         : std_logic_vector(4 downto 0); -- 1-31
        Hour                                        : std_logic_vector(4 downto 0); -- 1-24
        Minute                                      : std_logic_vector(5 downto 0); -- 0-59
        Second                                      : std_logic_vector(5 downto 0); -- 0-60
        Valid                                       : std_logic;
    end record;

    type SatInfo_Type is record
        NumberOfSeenSats                            : std_logic_vector(7 downto 0);
        NumberOfLockedSats                          : std_logic_vector(7 downto 0); -- the Satellite is counted in if it has flags: (QualityInd>=4 and Health==1)
    end record;
    
    type AntennaInfo_Type is record
        Status                                      : std_logic_vector(2 downto 0); -- 0=INIT, 1=DONTKNOW, 2=OK, 3=SHORT, 4=OPEN
        JamState                                    : std_logic_vector(1 downto 0);
        JamInd                                      : std_logic_vector(7 downto 0);
    end record;
    
    type AntennaFix_Type is record
        GnssFix                                     : std_logic_vector(7 downto 0);
        GnssFixOk                                   : std_logic;
        SpoofDetState                               : std_logic_vector(1 downto 0);
    end record;
    
    --*************************************************************************************
    -- Constant Definitions
    --*************************************************************************************
    -- RX UART 
    constant ClksPerUartBit_Array_Con               : ClksPerUartBit_Array_Type(2 to 9) := (
                                                        2 => ((1000000000 / 4800) / ClockPeriod_Gen),
                                                        3 => ((1000000000 / 9600) / ClockPeriod_Gen),
                                                        4 => ((1000000000 / 19200) / ClockPeriod_Gen),
                                                        5 => ((1000000000 / 38400) / ClockPeriod_Gen),
                                                        6 => ((1000000000 / 57600) / ClockPeriod_Gen),
                                                        7 => ((1000000000 / 115200) / ClockPeriod_Gen),
                                                        8 => ((1000000000 / 230400) / ClockPeriod_Gen),
                                                        9 => ((1000000000 / 460800) / ClockPeriod_Gen));

    -- UBX Messages
    constant Ubx_Sync_Con                           : std_logic_vector(15 downto 0) := x"62B5"; -- received in little endian

    constant UbxMon_Class_Con                       : std_logic_vector(7 downto 0) := x"0A";
    constant UbxNav_Class_Con                       : std_logic_vector(7 downto 0) := x"01";

    constant UbxMonHw_Id_Con                        : std_logic_vector(7 downto 0) := x"09";
    constant UbxMonHw_Length_Con                    : std_logic_vector(15 downto 0) := x"003C"; -- received in little endian
    constant UbxMonHw_OffsetAntStatus_Con           : std_logic_vector(15 downto 0) := x"0014";
    constant UbxMonHw_OffsetJamState_Con            : std_logic_vector(15 downto 0) := x"0016";
    constant UbxMonHw_OffsetJamInd_Con              : std_logic_vector(15 downto 0) := x"002D";

    constant UbxNavSat_Id_Con                       : std_logic_vector(7 downto 0) := x"35";
    constant UbxNavSat_OffsetSatNr_Con              : std_logic_vector(15 downto 0) := x"0005";
    constant UbxNavSat_OffsetLoopStart_Con          : std_logic_vector(15 downto 0) := x"0008"; -- loop starts at 8th Byte of the message
    constant UbxNavSat_OffsetLoopLength_Con         : std_logic_vector(3 downto 0) := x"C"; -- each loop is 12 Bytes
    constant UbxNavSat_OffsetQualInd_Con            : std_logic_vector(3 downto 0) := x"8"; -- offset of QualInd at each 12-byte loop 
    
    constant UbxNavStatus_Id_Con                    : std_logic_vector(7 downto 0) := x"03";
    constant UbxNavStatus_Length_Con                : std_logic_vector(15 downto 0) := x"0010"; -- received in little endian
    constant UbxNavStatus_OffsetGnssFix_Con         : std_logic_vector(15 downto 0) := x"0004";
    constant UbxNavStatus_OffsetGnssFixOk_Con       : std_logic_vector(15 downto 0) := x"0005";
    constant UbxNavStatus_OffsetSpoofDet_Con        : std_logic_vector(15 downto 0) := x"0007";

    constant UbxNavTimeLs_Id_Con                    : std_logic_vector(7 downto 0) := x"26";
    constant UbxNavTimeLs_Length_Con                : std_logic_vector(15 downto 0) := x"0018"; -- received in little endian
    constant UbxNavTimeLs_OffsetSrcCurrLs_Con       : std_logic_vector(15 downto 0) := x"0008";
    constant UbxNavTimeLs_OffsetCurrLs_Con          : std_logic_vector(15 downto 0) := x"0009";
    constant UbxNavTimeLs_SrcLsChange_Con           : std_logic_vector(15 downto 0) := x"000A";
    constant UbxNavTimeLs_OffsetLsChange_Con        : std_logic_vector(15 downto 0) := x"000B";
    constant UbxNavTimeLs_OffsetTimeToLs_Con        : std_logic_vector(15 downto 0) := x"000C";
    constant UbxNavTimeLs_OffsetValidLs_Con         : std_logic_vector(15 downto 0) := x"0017";

    constant UbxNavTimeUtc_Id_Con                   : std_logic_vector(7 downto 0) := x"21";
    constant UbxNavTimeUtc_Length_Con               : std_logic_vector(15 downto 0) := x"0014"; -- received in little endian
    constant UbxNavTimeUtc_OffsetYear_Con           : std_logic_vector(15 downto 0) := x"000C";
    constant UbxNavTimeUtc_OffsetMonth_Con          : std_logic_vector(15 downto 0) := x"000E";
    constant UbxNavTimeUtc_OffsetDay_Con            : std_logic_vector(15 downto 0) := x"000F";
    constant UbxNavTimeUtc_OffsetHour_Con           : std_logic_vector(15 downto 0) := x"0010";
    constant UbxNavTimeUtc_OffsetMinute_Con         : std_logic_vector(15 downto 0) := x"0011";
    constant UbxNavTimeUtc_OffsetSecond_Con         : std_logic_vector(15 downto 0) := x"0012";
    constant UbxNavTimeUtc_OffsetUtcValid_Con       : std_logic_vector(15 downto 0) := x"0013";
    
    constant MsgTimeoutMillisecond_Con              : natural := 3000; -- message reception timeout of 3s
    constant NanosInMillisecond_Con                 : natural := 1000000; 

    constant UtcOffsetInfo_Type_Reset               : UtcOffsetInfo_Type := (
                                                        Leap59                   => '0',
                                                        Leap61                   => '0',
                                                        LeapAnnouncement         => '0',
                                                        LeapChangeValid          => '0',
                                                        SrcOfCurLeapSecond       => (others => '0'),
                                                        TimeToLeapSecond         => (others => '0'),
                                                        TimeToLeapSecondValid    => '0', 
                                                        CurrentUtcOffset         => (others => '0'),
                                                        CurrentUtcOffsetValid    => '0',
                                                        CurrentTaiGnssOffset     => (others => '0'),
                                                        CurrentTaiGnssOffsetValid=> '0');    

    constant ToD_Type_Reset                         : ToD_Type := (
                                                        Year                     => x"7B2", --1970
                                                        Month                    => "0001", 
                                                        Day                      => "00001", 
                                                        Hour                     => (others => '0'), 
                                                        Minute                   => (others => '0'), 
                                                        Second                   => (others => '0'), 
                                                        Valid                    => '0');

    constant SatInfo_Type_Reset                     : SatInfo_Type := (
                                                        NumberOfSeenSats         => (others => '0'),
                                                        NumberOfLockedSats       => (others => '0'));


    constant AntennaInfo_Type_Reset                 : AntennaInfo_Type := (
                                                        Status                   => "001",
                                                        JamState                 => (others => '0'),
                                                        JamInd                   => (others => '0'));

    constant AntennaFix_Type_Reset                  : AntennaFix_Type := (
                                                        GnssFix                  => (others => '0'),
                                                        GnssFixOk                => '0',
                                                        SpoofDetState            => (others => '0'));

    -- TOD Slave version
    constant TodSlaveMajorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(0, 8));
    constant TodSlaveMinorVersion_Con               : std_logic_vector(7 downto 0) := std_logic_vector(to_unsigned(2, 8));
    constant TodSlaveBuildVersion_Con               : std_logic_vector(15 downto 0) := std_logic_vector(to_unsigned(1, 16));
    constant TodSlaveVersion_Con                    : std_logic_vector(31 downto 0) := TodSlaveMajorVersion_Con & TodSlaveMinorVersion_Con & TodSlaveBuildVersion_Con;

    -- TAI conversion 
    constant SecondsPerMinute_Con                   : natural := 60;
    constant SecondsPerHour_Con                     : natural := 60 * SecondsPerMinute_Con;
    constant SecondsPerDay_Con                      : natural := 24 * SecondsPerHour_Con;
    constant SecondsPerMonthArray_Con               : SecondsPerMonthArray_Type := (
                                                        1  => (31 * SecondsPerDay_Con),
                                                        2  => (28 * SecondsPerDay_Con),
                                                        3  => (31 * SecondsPerDay_Con),
                                                        4  => (30 * SecondsPerDay_Con),
                                                        5  => (31 * SecondsPerDay_Con),
                                                        6  => (30 * SecondsPerDay_Con),
                                                        7  => (31 * SecondsPerDay_Con),
                                                        8  => (31 * SecondsPerDay_Con),
                                                        9  => (30 * SecondsPerDay_Con),
                                                        10 => (31 * SecondsPerDay_Con),
                                                        11 => (30 * SecondsPerDay_Con),
                                                        12 => (31 * SecondsPerDay_Con));
    constant SecondsPerYear_Con                     : natural := 365 * SecondsPerDay_Con;
    
    -- TSIPv1 constants
    constant Tsip_Delimiter1_Con                    : std_logic_vector(7 downto 0) := x"10";
    constant Tsip_Delimiter2_Con                    : std_logic_vector(7 downto 0) := x"03";
    constant TsipTiming_ID_Con                      : std_logic_vector(15 downto 0) := x"A100";
    constant TsipPosition_ID_Con                    : std_logic_vector(15 downto 0) := x"A111";
    constant TsipSatellite_ID_Con                   : std_logic_vector(15 downto 0) := x"A200";
    constant TsipAlarms_ID_Con                      : std_logic_vector(15 downto 0) := x"A300";
    constant TsipReceiver_ID_Con                    : std_logic_vector(15 downto 0) := x"A311";
    constant TsipModeResponse_Con                   : std_logic_vector(7 downto 0) := x"02";

    -- Tsip message offsets
    constant TsipLengthOffset_Con                   : std_logic_vector(15 downto 0) := x"0003"; -- offset of payload counter to length counter
    constant TsipPosition_OffsetFixType_Con         : std_logic_vector(15 downto 0) := x"0007";
    constant TsipReceiver_OffsetMode_Con            : std_logic_vector(15 downto 0) := x"0006";
    constant TsipReceiver_OffsetStatus_Con          : std_logic_vector(15 downto 0) := x"0007";
    constant TsipTiming_OffsetHours_Con             : std_logic_vector(15 downto 0) := x"000C";
    constant TsipTiming_OffsetMinutes_Con           : std_logic_vector(15 downto 0) := x"000D";
    constant TsipTiming_OffsetSeconds_Con           : std_logic_vector(15 downto 0) := x"000E";
    constant TsipTiming_OffsetMonth_Con             : std_logic_vector(15 downto 0) := x"000F";
    constant TsipTiming_OffsetDay_Con               : std_logic_vector(15 downto 0) := x"0010";
    constant TsipTiming_OffsetYearHigh_Con          : std_logic_vector(15 downto 0) := x"0011";
    constant TsipTiming_OffsetYearLow_Con           : std_logic_vector(15 downto 0) := x"0012";
    constant TsipTiming_OffsetTimebase_Con          : std_logic_vector(15 downto 0) := x"0013";
    constant TsipTiming_OffsetFlags_Con             : std_logic_vector(15 downto 0) := x"0015";
    constant TsipTiming_OffsetUtcOffsetHigh_Con     : std_logic_vector(15 downto 0) := x"0016";
    constant TsipTiming_OffsetUtcOffsetLow_Con      : std_logic_vector(15 downto 0) := x"0017";
    constant TsipTiming_OffsetMinorAlarmsHigh_Con   : std_logic_vector(15 downto 0) := x"0008";
    constant TsipTiming_OffsetMinorAlarmsLow_Con    : std_logic_vector(15 downto 0) := x"0009";
    constant TsipTiming_OffsetMajorAlarmsHigh_Con   : std_logic_vector(15 downto 0) := x"0010";
    constant TsipTiming_OffsetMajorAlarmsLow_Con    : std_logic_vector(15 downto 0) := x"0011";
    constant TsipSatellite_OffsetId_Con             : std_logic_vector(15 downto 0) := x"0008";
    constant TsipSatellite_OffsetFlags_Con          : std_logic_vector(15 downto 0) := x"0018";
                                                             
    constant TsipReceiver_ModeODC_Con               : std_logic_vector(7 downto 0) := x"06"; -- overdetermined clock
    constant TsipReceiver_SatusGnssFix_Con          : std_logic_vector(7 downto 0) := x"FF"; -- Gnss time fix in overdetermined mode


    -- AXI registers                                                  Addr       , Mask       , RW  , Reset
    constant TodSlaveControl_Reg_Con                : Axi_Reg_Type:= (x"00000000", x"3F1FF801", Rw_E, x"00000000");
    constant TodSlaveStatus_Reg_Con                 : Axi_Reg_Type:= (x"00000004", x"00000007", Wc_E, x"00000000");
    constant TodSlaveUartPolarity_Reg_Con           : Axi_Reg_Type:= (x"00000008", x"00000001", Rw_E, x"00000000");
    constant TodSlaveVersion_Reg_Con                : Axi_Reg_Type:= (x"0000000C", x"FFFFFFFF", Ro_E, TodSlaveVersion_Con);
    constant TodSlaveCorrection_Reg_Con             : Axi_Reg_Type:= (x"00000010", x"FFFFFFFF", Ro_E, x"00000000"); -- unused!
    constant TodSlaveUartBaudRate_Reg_Con           : Axi_Reg_Type:= (x"00000020", x"0000000F", Rw_E, x"00000002"); -- UBX allowed baudrate: 2=>4800 3=>9600 4=>19200 5=>38400 6=>57600 7=>115200 8=>230400 9=>460800
    constant TodSlaveUtcStatus_Reg_Con              : Axi_Reg_Type:= (x"00000030", x"000171FF", Ro_E, x"00000000");
    constant TodSlaveTimeToLeapSecond_Reg_Con       : Axi_Reg_Type:= (x"00000034", x"FFFFFFFF", Ro_E, x"00000000");
    constant TodSlaveAntennaStatus_Reg_Con          : Axi_Reg_Type:= (x"00000040", x"37FF1FFF", Ro_E, x"00000000"); 
    constant TodSlaveSatNumber_Reg_Con              : Axi_Reg_Type:= (x"00000044", x"0001FFFF", Ro_E, x"00000000");

    constant TodSlaveControl_EnTsipBit_Con          : natural := 29;
    constant TodSlaveControl_EnUbxBit_Con           : natural := 28;
    constant TodSlaveControl_DisUbxNavSatBit_Con    : natural := 20;
    constant TodSlaveControl_DisUbxHwMonBit_Con     : natural := 19;
    constant TodSlaveControl_DisUbxNavStatusBit_Con : natural := 18;
    constant TodSlaveControl_DisUbxNavTimeUtcBit_Con: natural := 17;
    constant TodSlaveControl_DisUbxNavTimeLsBit_Con : natural := 16;
    constant TodSlaveControl_DisTsipSatelliteBit_Con: natural := 20;
    constant TodSlaveControl_DisTsipPositionBit_Con : natural := 19;
    constant TodSlaveControl_DisTsipReceiverBit_Con : natural := 18;
    constant TodSlaveControl_DisTsipTimingBit_Con   : natural := 17;
    constant TodSlaveControl_DisTsipAlarmsBit_Con   : natural := 16;
    constant TodSlaveControl_EnableBit_Con          : natural := 0;

    constant TodSlaveStatus_ParserErrorBit_Con      : natural := 0;
    constant TodSlaveStatus_ChecksumErrorBit_Con    : natural := 1;
    constant TodSlaveStatus_UartErrorBit_Con        : natural := 2;
    
    constant TodSlaveUartPolarity_PolarityBit_Con   : natural := 0;
    
    constant TodSlaveUtcStatus_UtcOffsetValidBit_Con: natural := 8;
    constant TodSlaveUtcStatus_LeapAnnounceBit_Con  : natural := 12;
    constant TodSlaveUtcStatus_Leap59Bit_Con        : natural := 13;
    constant TodSlaveUtcStatus_Leap61Bit_Con        : natural := 14;
    constant TodSlaveUtcStatus_LeapInfoValidBit_Con : natural := 16;
    
    --*************************************************************************************
    -- Signal Definitions
    --*************************************************************************************

    -- Rx Uart converted to Msg byte with valid flag
    signal ClksPerUartBit_Dat                       : natural range 0 to ((1000000000/ClockPeriod_Gen)-1) := ClksPerUartBit_Array_Con(UartDefaultBaudRate_Gen);
    signal ClksPerUartBitCounter_CntReg             : natural range 0 to ((1000000000/ClockPeriod_Gen)-1) := 0;
    signal BitsPerMsgDataCounter_CntReg             : natural range 0 to 7 := 0;
    signal MsgData_DatReg                           : std_logic_vector(7 downto 0);
    signal MsgDataValid_ValReg                      : std_logic;
    signal RxUart_ShiftReg                          : std_logic_vector(1 downto 0);
    signal RxUart_DatReg                            : std_logic;
    signal RxUart_DatOldReg                         : std_logic;
    signal UartRxState_StaReg                       : UartRxState_Type;
    signal UartPolarity_Dat                         : boolean := UartPolarity_Gen;
    signal UartError_DatReg                         : std_logic;

    -- ToD control
    signal Enable_Ena                               : std_logic;
    signal Enable_Ubx_Ena                           : std_logic;
    signal Disable_UbxNavSat_Ena                    : std_logic;
    signal Disable_UbxHwMon_Ena                     : std_logic;
    signal Disable_UbxNavStatus_Ena                 : std_logic;
    signal Disable_UbxNavTimeUtc_Ena                : std_logic;
    signal Disable_UbxNavTimeLs_Ena                 : std_logic;
    
    -- Decode msg 
    signal DetectUbxMsgState                        : DetectUbxMsgState_Type := Idle_St;
    signal UbxPayloadCount_CntReg                   : std_logic_Vector(15 downto 0) := x"0000";
    signal UbxParseError_DatReg                     : std_logic;
    signal UbxChecksumError_DatReg                  : std_logic;
    signal UbxCheckLengthFlag_DatReg                : std_logic;
    signal UbxChecksumA_DatReg                      : unsigned(7 downto 0):= (others => '0');
    signal UbxChecksumB_DatReg                      : unsigned(7 downto 0):= (others => '0');

    signal CheckMsg_UbxMonHw_DatReg                 : std_logic := '0';
    signal CheckMsg_UbxNavSat_DatReg                : std_logic := '0';
    signal CheckMsg_UbxNavStatus_DatReg             : std_logic := '0';
    signal CheckMsg_UbxNavTimeLs_DatReg             : std_logic := '0';
    signal CheckMsg_UbxNavTimeUtc_DatReg            : std_logic := '0';

    signal AntennaInfo_DatReg                       : AntennaInfo_Type := AntennaInfo_Type_Reset; 
    signal AntennaInfoValid_ValReg                  : std_logic := '0';
    signal AntennaInfoValid_ValOldReg               : std_logic := '0';

    signal AntennaFix_DatReg                        : AntennaFix_Type := AntennaFix_Type_Reset;
    signal AntennaFixValid_ValReg                   : std_logic := '0';
    signal AntennaFixValid_ValOldReg                : std_logic := '0';

    signal SatInfo_DatReg                           : SatInfo_Type := SatInfo_Type_Reset;
    signal SatInfoValid_ValReg                      : std_logic := '0';
    signal SatInfoValid_ValOldReg                   : std_logic := '0';
    signal UbxNavSat_Length_DatReg                  : std_logic_vector(15 downto 0) := (others => '0');
    signal UbxNavSat_SatCounter_DatReg              : std_logic_vector(7 downto 0) := (others => '0'); -- UBX-NAV-SAT has a dynamic length depending on the number of the seen Sats
    signal UbxNavSat_InLoopCounter_DatReg           : std_logic_vector(3 downto 0) := (others => '0'); -- UBX-NAV-SAT has a dynamic length , for each seen Sat there is info of 12 bytes 
    signal UbxNavSat_Loop_DatReg                    : std_logic := '0';

    signal UtcOffsetInfo_DatReg                     : UtcOffsetInfo_Type := UtcOffsetInfo_Type_Reset;
    signal UtcOffsetInfoValid_ValReg                : std_logic;
    signal UtcOffsetInfoValid_ValOldReg             : std_logic;

    signal RxToD_DatReg                             : ToD_Type := ToD_Type_Reset;
    signal RxToDValid_ValReg                        : std_logic;
    signal RxToDValid_ValOldReg                     : std_logic;

    -- Message timeout
    signal UbxNavSat_TimeoutCounter_CntReg          : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal UbxHwMon_TimeoutCounter_CntReg           : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal UbxNavStatus_TimeoutCounter_CntReg       : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal UbxNavTimeUtc_TimeoutCounter_CntReg      : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal UbxNavTimeLs_TimeoutCounter_CntReg       : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal MillisecondCounter_CntReg                : natural range 0 to (1000000 + ClockPeriod_Gen) := 0;
    signal MillisecondFlag_EvtReg                   : std_logic := '0';
    signal UbxNavSat_Timeout_EvtReg                 : std_logic := '0';
    signal UbxHwMon_Timeout_EvtReg                  : std_logic := '0';
    signal UbxNavStatus_Timeout_EvtReg              : std_logic := '0';
    signal UbxNavTimeUtc_Timeout_EvtReg             : std_logic := '0';
    signal UbxNavTimeLs_Timeout_EvtReg              : std_logic := '0';

    signal ClockTime_Nanosecond_DatReg              : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    signal ClockTime_Nanosecond_OldDatReg           : std_logic_vector((NanosecondWidth_Con-1) downto 0);
    
    -- TAI conversion
    signal ClockTime_Second_DatReg                  : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal TimeAdjustment_Second_DatReg             : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal TimeAdjustment_Nanosecond_DatReg         : std_logic_vector((NanosecondWidth_Con-1) downto 0) := (others => '0');
    signal TimeAdjustment_ValReg                    : std_logic;
    signal GnssTime_Second_DatReg                   : std_logic_vector((SecondWidth_Con-1) downto 0) := (others => '0');
    signal TaiConversionState_StaReg                : TaiConversionState_Type := Idle_St;
    signal LeapYear_DatReg                          : std_logic;
    signal SkipTaiConversion_ValReg                 : std_logic;
    signal TimeCounter_CntReg                       : natural := 0;
    signal Year_004_Counter_CntReg                  : natural range 0 to 5 := 0; -- leap year every 4 years 
    signal Year_100_Counter_CntReg                  : natural range 0 to 101 := 0; -- no leap year every 100 years
    signal Year_400_Counter_CntReg                  : natural range 0 to 401 := 0; -- leap year every 400 years
    signal ToD_DatReg                               : ToD_Type := ToD_Type_Reset;
    signal ToDValid_ValReg                          : std_logic;
    
    -- Trimble Standatd Interface Protocol v1 (TSIPv1) related signals
    signal Enable_Tsip_Ena                          : std_logic;
    signal DetectTsipMsgState                       : DetectTsipMsgState_Type;
    signal MsgDataOld_DatReg                        : std_logic_vector(7 downto 0);
    signal MsgDataOldValid_ValReg                   : std_logic;
    signal TsipMsgData_DatReg                       : std_logic_vector(7 downto 0);
    signal TsipMsgDataValid_ValReg                  : std_logic;
    signal TsipPaddingSkipped_DatReg                : std_logic;
    signal TsipChecksum_DatReg                      : std_logic_vector(7 downto 0);

    signal TsipEoF_DatReg                           : std_logic := '0';
    signal TsipMsg_A1_DatReg                        : std_logic := '0';
    signal TsipMsg_A2_DatReg                        : std_logic := '0';
    signal TsipMsg_A3_DatReg                        : std_logic := '0';
    signal TsipTiming_DatReg                        : std_logic := '0';
    signal TsipPosition_DatReg                      : std_logic := '0';
    signal TsipSatellite_DatReg                     : std_logic := '0';
    signal TsipAlarms_DatReg                        : std_logic := '0';
    signal TsipReceiver_DatReg                      : std_logic := '0';
    signal TsipLength_DatReg                        : std_logic_vector(15 downto 0) := (others=>'0');
    signal TsipPayloadCount_CntReg                  : std_logic_vector(15 downto 0) := (others=>'0');
    signal TsipParseError_DatReg                    : std_logic := '0';
    signal TsipChecksumError_DatReg                 : std_logic := '0';
                                                 
    signal TsipPosition_MsgVal_ValReg               : std_logic := '0';
    signal TsipPosition_MsgValOld_ValReg            : std_logic := '0';
    signal TsipReceiver_MsgVal_ValReg               : std_logic := '0';
    signal TsipReceiver_MsgValOld_ValReg            : std_logic := '0';
    signal TsipTiming_MsgVal_ValReg                 : std_logic := '0';
    signal TsipTiming_MsgValOld_ValReg              : std_logic := '0';
    signal TsipAlarms_MsgVal_ValReg                 : std_logic := '0';
    signal TsipAlarms_MsgValOld_ValReg              : std_logic := '0';
    signal TsipSatellite_MsgVal_ValReg              : std_logic := '0';
    signal TsipSatellite_MsgValOld_ValReg           : std_logic := '0';
                                                 
    signal TsipReceiver_ODC_DatReg                  : std_logic := '0';
    signal TsipAlarms_NoSatellites_DatReg           : std_logic := '0';
    signal TsipAlarms_ClearSatellites_DatReg        : std_logic := '0';
    signal TsipAlarms_NoPps_DatReg                  : std_logic := '0';
    signal TsipAlarms_BadPps_DatReg                 : std_logic := '0';
    signal TsipSatellite_CntSatellites_DatReg       : SatInfo_Type := SatInfo_Type_Reset; -- temp store of the satellite counters until a same satellite is seen again (a 'round' has completed)
    signal TsipSatellite_IncreaseSeenSat_DatReg     : std_logic := '0'; -- temp mark a satellite as seen, until the message is validated 
    signal TsipSatellite_IncreaseLockSat_DatReg     : std_logic := '0'; -- temp mark a satellite as locked, until the message is validated
    
    signal Disable_TsipTiming_Ena                   : std_logic := '0';
    signal Disable_TsipPosition_Ena                 : std_logic := '0';
    signal Disable_TsipReceiver_Ena                 : std_logic := '0';
    signal Disable_TsipAlarms_Ena                   : std_logic := '0';
    signal Disable_TsipSatellite_Ena                : std_logic := '0';

    signal TsipTiming_TimeoutCounter_CntReg         : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal TsipAlarms_TimeoutCounter_CntReg         : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal TsipReceiver_TimeoutCounter_CntReg       : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal TsipPosition_TimeoutCounter_CntReg       : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal TsipSatellite_TimeoutCounter_CntReg      : natural range 0 to (MsgTimeoutMillisecond_Con + 1) := 0;
    signal TsipTiming_Timeout_EvtReg                : std_logic := '0';
    signal TsipAlarms_Timeout_EvtReg                : std_logic := '0';
    signal TsipReceiver_Timeout_EvtReg              : std_logic := '0';
    signal TsipPosition_Timeout_EvtReg              : std_logic := '0';
    signal TsipSatellite_Timeout_EvtReg             : std_logic := '0';

    
    -- Axi Regs
    signal Axi_AccessState_StaReg                   : Axi_AccessState_Type:= Axi_AccessState_Type_Rst_Con;

    signal AxiWriteAddrReady_RdyReg                 : std_logic;       
    signal AxiWriteDataReady_RdyReg                 : std_logic;
    signal AxiWriteRespValid_ValReg                 : std_logic;
    signal AxiWriteRespResponse_DatReg              : std_logic_vector(1 downto 0);   
    signal AxiReadAddrReady_RdyReg                  : std_logic;
    signal AxiReadDataValid_ValReg                  : std_logic;
    signal AxiReadDataResponse_DatReg               : std_logic_vector(1 downto 0);
    signal AxiReadDataData_DatReg                   : std_logic_vector(31 downto 0);

    signal TodSlaveControl_DatReg                   : std_logic_Vector(31 downto 0);
    signal TodSlaveStatus_DatReg                    : std_logic_Vector(31 downto 0);
    signal TodSlaveUartPolarity_DatReg              : std_logic_Vector(31 downto 0);
    signal TodSlaveVersion_DatReg                   : std_logic_Vector(31 downto 0);
    signal TodSlaveCorrection_DatReg                : std_logic_Vector(31 downto 0);
    signal TodSlaveUartBaudRate_DatReg              : std_logic_Vector(31 downto 0);
    signal TodSlaveUtcStatus_DatReg                 : std_logic_Vector(31 downto 0);
    signal TodSlaveTimeToLeapSecond_DatReg          : std_logic_Vector(31 downto 0);
    signal TodSlaveAntennaStatus_DatReg             : std_logic_Vector(31 downto 0);
    signal TodSlaveSatNumber_DatReg                 : std_logic_Vector(31 downto 0);

--*****************************************************************************************
-- Architecture Implementation
--*****************************************************************************************
begin

    --*************************************************************************************
    -- Concurrent Statements
    --*************************************************************************************
    Enable_Ena                                      <= TodSlaveControl_DatReg(TodSlaveControl_EnableBit_Con);
    Enable_Ubx_Ena                                  <= TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con);
    Enable_Tsip_Ena                                 <= ((TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)) and (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con))); -- Ensure UBX is disabled , if TSIPv1 is enabled. Else, select UBX 
    Disable_UbxNavSat_Ena                           <= ((TodSlaveControl_DatReg(TodSlaveControl_DisUbxNavSatBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con)));
    Disable_UbxHwMon_Ena                            <= ((TodSlaveControl_DatReg(TodSlaveControl_DisUbxHwMonBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con)));
    Disable_UbxNavStatus_Ena                        <= ((TodSlaveControl_DatReg(TodSlaveControl_DisUbxNavStatusBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con)));
    Disable_UbxNavTimeUtc_Ena                       <= ((TodSlaveControl_DatReg(TodSlaveControl_DisUbxNavTimeUtcBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con)));
    Disable_UbxNavTimeLs_Ena                        <= ((TodSlaveControl_DatReg(TodSlaveControl_DisUbxNavTimeLsBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnUbxBit_Con)));
    Disable_TsipTiming_Ena                          <= ((TodSlaveControl_DatReg(TodSlaveControl_DisTsipTimingBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)));
    Disable_TsipPosition_Ena                        <= ((TodSlaveControl_DatReg(TodSlaveControl_DisTsipPositionBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)));
    Disable_TsipReceiver_Ena                        <= ((TodSlaveControl_DatReg(TodSlaveControl_DisTsipReceiverBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)));
    Disable_TsipAlarms_Ena                          <= ((TodSlaveControl_DatReg(TodSlaveControl_DisTsipAlarmsBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)));
    Disable_TsipSatellite_Ena                       <= ((TodSlaveControl_DatReg(TodSlaveControl_DisTsipSatelliteBit_Con)) or (not TodSlaveControl_DatReg(TodSlaveControl_EnTsipBit_Con)));

    UartPolarity_Dat                                <= true when (TodSlaveUartPolarity_DatReg(TodSlaveUartPolarity_PolarityBit_Con) = '1') else
                                                       false;

    ClksPerUartBit_Dat                              <= ClksPerUartBit_Array_Con(2) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(2,4))) else -- 4800 
                                                       ClksPerUartBit_Array_Con(3) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(3,4))) else -- 9600 
                                                       ClksPerUartBit_Array_Con(4) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(4,4))) else -- 19200 
                                                       ClksPerUartBit_Array_Con(5) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(5,4))) else -- 38400 
                                                       ClksPerUartBit_Array_Con(6) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(6,4))) else -- 57600 
                                                       ClksPerUartBit_Array_Con(7) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(7,4))) else -- 115200
                                                       ClksPerUartBit_Array_Con(8) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(8,4))) else -- 230400
                                                       ClksPerUartBit_Array_Con(9) when (TodSlaveUartBaudRate_DatReg(3 downto 0) = std_logic_vector(to_unsigned(9,4))) else -- 460800
                                                       ClksPerUartBit_Array_Con(UartDefaultBaudRate_Gen); -- Default BaudRate 

    AxiWriteAddrReady_RdyOut                        <= AxiWriteAddrReady_RdyReg;
    AxiWriteDataReady_RdyOut                        <= AxiWriteDataReady_RdyReg;
    AxiWriteRespValid_ValOut                        <= AxiWriteRespValid_ValReg;
    AxiWriteRespResponse_DatOut                     <= AxiWriteRespResponse_DatReg;
    AxiReadAddrReady_RdyOut                         <= AxiReadAddrReady_RdyReg;
    AxiReadDataValid_ValOut                         <= AxiReadDataValid_ValReg;
    AxiReadDataResponse_DatOut                      <= AxiReadDataResponse_DatReg;
    AxiReadDataData_DatOut                          <= AxiReadDataData_DatReg;

    TimeAdjustment_Second_DatOut                    <= TimeAdjustment_Second_DatReg;
    TimeAdjustment_Nanosecond_DatOut                <= TimeAdjustment_Nanosecond_DatReg;
    TimeAdjustment_ValOut                           <= TimeAdjustment_ValReg;
    
    --*************************************************************************************
    -- Procedural Statements
    --*************************************************************************************

    -- metastability registers
    RxUartMetastability_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            RxUart_ShiftReg <= (others => '0');
            RxUart_DatReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then 
            RxUart_ShiftReg(1) <= RxUart_ShiftReg(0);
            RxUart_ShiftReg(0) <= RxUart_DatIn;
            if (UartPolarity_Dat = true) then -- use the input value or its inversion depending on the polarity configuration 
                RxUart_DatReg <= RxUart_ShiftReg(1);
            else
                RxUart_DatReg <= not RxUart_ShiftReg(1);
            end if;
        end if;
    end process RxUartMetastability_Prc;

    -- RxUart FSM. The UART message sequence is Start(1bit)=>Data(8bits)=>Stop(1bit) (no parity)
    RxUartFsm_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then   
            UartRxState_StaReg <= Idle_St;
            MsgDataValid_ValReg <= '0';
            MsgData_DatReg <= (others => '0');
            RxUart_DatOldReg <= '0';
            ClksPerUartBitCounter_CntReg <= 0;
            BitsPerMsgDataCounter_CntReg <= 0;
            UartError_DatReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then 
            RxUart_DatOldReg <= RxUart_DatReg;
            MsgDataValid_ValReg <= '0';
            UartError_DatReg <= '0';

            case (UartRxState_StaReg) is
                when Idle_St =>
                    MsgData_DatReg <= (others => '0');
                    ClksPerUartBitCounter_CntReg <= 0;
                    BitsPerMsgDataCounter_CntReg <= 0;
                    if ((RxUart_DatReg = '0') and (RxUart_DatOldReg = '1')) then -- it could be a Start bit
                        ClksPerUartBitCounter_CntReg <= (to_integer((to_unsigned(ClksPerUartBit_Dat, 32)) srl 1) -1); -- count down to the middle of the bit-reception cycle
                        UartRxState_StaReg <= Start_St;
                    end if;

                when Start_St =>
                    if (ClksPerUartBitCounter_CntReg > 0) then 
                        ClksPerUartBitCounter_CntReg <= ClksPerUartBitCounter_CntReg - 1;
                    else 
                        if (RxUart_DatReg = '0') then -- verify that the Start bit is as expected
                            ClksPerUartBitCounter_CntReg <= ClksPerUartBit_Dat - 1; -- count down to the middle of the next bit-reception cycle
                            UartRxState_StaReg <= Data_St;
                        else
                            UartError_DatReg <= '1'; -- raise an error when the byte-read was incomplete
                            UartRxState_StaReg <= Idle_St;
                        end if;
                    end if;

                when Data_St =>
                    if (ClksPerUartBitCounter_CntReg > 0) then 
                        ClksPerUartBitCounter_CntReg <= ClksPerUartBitCounter_CntReg - 1;
                    else 
                        MsgData_DatReg(BitsPerMsgDataCounter_CntReg) <= RxUart_DatReg; -- assign the uart bit to the byte data
                        ClksPerUartBitCounter_CntReg <= ClksPerUartBit_Dat - 1; -- count down to the middle of the next bit-reception cycle
                        if (BitsPerMsgDataCounter_CntReg < 7) then -- if not all of the data have been read, stay here
                            BitsPerMsgDataCounter_CntReg <= BitsPerMsgDataCounter_CntReg + 1; -- prepare for the next bit of the byte
                        else -- the whole byte has been read
                            UartRxState_StaReg <= Stop_St;
                        end if;
                    end if;

                when Stop_St =>
                    if (ClksPerUartBitCounter_CntReg > 0) then 
                        ClksPerUartBitCounter_CntReg <= ClksPerUartBitCounter_CntReg - 1;
                    else 
                        UartRxState_StaReg <= Idle_St;
                        if (RxUart_DatReg = '1') then -- verify that the Stop bit is as expected
                            MsgDataValid_ValReg <= '1'; -- active for 1 clk
                        else
                            UartError_DatReg <= '1'; -- raise an error when the byte-read was incomplete
                        end if;
                    end if;
            end case;
        end if;
    end process RxUartFsm_Prc;

    -- Detect and decode UBX messages. Extract the following information:  
    --   - from MON-HW      => Antenna status and Jamming status
    --   - from NAV-SAT     => Sats Number and Locked Sats
    --   - from NAV-STATUS  => GPS fix
    --   - from NAV-TIMELS  => the current LS, the TimeToLsEvent, if there is LS change coming and to which direction
    --   - from NAV-TIMEUTC => ToD in format YYYYMMDDHHmmSS
    -- Detect and decode TSIP messages. Extract the following information:  
    --   - from Timing Info         => ToD in format YYYYMMDDHHmmSS and UTC offset
    --   - from Satellite Info      => Sats Number and Locked Sats (one message per satellite, each sat can be acquired, or acquired and used in time lock)
    --   - from Alarms Info         => pending Leap Second, Jam Indication, Spoof Indication, Antenna Status
    --   - from Position Info       => Gnss fix of the antenna
    --   - from Receiver Info       => Gnss fix OK of the antenna
    DecodeMsgFsm_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            UbxPayloadCount_CntReg <= x"0000";
            UbxNavSat_Length_DatReg <= x"0000";
            
            AntennaInfo_DatReg <= AntennaInfo_Type_Reset;
            AntennaInfoValid_ValReg <= '0';

            AntennaFix_DatReg <= AntennaFix_Type_Reset;
            AntennaFixValid_ValReg <= '0';

            UtcOffsetInfo_DatReg <= UtcOffsetInfo_Type_Reset;
            UtcOffsetInfoValid_ValReg <= '0';
            
            RxToD_DatReg <= ToD_Type_Reset;
            RxToDValid_ValReg <= '0';
            
            SatInfo_DatReg <= SatInfo_Type_Reset;
            SatInfoValid_ValReg <= '0';
            UbxNavSat_SatCounter_DatReg <= (others => '0');
            UbxNavSat_InLoopCounter_DatReg <= (others => '0');
            UbxNavSat_Loop_DatReg <= '0';
                
            DetectUbxMsgState <= Idle_St;
            UbxParseError_DatReg <= '0';
            UbxChecksumError_DatReg <= '0';
            UbxCheckLengthFlag_DatReg <= '0';
            UbxChecksumA_DatReg <= (others => '0');
            UbxChecksumB_DatReg <= (others => '0');
            
            CheckMsg_UbxMonHw_DatReg <= '0';    
            CheckMsg_UbxNavSat_DatReg <= '0';    
            CheckMsg_UbxNavStatus_DatReg <= '0';
            CheckMsg_UbxNavTimeLs_DatReg <= '0';
            CheckMsg_UbxNavTimeUtc_DatReg <= '0';
            
            -- TSIP decoding signals
            MsgDataOld_DatReg <= (others=>'0');
            MsgDataOldValid_ValReg <= '0';
            TsipMsgData_DatReg <= (others=>'0');
            TsipMsgDataValid_ValReg <= '0';
            TsipEoF_DatReg <= '0';
            TsipPaddingSkipped_DatReg <= '0';
            TsipChecksum_DatReg <= (others=>'0');
            
            TsipPosition_MsgVal_ValReg <= '0';
            TsipPosition_MsgValOld_ValReg <= '0';
            TsipReceiver_MsgVal_ValReg <= '0';
            TsipReceiver_MsgValOld_ValReg <= '0';
            TsipTiming_MsgVal_ValReg <= '0';
            TsipTiming_MsgValOld_ValReg <= '0';
            TsipAlarms_MsgVal_ValReg <= '0';
            TsipAlarms_MsgValOld_ValReg <= '0';
            TsipSatellite_MsgVal_ValReg <= '0';
            TsipSatellite_MsgValOld_ValReg <= '0';
            
            TsipReceiver_ODC_DatReg <= '0';
            TsipAlarms_NoSatellites_DatReg <= '0';
            TsipAlarms_ClearSatellites_DatReg <= '0';
            TsipAlarms_NoPps_DatReg <= '0';
            TsipAlarms_BadPps_DatReg <= '0';
            TsipSatellite_CntSatellites_DatReg <= SatInfo_Type_Reset; 
            TsipSatellite_IncreaseSeenSat_DatReg <= '0';            
            TsipSatellite_IncreaseLockSat_DatReg <= '0';            
            
            TsipParseError_DatReg <= '0';
            TsipChecksumError_DatReg <= '0';
            
            ClockTime_Nanosecond_DatReg <= (others=>'0');
            ClockTime_Nanosecond_OldDatReg <= (others=>'0');
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            UbxParseError_DatReg <= '0';
            UbxChecksumError_DatReg <= '0';
            ClockTime_Nanosecond_DatReg <= ClockTime_Nanosecond_DatIn;
            ClockTime_Nanosecond_OldDatReg <= ClockTime_Nanosecond_DatReg;

            -- Decoding of the UBX messages
            -- The data valid flag is active for 1 clk per byte data 
            if (MsgDataValid_ValReg = '1') then 
                case (DetectUbxMsgState) is
                    when Idle_St => 
                        CheckMsg_UbxMonHw_DatReg <= '0';    
                        CheckMsg_UbxNavSat_DatReg <= '0';    
                        CheckMsg_UbxNavStatus_DatReg <= '0';
                        CheckMsg_UbxNavTimeLs_DatReg <= '0';
                        CheckMsg_UbxNavTimeUtc_DatReg <= '0';

                        UbxPayloadCount_CntReg <= x"0000";
                        UbxCheckLengthFlag_DatReg <= '0';
                        UbxChecksumA_DatReg <= (others => '0');
                        UbxChecksumB_DatReg <= (others => '0');
                        
                        UbxNavSat_SatCounter_DatReg <= (others => '0');
                        UbxNavSat_InLoopCounter_DatReg <= (others => '0');
                        UbxNavSat_Loop_DatReg <= '0';
                        
                        -- Start decoding only if the UBX messages are enabled
                        if (Enable_Ubx_Ena='1') then 
                            if (MsgData_DatReg = Ubx_Sync_Con(7 downto 0)) then 
                                DetectUbxMsgState <= UbxHeader_Sync_St;
                            end if;
                        end if;

                    when UbxHeader_Sync_St => 
                        if (MsgData_DatReg = Ubx_Sync_Con(15 downto 8)) then 
                            DetectUbxMsgState <= UbxHeader_Class_St;
                        else
                            DetectUbxMsgState <= Idle_St;
                            UbxParseError_DatReg <= '1'; -- invalid frame
                        end if;

                    when UbxHeader_Class_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if (MsgData_DatReg = UbxMon_Class_Con) then 
                            DetectUbxMsgState <= UbxHeader_MonId_St;
                        elsif (MsgData_DatReg = UbxNav_Class_Con) then 
                            DetectUbxMsgState <= UbxHeader_NavId_St;
                        else
                            DetectUbxMsgState <= Idle_St; -- not supported message 
                        end if;

                    when UbxHeader_MonId_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if ((CheckMsg_UbxMonHw_DatReg = '0') and (MsgData_DatReg = UbxMonHw_Id_Con)) then 
                            CheckMsg_UbxMonHw_DatReg <= '1';
                        elsif (CheckMsg_UbxMonHw_DatReg = '1') then 
                            if ((UbxCheckLengthFlag_DatReg = '0') and (MsgData_DatReg = UbxMonHw_Length_Con(7 downto 0))) then 
                                UbxCheckLengthFlag_DatReg <= '1';
                            elsif ((UbxCheckLengthFlag_DatReg = '1') and (MsgData_DatReg = UbxMonHw_Length_Con(15 downto 8))) then 
                                DetectUbxMsgState <= UbxMonHw_CheckPayload_St;
                                AntennaInfoValid_ValReg <= '0'; -- invalidate the data when new updated data will be received until the checksum is veirifed
                            else
                                UbxParseError_DatReg <= '1'; -- the ID has been detected but the length is wrong
                                DetectUbxMsgState <= Idle_St; 
                            end if;
                        else
                            DetectUbxMsgState <= Idle_St; -- not supported message
                        end if;

                    when UbxHeader_NavId_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if ((CheckMsg_UbxNavSat_DatReg = '0') and (CheckMsg_UbxNavStatus_DatReg = '0') and CheckMsg_UbxNavTimeLs_DatReg = '0' and CheckMsg_UbxNavTimeUtc_DatReg = '0') then 
                            if (MsgData_DatReg = UbxNavSat_Id_Con) then 
                                CheckMsg_UbxNavSat_DatReg <= '1';
                            elsif (MsgData_DatReg = UbxNavStatus_Id_Con) then
                                CheckMsg_UbxNavStatus_DatReg <= '1';
                            elsif (MsgData_DatReg = UbxNavTimeLs_Id_Con) then
                                CheckMsg_UbxNavTimeLs_DatReg <= '1';
                            elsif (MsgData_DatReg = UbxNavTimeUtc_Id_Con) then
                                CheckMsg_UbxNavTimeUtc_DatReg <= '1';
                            else
                                DetectUbxMsgState <= Idle_St; -- not supported message
                            end if;
                        elsif (CheckMsg_UbxNavSat_DatReg = '1') then
                            if (UbxCheckLengthFlag_DatReg = '0') then 
                                UbxNavSat_Length_DatReg(7 downto 0) <= MsgData_DatReg;
                                UbxCheckLengthFlag_DatReg <= '1';
                            else
                                DetectUbxMsgState <= UbxNavSat_CheckPayload_St;
                                UbxNavSat_Length_DatReg(15 downto 8) <= MsgData_DatReg;
                                SatInfoValid_ValReg <= '0'; -- invalidate the data when new updated data will be received until the checksum is verified
                            end if;
                        elsif (CheckMsg_UbxNavStatus_DatReg = '1') then
                            if ((UbxCheckLengthFlag_DatReg = '0') and (MsgData_DatReg = UbxNavStatus_Length_Con(7 downto 0))) then 
                                UbxCheckLengthFlag_DatReg <= '1';
                            elsif ((UbxCheckLengthFlag_DatReg = '1') and (MsgData_DatReg = UbxNavStatus_Length_Con(15 downto 8))) then 
                                DetectUbxMsgState <= UbxNavStatus_CheckPayload_St;
                                AntennaFixValid_ValReg <= '0'; -- invalidate the data when new updated data will be received until the checksum is verified
                            else
                                UbxParseError_DatReg <= '1'; -- the ID has been detected but the length is wrong
                                DetectUbxMsgState <= Idle_St; 
                            end if;
                        elsif (CheckMsg_UbxNavTimeLs_DatReg = '1') then 
                            if ((UbxCheckLengthFlag_DatReg = '0') and (MsgData_DatReg = UbxNavTimeLs_Length_Con(7 downto 0))) then 
                                UbxCheckLengthFlag_DatReg <= '1';
                            elsif ((UbxCheckLengthFlag_DatReg = '1') and (MsgData_DatReg = UbxNavTimeLs_Length_Con(15 downto 8))) then 
                                DetectUbxMsgState <= UbxNavTimeLs_CheckPayload_St;
                                UtcOffsetInfoValid_ValReg <= '0'; -- invalidate the data when new updated data will be received until the checksum is verified
                            else
                                UbxParseError_DatReg <= '1'; -- the ID has been detected but the length is wrong
                                DetectUbxMsgState <= Idle_St; 
                            end if;
                        elsif (CheckMsg_UbxNavTimeUtc_DatReg = '1') then 
                            if ((UbxCheckLengthFlag_DatReg = '0') and (MsgData_DatReg = UbxNavTimeUtc_Length_Con(7 downto 0))) then 
                                UbxCheckLengthFlag_DatReg <= '1';
                            elsif ((UbxCheckLengthFlag_DatReg = '1') and (MsgData_DatReg = UbxNavTimeUtc_Length_Con(15 downto 8))) then 
                                DetectUbxMsgState <= UbxNavTimeUtc_CheckPayload_St;
                                RxToDValid_ValReg <= '0'; -- invalidate the data when new updated data will be received until the checksum is verified
                            else
                                UbxParseError_DatReg <= '1'; -- the ID has been detected but the length is wrong
                                DetectUbxMsgState <= Idle_St; 
                            end if;
                        else    
                            DetectUbxMsgState <= Idle_St; -- not supported message
                            UbxParseError_DatReg <= '1'; -- the ID has been detected but the length is wrong
                        end if;

                    -- Extract info from Payload of UBX-MON-HW
                    when UbxMonHw_CheckPayload_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if (UbxPayloadCount_CntReg = UbxMonHw_OffsetAntStatus_Con) then 
                            AntennaInfo_DatReg.Status  <= MsgData_DatReg(2 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxMonHw_OffsetJamState_Con) then
                            AntennaInfo_DatReg.JamState <= MsgData_DatReg(3 downto 2);
                        elsif (UbxPayloadCount_CntReg = UbxMonHw_OffsetJamInd_Con) then
                            AntennaInfo_DatReg.JamInd <= MsgData_DatReg;
                        end if;
                        if (unsigned(UbxPayloadCount_CntReg) < (unsigned(UbxMonHw_Length_Con) - 1)) then 
                            UbxPayloadCount_CntReg <= std_logic_Vector(unsigned(UbxPayloadCount_CntReg) + 1);
                        else
                            DetectUbxMsgState <= UbxChecksum_ChecksumA_St;
                        end if;

                    -- Extract info from Payload of UBX-NAV-Sat
                    when UbxNavSat_CheckPayload_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        UbxPayloadCount_CntReg <= std_logic_Vector(unsigned(UbxPayloadCount_CntReg) + 1);
                        if (UbxPayloadCount_CntReg = UbxNavSat_OffsetSatNr_Con) then 
                            SatInfo_DatReg.NumberOfSeenSats <= MsgData_DatReg;
                            SatInfo_DatReg.NumberOfLockedSats <= (others => '0');
                        elsif (UbxPayloadCount_CntReg = std_logic_vector(unsigned(UbxNavSat_OffsetLoopStart_Con)-1)) then 
                            UbxNavSat_Loop_DatReg <= '1'; -- at the next valid byte, the loop starts
                        end if;
                        -- 12-bytes-loop for the number of Sats
                        if ((UbxNavSat_Loop_DatReg = '1') and (unsigned(UbxNavSat_SatCounter_DatReg) < unsigned(SatInfo_DatReg.NumberOfSeenSats))) then 
                            if (unsigned(UbxNavSat_InLoopCounter_DatReg) < (unsigned(UbxNavSat_OffsetLoopLength_Con) - 1)) then -- this is not the last byte of the loop
                                UbxNavSat_InLoopCounter_DatReg <= std_logic_vector(unsigned(UbxNavSat_InLoopCounter_DatReg) + 1); 
                                if (UbxNavSat_InLoopCounter_DatReg = UbxNavSat_OffsetQualInd_Con(3 downto 0)) then 
                                    if ((unsigned(MsgData_DatReg(2 downto 0)) >= 4) and (MsgData_DatReg(5 downto 4) = "01")) then -- if Signal QualInd (bits 2-0) is locked, and signal health (bits 5-4) is good
                                        SatInfo_DatReg.NumberOfLockedSats <= std_logic_vector(unsigned(SatInfo_DatReg.NumberOfLockedSats) + 1);-- consider the receiver locked to the Sat
                                    end if;
                                end if;
                            else -- this is the last byte of the loop
                                if (unsigned(UbxNavSat_SatCounter_DatReg) = unsigned(SatInfo_DatReg.NumberOfSeenSats)-1) then -- this is the last byte of the last loop
                                    DetectUbxMsgState <= UbxChecksum_ChecksumA_St;
                                end if;
                                UbxNavSat_InLoopCounter_DatReg <= (others => '0');
                                UbxNavSat_SatCounter_DatReg <= std_logic_vector(unsigned(UbxNavSat_SatCounter_DatReg) + 1);
                            end if;
                        elsif (UbxNavSat_Loop_DatReg = '1') then 
                            DetectUbxMsgState <= Idle_St; -- parsing error 
                            UbxParseError_DatReg <= '1'; 
                        end if;

                    -- Extract info from Payload of UBX-NAV-Status
                    when UbxNavStatus_CheckPayload_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if (UbxPayloadCount_CntReg = UbxNavStatus_OffsetGnssFix_Con) then 
                            AntennaFix_DatReg.GnssFix <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = UbxNavStatus_OffsetGnssFixOk_Con) then
                            AntennaFix_DatReg.GnssFixOk <= MsgData_DatReg(0);
                        elsif (UbxPayloadCount_CntReg = UbxNavStatus_OffsetSpoofDet_Con) then
                            AntennaFix_DatReg.SpoofDetState <= MsgData_DatReg(4 downto 3);
                        end if;
                        if (unsigned(UbxPayloadCount_CntReg) < (unsigned(UbxNavStatus_Length_Con) - 1)) then 
                            UbxPayloadCount_CntReg <= std_logic_Vector(unsigned(UbxPayloadCount_CntReg) + 1);
                        else
                            DetectUbxMsgState <= UbxChecksum_ChecksumA_St;
                        end if;

                    -- Extract info from Payload of UBX-NAV-TimeLs
                    when UbxNavTimeLs_CheckPayload_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if (UbxPayloadCount_CntReg = UbxNavTimeLs_OffsetSrcCurrLs_Con) then 
                            UtcOffsetInfo_DatReg.SrcOfCurLeapSecond <= MsgData_DatReg; -- offset of the Sat system to UTC
                        -- get the TAI-UTC offset, since TAI is used by the adjustable clock 
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeLs_OffsetCurrLs_Con) then 
                            if ((UtcOffsetInfo_DatReg.SrcOfCurLeapSecond = x"01") or (UtcOffsetInfo_DatReg.SrcOfCurLeapSecond = x"02") or (UtcOffsetInfo_DatReg.SrcOfCurLeapSecond = x"05")) then -- GPS, or derived from dif GPS to Glonass, or Galileo
                                UtcOffsetInfo_DatReg.CurrentUtcOffset <= std_logic_vector(unsigned(MsgData_DatReg) + 19); -- add the GPS-TAI offset to UTC-GPS offset
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffset <= std_logic_vector(unsigned(MsgData_DatReg) + 19); -- add the GPS-TAI offset to UTC-GPS offset
                            elsif (UtcOffsetInfo_DatReg.SrcOfCurLeapSecond = x"04") then -- Beidou
                                UtcOffsetInfo_DatReg.CurrentUtcOffset <= std_logic_vector(unsigned(MsgData_DatReg) + 33); -- add the Beidou-TAI offset to UTC-Beidou offset
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffset <= std_logic_vector(unsigned(MsgData_DatReg) + 33); -- add the Beidou-TAI offset to UTC-Beidou offset
                            else -- else assume that the utc offset is provided directly
                                UtcOffsetInfo_DatReg.CurrentUtcOffset <= MsgData_DatReg;
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffset <= MsgData_DatReg;
                            end if;
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeLs_SrcLsChange_Con) then
                            -- if the src of the leap second change is GPS, GAL, GLO, or Beidu, then we can trust the leap second info
                            if ((MsgData_DatReg = x"02") or (MsgData_DatReg = x"04") or (MsgData_DatReg = x"05") or (MsgData_DatReg = x"06")) then 
                                UtcOffsetInfo_DatReg.LeapChangeValid <= '1';
                            else
                                UtcOffsetInfo_DatReg.LeapChangeValid <= '0';
                            end if;
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeLs_OffsetLsChange_Con) then
                            if (MsgData_DatReg = x"00") then -- no leap second scheduled
                                UtcOffsetInfo_DatReg.LeapAnnouncement <= '0';
                                UtcOffsetInfo_DatReg.Leap59 <= '0';
                                UtcOffsetInfo_DatReg.Leap61 <= '0';
                            elsif (MsgData_DatReg = x"01") then -- leap second 61
                                UtcOffsetInfo_DatReg.LeapAnnouncement <= '1';
                                UtcOffsetInfo_DatReg.Leap59 <= '0';
                                UtcOffsetInfo_DatReg.Leap61 <= '1';
                            elsif (MsgData_DatReg = x"FF") then -- leap second 59
                                UtcOffsetInfo_DatReg.LeapAnnouncement <= '1';
                                UtcOffsetInfo_DatReg.Leap59 <= '1';
                                UtcOffsetInfo_DatReg.Leap61 <= '0';
                            else 
                                UtcOffsetInfo_DatReg.LeapAnnouncement <= '0';
                                UtcOffsetInfo_DatReg.Leap59 <= '0';
                                UtcOffsetInfo_DatReg.Leap61 <= '0';
                            end if;
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeLs_OffsetTimeToLs_Con) then
                            UtcOffsetInfo_DatReg.TimeToLeapSecond(7 downto 0) <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = std_logic_vector(unsigned(UbxNavTimeLs_OffsetTimeToLs_Con) + 1)) then
                            UtcOffsetInfo_DatReg.TimeToLeapSecond(15 downto 8) <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = std_logic_vector(unsigned(UbxNavTimeLs_OffsetTimeToLs_Con) + 2)) then
                            UtcOffsetInfo_DatReg.TimeToLeapSecond(23 downto 16) <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = std_logic_vector(unsigned(UbxNavTimeLs_OffsetTimeToLs_Con) + 3)) then
                            UtcOffsetInfo_DatReg.TimeToLeapSecond(31 downto 24) <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeLs_OffsetValidLs_Con) then
                            UtcOffsetInfo_DatReg.CurrentUtcOffsetValid <= MsgData_DatReg(0);
                            UtcOffsetInfo_DatReg.TimeToLeapSecondValid <= MsgData_DatReg(1);
                            UtcOffsetInfo_DatReg.CurrentTaiGnssOffsetValid <= MsgData_DatReg(0);
                        end if;
                        if (unsigned(UbxPayloadCount_CntReg) < (unsigned(UbxNavTimeLs_Length_Con) - 1)) then 
                            UbxPayloadCount_CntReg <= std_logic_Vector(unsigned(UbxPayloadCount_CntReg) + 1);
                        else
                            DetectUbxMsgState <= UbxChecksum_ChecksumA_St;
                        end if;

                    -- Extract info from Payload of UBX-NAV-TimeUtc
                    when UbxNavTimeUtc_CheckPayload_St => 
                        UbxChecksumA_DatReg <= UbxChecksumA_DatReg + unsigned(MsgData_DatReg);
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- ChecksumB gets now the previous value of ChecksumA. Its latest value will be added later.
                        if (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetYear_Con) then 
                            RxToD_DatReg.Year(7 downto 0) <= MsgData_DatReg;
                        elsif (UbxPayloadCount_CntReg = std_logic_vector(unsigned(UbxNavTimeUtc_OffsetYear_Con) + 1)) then
                            RxToD_DatReg.Year(11 downto 8) <= MsgData_DatReg(3 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetMonth_Con) then
                            RxToD_DatReg.Month <= MsgData_DatReg(3 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetDay_Con) then
                            RxToD_DatReg.Day <= MsgData_DatReg(4 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetHour_Con) then
                            RxToD_DatReg.Hour <= MsgData_DatReg(4 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetMinute_Con) then
                            RxToD_DatReg.Minute <= MsgData_DatReg(5 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetSecond_Con) then
                            RxToD_DatReg.Second <= MsgData_DatReg(5 downto 0);
                        elsif (UbxPayloadCount_CntReg = UbxNavTimeUtc_OffsetUtcValid_Con) then
                            RxToD_DatReg.Valid <= MsgData_DatReg(2);
                        end if;
                        if (unsigned(UbxPayloadCount_CntReg) < (unsigned(UbxNavTimeUtc_Length_Con) - 1)) then 
                            UbxPayloadCount_CntReg <= std_logic_Vector(unsigned(UbxPayloadCount_CntReg) + 1);
                        else
                            DetectUbxMsgState <= UbxChecksum_ChecksumA_St;
                        end if;

                    when UbxChecksum_ChecksumA_St => 
                        UbxChecksumB_DatReg <= UbxChecksumB_DatReg + UbxChecksumA_DatReg ; -- The latest value of ChecksumA is added now.
                        if (MsgData_DatReg = std_logic_vector(UbxChecksumA_DatReg)) then 
                            DetectUbxMsgState <= UbxChecksum_ChecksumB_St;
                        else
                            UbxChecksumError_DatReg <= '1';
                            DetectUbxMsgState <= Idle_St;
                        end if;

                    when UbxChecksum_ChecksumB_St => 
                        if (MsgData_DatReg = std_logic_vector(UbxChecksumB_DatReg)) then 
                            -- The Format and Checksum of the received message is valid. Activate the valid flag of the extracted info.
                            if (CheckMsg_UbxMonHw_DatReg = '1') then
                                AntennaInfoValid_ValReg <= '1'; 
                            elsif (CheckMsg_UbxNavSat_DatReg = '1') then 
                                SatInfoValid_ValReg <= '1';
                            elsif (CheckMsg_UbxNavStatus_DatReg = '1') then 
                                AntennaFixValid_ValReg <= '1';
                            elsif (CheckMsg_UbxNavTimeLs_DatReg = '1') then 
                                UtcOffsetInfoValid_ValReg <= '1';
                            elsif (CheckMsg_UbxNavTimeUtc_DatReg = '1') then
                                RxToDValid_ValReg <= '1';
                            end if;
                        else
                            UbxChecksumError_DatReg <= '1';
                        end if;
                        DetectUbxMsgState <= Idle_St;

                end case;
            end if;
            
            -- If TSIPv1 is enabled, first remove the padded Delimiter1 bytes
            TsipEoF_DatReg <= '0';
            TsipMsgDataValid_ValReg <= '0';
            if (Enable_Tsip_Ena = '1') then 
                MsgDataOldValid_ValReg <= MsgDataValid_ValReg;
                if (MsgDataValid_ValReg = '1') then 
                    MsgDataOld_DatReg <= MsgData_DatReg;
                    if ((MsgDataOld_DatReg = Tsip_Delimiter1_Con) and (MsgData_DatReg = Tsip_Delimiter1_Con) and (TsipPaddingSkipped_DatReg = '0')) then 
                        TsipPaddingSkipped_DatReg <= '1'; -- when 2 Delimiter1 bytes are in sequence, skip the 2nd byte. A 3rd Delimiter1 byte in sequence is accepted, a 4th is skipped and so on...
                    else
                        TsipMsgData_DatReg <= MsgDataOld_DatReg;
                        TsipMsgDataValid_ValReg <= '1';
                        TsipPaddingSkipped_DatReg <= '0'; -- clear the flag, when there is no skipping of delimiter 
                    end if;
                    
                    -- when Delimiter2 follows a Delimiter1 byte which was not skipped, it is the end of frame
                    if ((MsgDataOld_DatReg = Tsip_Delimiter1_Con) and (MsgData_DatReg = Tsip_Delimiter2_Con) and (TsipPaddingSkipped_DatReg = '0')) then
                        TsipEoF_DatReg  <= '1';
                    end if;
                elsif (TsipEoF_DatReg = '1') then -- at the end of the msg, send also the last character to tsip msg and clear the buffer
                    TsipMsgData_DatReg <= MsgDataOld_DatReg;
                    TsipMsgDataValid_ValReg <= '1';
                    TsipPaddingSkipped_DatReg <= '0'; -- clear the flag, when there is no skipping of delimiter 
                    MsgDataOld_DatReg <= (others=>'0');
                end if;
            else
                MsgDataOld_DatReg <= (others=>'0');
                MsgDataOldValid_ValReg <= '0';
                TsipMsgData_DatReg <= (others=>'0');
                TsipPaddingSkipped_DatReg <= '0';
            end if;
            
            -- Decoding of the TSIPv1 messages             
            TsipParseError_DatReg <= '0';
            TsipChecksumError_DatReg <= '0';
            TsipPosition_MsgValOld_ValReg <= TsipPosition_MsgVal_ValReg;
            TsipReceiver_MsgValOld_ValReg <= TsipReceiver_MsgVal_ValReg;
            TsipTiming_MsgValOld_ValReg <= TsipTiming_MsgVal_ValReg;
            TsipAlarms_MsgValOld_ValReg <= TsipAlarms_MsgVal_ValReg;
            TsipSatellite_MsgValOld_ValReg <= TsipSatellite_MsgVal_ValReg;
            if (TsipMsgDataValid_ValReg = '1') then
                case (DetectTsipMsgState) is
                    when Idle_St => 
                        TsipChecksum_DatReg <= (others=>'0');
                        TsipMsg_A1_DatReg <= '0';
                        TsipMsg_A2_DatReg <= '0';
                        TsipMsg_A3_DatReg <= '0';
                        TsipTiming_DatReg <= '0';
                        TsipPosition_DatReg <= '0';
                        TsipSatellite_DatReg <= '0';
                        TsipAlarms_DatReg <= '0';  
                        TsipReceiver_DatReg <= '0';
                        TsipLength_DatReg <= (others=>'0');
                        TsipPayloadCount_CntReg <= (others=>'0');
                        TsipReceiver_ODC_DatReg <= '0';
                        -- Start decoding only if the TSIP messages are enabled
                        if (Enable_Tsip_Ena = '1') then 
                            if ((TsipMsgData_DatReg = Tsip_Delimiter1_Con) and (TsipEoF_DatReg /= '1')) then -- Frame start byte
                                DetectTsipMsgState <= TsipPacket_Id_St;
                                TsipPayloadCount_CntReg <= std_logic_vector(to_unsigned(1,16));  
                            end if;
                        end if;

                    when TsipPacket_Id_St => 
                        TsipChecksum_DatReg <= TsipMsgData_DatReg;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        DetectTsipMsgState <= TsipPacket_SubId_St;
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;
                            TsipParseError_DatReg <= '1';                            
                        elsif (TsipMsgData_DatReg = TsipTiming_ID_Con(15 downto 8)) then -- A1 packet
                            TsipMsg_A1_DatReg <= '1';
                        elsif (TsipMsgData_DatReg = TsipSatellite_ID_Con(15 downto 8)) then -- A2 packet
                            TsipMsg_A2_DatReg <= '1';
                        elsif (TsipMsgData_DatReg = TsipAlarms_ID_Con(15 downto 8)) then -- A3 packet
                            TsipMsg_A3_DatReg <= '1';
                        else 
                            DetectTsipMsgState <= Idle_St; -- not supported message
                        end if;

                    when TsipPacket_SubId_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        DetectTsipMsgState <= TsipLength1_St;
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;
                            TsipParseError_DatReg <= '1';
                        elsif ((TsipMsg_A1_DatReg = '1') and (TsipMsgData_DatReg = TsipTiming_ID_Con(7 downto 0))) then -- Timing Info
                            TsipTiming_DatReg <= '1';
                        elsif ((TsipMsg_A1_DatReg = '1') and (TsipMsgData_DatReg = TsipPosition_ID_Con(7 downto 0))) then-- Position Info
                            TsipPosition_DatReg <= '1';
                        elsif ((TsipMsg_A2_DatReg = '1') and (TsipMsgData_DatReg = TsipSatellite_ID_Con(7 downto 0))) then-- Satellite Info
                            TsipSatellite_DatReg <= '1';
                        elsif ((TsipMsg_A3_DatReg = '1') and (TsipMsgData_DatReg = TsipAlarms_ID_Con(7 downto 0))) then-- System Alarms
                            TsipAlarms_DatReg <= '1';
                        elsif ((TsipMsg_A3_DatReg = '1') and (TsipMsgData_DatReg = TsipReceiver_ID_Con(7 downto 0))) then-- Receiver Status
                            TsipReceiver_DatReg <= '1';
                        else
                            DetectTsipMsgState <= Idle_St; -- not supported message
                        end if;

                    when TsipLength1_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        TsipLength_DatReg(15 downto 8) <= TsipMsgData_DatReg;
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;
                            TsipParseError_DatReg <= '1';
                        else
                            DetectTsipMsgState <= TsipLength2_St;
                        end if;
                        
                    when TsipLength2_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        TsipLength_DatReg(7 downto 0) <= TsipMsgData_DatReg;
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;
                            TsipParseError_DatReg <= '1';
                        else
                            DetectTsipMsgState <= TsipMode_St;
                        end if;
                        
                    when TsipMode_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;
                            TsipParseError_DatReg <= '1';                        
                        elsif (TsipMsgData_DatReg = TsipModeResponse_Con) then -- assure that this is a response message
                            if (TsipTiming_DatReg = '1') then 
                                TsipTiming_MsgVal_ValReg <= '0'; -- invalidate msg flag until verifying that the frame is valid
                                DetectTsipMsgState <= TsipTiming_Data_St;
                            elsif (TsipPosition_DatReg = '1') then 
                                TsipPosition_MsgVal_ValReg <= '0'; -- invalidate msg flag until verifying that the frame is valid
                                DetectTsipMsgState <= TsipPosition_Data_St;
                            elsif (TsipSatellite_DatReg = '1') then 
                                TsipSatellite_MsgVal_ValReg <= '0'; -- invalidate msg flag until verifying that the frame is valid
                                TsipSatellite_IncreaseSeenSat_DatReg <= '0';
                                TsipSatellite_IncreaseLockSat_DatReg <= '0';
                                DetectTsipMsgState <= TsipSatellite_Data_St;
                            elsif (TsipAlarms_DatReg = '1') then 
                                TsipAlarms_MsgVal_ValReg <= '0'; -- invalidate msg flag until verifying that the frame is valid
                                DetectTsipMsgState <= TsipAlarms_Data_St;
                            elsif (TsipReceiver_DatReg = '1') then 
                                TsipReceiver_MsgVal_ValReg <= '0'; -- invalidate msg flag until verifying that the frame is valid
                                DetectTsipMsgState <= TsipReceiver_Data_St;
                            else -- it should  never happen
                                DetectTsipMsgState <= Idle_St;
                            end if;
                        else
                            DetectTsipMsgState <= Idle_St; -- not supported message
                        end if;

                    when TsipTiming_Data_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        if (TsipPayloadCount_CntReg = TsipTiming_OffsetHours_Con) then 
                            RxToD_DatReg.Hour <= TsipMsgData_DatReg(4 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetMinutes_Con) then
                            RxToD_DatReg.Minute <= TsipMsgData_DatReg(5 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetSeconds_Con) then
                            RxToD_DatReg.Second <= TsipMsgData_DatReg(5 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetMonth_Con) then
                            RxToD_DatReg.Month <= TsipMsgData_DatReg(3 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetDay_Con) then
                            RxToD_DatReg.Day <= TsipMsgData_DatReg(4 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetYearHigh_Con) then
                            RxToD_DatReg.Year(11 downto 8) <= TsipMsgData_DatReg(3 downto 0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetYearLow_Con) then
                            RxToD_DatReg.Year(7 downto 0) <= TsipMsgData_DatReg;
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetTimebase_Con) then
                            UtcOffsetInfo_DatReg.SrcOfCurLeapSecond <= TsipMsgData_DatReg;
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetFlags_Con) then
                            RxToD_DatReg.Valid  <= TsipMsgData_DatReg(1);
                            UtcOffsetInfo_DatReg.CurrentUtcOffsetValid <= TsipMsgData_DatReg(0);
                            UtcOffsetInfo_DatReg.CurrentTaiGnssOffsetValid <= TsipMsgData_DatReg(0);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetUtcOffsetHigh_Con) then
                            if (TsipMsgData_DatReg/= x"00") then -- utc offset should be positive and less than 256
                                UtcOffsetInfo_DatReg.CurrentUtcOffsetValid <= '0';
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffsetValid <= '0';
                            end if;
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetUtcOffsetLow_Con) then
                            if ((UtcOffsetInfo_DatReg.SrcOfCurLeapSecond(3 downto 0) = "0000") or (UtcOffsetInfo_DatReg.SrcOfCurLeapSecond(2 downto 0) = "0011")) then -- for GPS and GAL 
                                UtcOffsetInfo_DatReg.CurrentUtcOffset <= std_logic_vector(unsigned(TsipMsgData_DatReg) + 19); -- add the GPS-TAI offset to UTC-GPS offset
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffset <= std_logic_vector(to_unsigned(19,8)); -- the GPS-TAI offset will be added to GPS time to calculate the TAI 
                            elsif (UtcOffsetInfo_DatReg.SrcOfCurLeapSecond(3 downto 0) = "0010")  then -- for BEIDU
                                UtcOffsetInfo_DatReg.CurrentUtcOffset <= std_logic_vector(unsigned(TsipMsgData_DatReg) + 33); -- add the Beidou-TAI offset to UTC-Beidou offset
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffset <= std_logic_vector(to_unsigned(33,8)); -- the Beidu-TAI offset will be added to Beidu time to calculate the TAI 
                            else
                                UtcOffsetInfo_DatReg.CurrentUtcOffsetValid <= '0'; -- else invalidate the utc offset
                                UtcOffsetInfo_DatReg.CurrentTaiGnssOffsetValid <= '0'; -- else invalidate the utc offset
                            end if;
                        end if;
                        if (TsipEoF_DatReg = '1') then 
                            TsipParseError_DatReg <= '1';
                            DetectTsipMsgState <= Idle_St;                        
                        elsif (TsipPayloadCount_CntReg = std_logic_vector(unsigned(TsipLength_DatReg) + unsigned(TsipLengthOffset_Con))) then 
                            DetectTsipMsgState <= TsipChecksum_St;
                        end if;
                    
                    when TsipPosition_Data_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        if (TsipPayloadCount_CntReg = TsipPosition_OffsetFixType_Con) then 
                            -- TSIP AntennaFix: 0:No/1:2D/2:3D
                            -- Reg AntennaFix : 0:No/2:2D/3:3D
                            if (TsipMsgData_DatReg = x"01") then 
                                AntennaFix_DatReg.GnssFix <= x"02"; --2D
                            elsif (TsipMsgData_DatReg = x"02") then
                                AntennaFix_DatReg.GnssFix <= x"03"; --3D
                            else
                                AntennaFix_DatReg.GnssFix <= x"00"; -- no fix
                            end if;
                        end if;
                        if (TsipEoF_DatReg = '1') then 
                            TsipParseError_DatReg <= '1';
                            DetectTsipMsgState <= Idle_St;                        
                        elsif (TsipPayloadCount_CntReg = std_logic_vector(unsigned(TsipLength_DatReg) + unsigned(TsipLengthOffset_Con))) then 
                            DetectTsipMsgState <= TsipChecksum_St;
                        end if;
                    
                    when TsipSatellite_Data_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        if (TsipPayloadCount_CntReg = TsipSatellite_OffsetId_Con) then 
                            if (unsigned(TsipMsgData_DatReg) > 99) then -- invalid ID, mark parse error
                                TsipParseError_DatReg <= '1';
                                DetectTsipMsgState <= Idle_St;
                            end if;
                        elsif (TsipPayloadCount_CntReg = TsipSatellite_OffsetFlags_Con) then 
                            if (TsipMsgData_DatReg(0) = '1') then -- satellite acquired
                                TsipSatellite_IncreaseSeenSat_DatReg <= '1'; -- flag that the satellite should be considered "seen"
                            end if;
                            if (TsipMsgData_DatReg(2) = '1') then -- satellite used in time fix
                                TsipSatellite_IncreaseLockSat_DatReg <= '1'; -- flag that the satellite should be considered "locked"
                            end if;
                        end if;
                        if (TsipEoF_DatReg = '1') then 
                            TsipParseError_DatReg <= '1';
                            DetectTsipMsgState <= Idle_St;                        
                        elsif (TsipPayloadCount_CntReg = std_logic_vector(unsigned(TsipLength_DatReg) + unsigned(TsipLengthOffset_Con))) then 
                            DetectTsipMsgState <= TsipChecksum_St;
                        end if;

                    when TsipAlarms_Data_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);                         
                        if (TsipPayloadCount_CntReg = TsipTiming_OffsetMinorAlarmsHigh_Con) then 
                            UtcOffsetInfo_DatReg.Leap59 <= TsipMsgData_DatReg(2);
                            UtcOffsetInfo_DatReg.Leap61 <= TsipMsgData_DatReg(1);
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetMinorAlarmsLow_Con) then 
                            UtcOffsetInfo_DatReg.LeapAnnouncement <= TsipMsgData_DatReg(2);
                            if (TsipMsgData_DatReg(0) = '1') then   
                                AntennaInfo_DatReg.Status <= "100"; -- antenna open
                            elsif (TsipMsgData_DatReg(1) = '1') then   
                                AntennaInfo_DatReg.Status <= "011"; -- antenna shorted
                            else
                                AntennaInfo_DatReg.Status <= "010"; -- if no alarms, antenna ok
                            end if;
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetMajorAlarmsHigh_Con) then 
                            if (TsipMsgData_DatReg(0) = '1') then  -- if there is a jamming alarm
                                AntennaInfo_DatReg.JamInd <= x"FF"; -- indicate strong jamming
                                AntennaInfo_DatReg.JamState <= "11"; -- indicate critical state
                            else
                                AntennaInfo_DatReg.JamInd <= x"00"; -- indicate no jamming
                                AntennaInfo_DatReg.JamState <= "01"; -- indicate OK state
                            end if;
                        elsif (TsipPayloadCount_CntReg = TsipTiming_OffsetMajorAlarmsLow_Con) then 
                            TsipAlarms_NoSatellites_DatReg <= TsipMsgData_DatReg(0); -- not tracking satelites
                            TsipAlarms_BadPps_DatReg <= TsipMsgData_DatReg(1); -- bad PPS
                            TsipAlarms_NoPps_DatReg <= TsipMsgData_DatReg(2); -- no PPS
                            if (TsipMsgData_DatReg(7) = '1') then -- spoofing alarm
                                AntennaFix_DatReg.SpoofDetState <= "10";
                            else
                                AntennaFix_DatReg.SpoofDetState <= "01";
                            end if;
                        end if;
                        if (TsipEoF_DatReg = '1') then 
                            TsipParseError_DatReg <= '1';
                            DetectTsipMsgState <= Idle_St;                        
                        elsif (TsipPayloadCount_CntReg = std_logic_vector(unsigned(TsipLength_DatReg) + unsigned(TsipLengthOffset_Con))) then 
                            DetectTsipMsgState <= TsipChecksum_St;
                        end if;
                    
                    when TsipReceiver_Data_St => 
                        for i in 7 downto 0 loop
                            TsipChecksum_DatReg(i) <= (TsipChecksum_DatReg(i) xor TsipMsgData_DatReg(i));
                        end loop;
                        TsipPayloadCount_CntReg <= std_logic_vector(unsigned(TsipPayloadCount_CntReg) + 1);  
                        if (TsipPayloadCount_CntReg = TsipReceiver_OffsetMode_Con) then 
                            if (TsipMsgData_DatReg = TsipReceiver_ModeODC_Con) then 
                                TsipReceiver_ODC_DatReg <= '1';
                            end if;
                        elsif (TsipPayloadCount_CntReg = TsipReceiver_OffsetStatus_Con) then 
                            if ((TsipReceiver_ODC_DatReg = '1') and (TsipMsgData_DatReg = TsipReceiver_SatusGnssFix_Con)) then -- overdetermined clock with fix time
                                AntennaFix_DatReg.GnssFixOk <= '1';
                            else
                                AntennaFix_DatReg.GnssFixOk <= '0';
                            end if;
                        end if;
                        if (TsipEoF_DatReg = '1') then 
                            TsipParseError_DatReg <= '1';
                            DetectTsipMsgState <= Idle_St;                        
                        elsif (TsipPayloadCount_CntReg = std_logic_vector(unsigned(TsipLength_DatReg) + unsigned(TsipLengthOffset_Con))) then 
                            DetectTsipMsgState <= TsipChecksum_St;
                        end if;
                    
                    when TsipChecksum_St => 
                        if (TsipEoF_DatReg = '1') then 
                            DetectTsipMsgState <= Idle_St;                        
                            TsipParseError_DatReg <= '1';
                        elsif (TsipMsgData_DatReg /= TsipChecksum_DatReg) then 
                            DetectTsipMsgState <= Idle_St; 
                            TsipChecksumError_DatReg <= '1';
                        else
                            DetectTsipMsgState <= TsipEoF1_St;                        
                        end if;
                        
                    when TsipEof1_St => 
                        -- verify that the end of the frame matches with the expected length 
                        if (TsipEoF_DatReg = '1') then 
                            -- the frame has valid format=> activate the validity flags
                            if (TsipTiming_DatReg = '1') then 
                                TsipTiming_MsgVal_ValReg <= '1';
                            elsif (TsipPosition_DatReg = '1') then 
                                TsipPosition_MsgVal_ValReg <= '1'; 
                            elsif (TsipSatellite_DatReg = '1') then 
                                TsipSatellite_MsgVal_ValReg <= '1';
                                if (TsipSatellite_IncreaseSeenSat_DatReg = '1') then 
                                    TsipSatellite_CntSatellites_DatReg.NumberOfSeenSats <= std_logic_vector(unsigned(TsipSatellite_CntSatellites_DatReg.NumberOfSeenSats) + 1);
                                end if;
                                if (TsipSatellite_IncreaseLockSat_DatReg = '1') then 
                                    TsipSatellite_CntSatellites_DatReg.NumberOfLockedSats <= std_logic_vector(unsigned(TsipSatellite_CntSatellites_DatReg.NumberOfLockedSats) + 1);
                                end if;
                            elsif (TsipAlarms_DatReg = '1') then 
                                TsipAlarms_MsgVal_ValReg <= '1';
                                TsipAlarms_ClearSatellites_DatReg <= TsipAlarms_NoSatellites_DatReg; -- the msg is valid. store the alarm if the satellite numbers should be cleared. 
                                UtcOffsetInfo_DatReg.TimeToLeapSecondValid <= UtcOffsetInfo_DatReg.CurrentUtcOffsetValid; -- the alarms msg is valid, so the leap second info is valid if the UTC offset is valid.
                            elsif (TsipReceiver_DatReg = '1') then 
                                TsipReceiver_MsgVal_ValReg <= '1';
                            end if;
                            DetectTsipMsgState <= TsipEoF2_St;                        
                        else
                            DetectTsipMsgState <= Idle_St;                        
                            TsipParseError_DatReg <= '1';
                        end if;
                    when TsipEof2_St => -- the end of character has been already checked. 
                        DetectTsipMsgState <= Idle_St;

                end case;
            end if;
            
            if ((Disable_UbxNavTimeUtc_Ena = '1') and (Enable_Ubx_Ena = '1')) then 
                RxToD_DatReg <= ToD_Type_Reset;
                RxToDValid_ValReg <= '0';
            elsif ((Disable_TsipTiming_Ena = '1') and (Enable_Tsip_Ena = '1')) then 
                RxToD_DatReg <= ToD_Type_Reset;
                TsipTiming_MsgVal_ValReg <= '0';
            end if;

            if ((ClockTime_Nanosecond_DatIn(29) = '0' and ClockTime_Nanosecond_DatReg(29) = '1') and (Enable_Tsip_Ena = '1')) then -- a new second begins, store and clear the TSIP satellite counters
                SatInfo_DatReg.NumberOfSeenSats <= TsipSatellite_CntSatellites_DatReg.NumberOfSeenSats; -- store the seen satellites
                SatInfo_DatReg.NumberOfLockedSats <= TsipSatellite_CntSatellites_DatReg.NumberOfLockedSats; -- store the locked satellites
                TsipSatellite_CntSatellites_DatReg <= SatInfo_Type_Reset; -- clear the counting 
            end if;
            
            if ((Disable_UbxNavSat_Ena = '1') and (Enable_Ubx_Ena = '1')) then 
                SatInfo_DatReg <= SatInfo_Type_Reset;
                SatInfoValid_ValReg <= '0';
            elsif ((Disable_TsipSatellite_Ena = '1') and (Enable_Tsip_Ena = '1')) then 
                SatInfo_DatReg <= SatInfo_Type_Reset;
                TsipSatellite_MsgVal_ValReg <= '0';
            end if;
            
            if ((Disable_UbxHwMon_Ena = '1') and (Enable_Ubx_Ena = '1')) then 
                AntennaInfo_DatReg <= AntennaInfo_Type_Reset;
                AntennaInfoValid_ValReg <= '0';
            -- reset all records that the message contributes to 
            elsif ((Disable_TsipAlarms_Ena = '1') and (Enable_Tsip_Ena = '1')) then 
                AntennaFix_DatReg <= AntennaFix_Type_Reset;
                AntennaInfo_DatReg <= AntennaInfo_Type_Reset;
                UtcOffsetInfo_DatReg <= UtcOffsetInfo_Type_Reset;
                TsipAlarms_MsgVal_ValReg <= '0';
            end if;

            if ((Disable_UbxNavStatus_Ena = '1') and (Enable_Ubx_Ena = '1')) then 
                AntennaFix_DatReg <= AntennaFix_Type_Reset;
                AntennaFixValid_ValReg <= '0';
            -- if one of the msgs that contribute are disabled, then invalidate all the corresponding records
            elsif ((Disable_TsipPosition_Ena = '1')  and (Enable_Tsip_Ena = '1')) then 
                AntennaFix_DatReg <= AntennaFix_Type_Reset;
                TsipPosition_MsgVal_ValReg <= '0';
            elsif ((Disable_TsipReceiver_Ena = '1') and (Enable_Tsip_Ena = '1')) then
                AntennaFix_DatReg <= AntennaFix_Type_Reset;
                TsipReceiver_MsgVal_ValReg <= '0';                
            end if;
            
            if ((Disable_UbxNavTimeLs_Ena = '1') and (Enable_Ubx_Ena = '1')) then 
                UtcOffsetInfo_DatReg <= UtcOffsetInfo_Type_Reset;
                UtcOffsetInfoValid_ValReg <= '0';
            end if;
            
            if (Enable_Ena = '0') then 
                AntennaInfo_DatReg <= AntennaInfo_Type_Reset;
                AntennaInfoValid_ValReg <= '0';

                AntennaFix_DatReg <= AntennaFix_Type_Reset;
                AntennaFixValid_ValReg <= '0';

                SatInfo_DatReg <= SatInfo_Type_Reset;
                SatInfoValid_ValReg <= '0';

                UtcOffsetInfo_DatReg <= UtcOffsetInfo_Type_Reset;
                UtcOffsetInfoValid_ValReg <= '0';

                RxToD_DatReg <= ToD_Type_Reset;
                RxToDValid_ValReg <= '0';

                TsipReceiver_MsgVal_ValReg <= '0';
                TsipPosition_MsgVal_ValReg <= '0';
                TsipAlarms_MsgVal_ValReg <= '0';
                TsipSatellite_MsgVal_ValReg <= '0';
                TsipTiming_MsgVal_ValReg <= '0';
                
                DetectUbxMsgState <= Idle_St;
                DetectTsipMsgState <= Idle_St;
            end if;
        end if;
    end process DecodeMsgFsm_Prc;

    -- Each supported message type is expected to be received once per second. 
    -- If a valid message type is not received for 3 seconds, then the valid flag of its corresponding register will be deactivated.
    MsgTimeout_Prc: process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            UbxNavSat_TimeoutCounter_CntReg  <= 0;
            UbxHwMon_TimeoutCounter_CntReg <= 0;
            UbxNavStatus_TimeoutCounter_CntReg <= 0;
            UbxNavTimeUtc_TimeoutCounter_CntReg <= 0;
            UbxNavTimeLs_TimeoutCounter_CntReg <= 0;
            MillisecondCounter_CntReg <= 0;
            MillisecondFlag_EvtReg  <= '0';
            
            UbxNavSat_Timeout_EvtReg <= '0';
            UbxHwMon_Timeout_EvtReg <= '0';
            UbxNavStatus_Timeout_EvtReg <= '0';
            UbxNavTimeUtc_Timeout_EvtReg <= '0';
            UbxNavTimeLs_Timeout_EvtReg <= '0';
            
            TsipTiming_Timeout_EvtReg <= '0';
            TsipReceiver_Timeout_EvtReg <= '0';
            TsipAlarms_Timeout_EvtReg <= '0';
            TsipPosition_Timeout_EvtReg <= '0';
            TsipSatellite_Timeout_EvtReg <= '0';
            
            TsipTiming_TimeoutCounter_CntReg <= 0;
            TsipReceiver_TimeoutCounter_CntReg <= 0;
            TsipAlarms_TimeoutCounter_CntReg <= 0;
            TsipPosition_TimeoutCounter_CntReg <= 0;
            TsipSatellite_TimeoutCounter_CntReg <= 0;
            
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            MillisecondFlag_EvtReg <= '0';
            UbxNavSat_Timeout_EvtReg <= '0';
            UbxHwMon_Timeout_EvtReg <= '0';
            UbxNavStatus_Timeout_EvtReg <= '0';
            UbxNavTimeUtc_Timeout_EvtReg <= '0';
            UbxNavTimeLs_Timeout_EvtReg <= '0';
            
            TsipTiming_Timeout_EvtReg <= '0';
            TsipReceiver_Timeout_EvtReg <= '0';
            TsipAlarms_Timeout_EvtReg <= '0';
            TsipPosition_Timeout_EvtReg <= '0';
            TsipSatellite_Timeout_EvtReg <= '0';

            -- count millisecond
            if (MillisecondCounter_CntReg < (NanosInMillisecond_Con-ClockPeriod_Gen)) then 
                MillisecondCounter_CntReg <= MillisecondCounter_CntReg + ClockPeriod_Gen;
            else    
                MillisecondCounter_CntReg <= 0;
                MillisecondFlag_EvtReg <= '1';
            end if;

            -- UbxNavSat timeout check 
            if ((SatInfoValid_ValReg = '1') and (SatInfoValid_ValOldReg = '0')) then -- a new message has just been validated
                UbxNavSat_TimeoutCounter_CntReg <= 0;
            elsif (UbxNavSat_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    UbxNavSat_TimeoutCounter_CntReg <= UbxNavSat_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                UbxNavSat_Timeout_EvtReg <= '1';
            end if;
            
            -- UbxHwMon timeout check 
            if ((AntennaInfoValid_ValReg = '1') and (AntennaInfoValid_ValOldReg = '0')) then -- a new message has just been validated
                UbxHwMon_TimeoutCounter_CntReg <= 0;
            elsif (UbxHwMon_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    UbxHwMon_TimeoutCounter_CntReg <= UbxHwMon_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                UbxHwMon_Timeout_EvtReg <= '1';
            end if;
               
            -- UbxNavStatus timeout check 
            if ((AntennaFixValid_ValReg = '1') and (AntennaFixValid_ValOldReg = '0')) then -- a new message has just been validated
                UbxNavStatus_TimeoutCounter_CntReg <= 0;
            elsif (UbxNavStatus_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    UbxNavStatus_TimeoutCounter_CntReg <= UbxNavStatus_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                UbxNavStatus_Timeout_EvtReg <= '1';
            end if;
            
            -- UbxNavTimeUtc timeout check 
            if ((RxToDValid_ValReg = '1') and (RxToDValid_ValOldReg = '0')) then -- a new message has just been validated
                UbxNavTimeUtc_TimeoutCounter_CntReg <= 0;
            elsif (UbxNavTimeUtc_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    UbxNavTimeUtc_TimeoutCounter_CntReg <= UbxNavTimeUtc_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                UbxNavTimeUtc_Timeout_EvtReg <= '1';
            end if;

            -- UbxNavTimeLs timeout check 
            if ((UtcOffsetInfoValid_ValReg = '1') and (UtcOffsetInfoValid_ValOldReg = '0')) then -- a new message has just been validated
                UbxNavTimeLs_TimeoutCounter_CntReg <= 0;
            elsif (UbxNavTimeLs_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    UbxNavTimeLs_TimeoutCounter_CntReg <= UbxNavTimeLs_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                UbxNavTimeLs_Timeout_EvtReg <= '1';
            end if;
            
            -- Tsip timing timeout check 
            if ((TsipTiming_MsgVal_ValReg = '1') and (TsipTiming_MsgValOld_ValReg = '0')) then -- a new message has just been validated
                TsipTiming_TimeoutCounter_CntReg <= 0;
            elsif (TsipTiming_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    TsipTiming_TimeoutCounter_CntReg <= TsipTiming_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                TsipTiming_Timeout_EvtReg <= '1';
            end if;
            
            -- Tsip Position timeout check 
            if ((TsipPosition_MsgVal_ValReg = '1') and (TsipPosition_MsgValOld_ValReg = '0')) then -- a new message has just been validated
                TsipPosition_TimeoutCounter_CntReg <= 0;
            elsif (TsipPosition_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    TsipPosition_TimeoutCounter_CntReg <= TsipPosition_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                TsipPosition_Timeout_EvtReg <= '1';
            end if;
            
            -- Tsip Receiver timeout check 
            if ((TsipReceiver_MsgVal_ValReg = '1') and (TsipReceiver_MsgValOld_ValReg = '0')) then -- a new message has just been validated
                TsipReceiver_TimeoutCounter_CntReg <= 0;
            elsif (TsipReceiver_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    TsipReceiver_TimeoutCounter_CntReg <= TsipReceiver_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                TsipReceiver_Timeout_EvtReg <= '1';
            end if;
            
            -- Tsip Alarms timeout check 
            if ((TsipAlarms_MsgVal_ValReg = '1') and (TsipAlarms_MsgValOld_ValReg = '0')) then -- a new message has just been validated
                TsipAlarms_TimeoutCounter_CntReg <= 0;
            elsif (TsipAlarms_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    TsipAlarms_TimeoutCounter_CntReg <= TsipAlarms_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                TsipAlarms_Timeout_EvtReg <= '1';
            end if;
            
            -- Tsip Satellite timeout check 
            if ((TsipSatellite_MsgVal_ValReg = '1') and (TsipSatellite_MsgValOld_ValReg = '0')) then -- a new message has just been validated
                TsipSatellite_TimeoutCounter_CntReg <= 0;
            elsif (TsipSatellite_TimeoutCounter_CntReg < MsgTimeoutMillisecond_Con) then -- timeout count 
                if (MillisecondFlag_EvtReg = '1') then 
                    TsipSatellite_TimeoutCounter_CntReg <= TsipSatellite_TimeoutCounter_CntReg + 1;
                end if;
            else -- timeout reached
                TsipSatellite_Timeout_EvtReg <= '1';
            end if;
            
        end if;
    end process MsgTimeout_Prc;
    
    -- Convert the UTC to TAI and change the time format from 0xYYYYMMDDHHmmSS to 0xSSSSSSSS
    -- The TAI is generated happens if a new valid time is received and the last received UTC offset is also valid
    -- The TAI is not generated if the timestamp is right before or after a potential leap second update
    ConvertTime_Prc:  process(SysClk_ClkIn, SysRstN_RstIn) is
    begin
        if (SysRstN_RstIn = '0') then
            ClockTime_Second_DatReg <= (others => '0');
            TimeAdjustment_Second_DatReg <= (others => '0');
            TimeAdjustment_Nanosecond_DatReg <= (others => '0'); 
            TimeAdjustment_ValReg <= '0';          
            GnssTime_Second_DatReg <= (others => '0');        
            TaiConversionState_StaReg <= Idle_St;        
            LeapYear_DatReg <= '0';        
            SkipTaiConversion_ValReg <= '0';
            TimeCounter_CntReg <= 0;
            Year_004_Counter_CntReg <= 0; -- leap year every 4 years 
            Year_100_Counter_CntReg <= 0; -- no leap year every 100 years
            Year_400_Counter_CntReg <= 0; -- leap year every 400 years

            ToD_DatReg <= ToD_Type_Reset;
            ToDValid_ValReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            ClockTime_Second_DatReg <= ClockTime_Second_DatIn;
            TimeAdjustment_Second_DatReg <= (others => '0');
            TimeAdjustment_Nanosecond_DatReg <= (others => '0'); -- jump exactly at a new second
            TimeAdjustment_ValReg <= '0';          
            SkipTaiConversion_ValReg <= '0';

            case TaiConversionState_StaReg is 
                when Idle_St => 
                    -- if the time has been updated from the GNSS receiver  
                    if (((RxToDValid_ValReg = '1') and (RxToDValid_ValOldReg = '0') and (Enable_Ubx_Ena = '1')) or 
                        ((TsipTiming_MsgVal_ValReg = '1') and (TsipTiming_MsgValOld_ValReg = '0') and (Enable_Tsip_Ena = '1'))) then 
                        -- if the latest time and UTC offset are valid (a valid Time LS message with valid UTC offset needs to be received first)
                        if ((RxToD_DatReg.Valid = '1') and (UtcOffsetInfo_DatReg.CurrentTaiGnssOffsetValid = '1') and (ClockTime_ValIn = '1') ) then  
                            -- if the timestamp is right before or right after a potential leap second, do not convert to a new TAI 
                            if (((unsigned(RxToD_DatReg.Hour) = 23) and (unsigned(RxToD_DatReg.Minute) = 59) and (unsigned(RxToD_DatReg.Second) >= 57) and -- at the end of a day 
                                 (((unsigned(RxToD_DatReg.Month) = 12) and (unsigned(RxToD_DatReg.Day) = 31)) or -- at the end of december 
                                  ((unsigned(RxToD_DatReg.Month) = 6) and (unsigned(RxToD_DatReg.Day) = 30)) or -- at the end of june  
                                  ((unsigned(RxToD_DatReg.Month) = 3) and (unsigned(RxToD_DatReg.Day) = 31)) or -- at the end of march 
                                  ((unsigned(RxToD_DatReg.Month) = 9) and (unsigned(RxToD_DatReg.Day) = 30)))) or -- at the end of september  
                                ((unsigned(RxToD_DatReg.Hour) = 0) and (unsigned(RxToD_DatReg.Minute) = 0) and (unsigned(RxToD_DatReg.Second) <= 3) and -- at the beginning of a day 
                                 (((unsigned(RxToD_DatReg.Month) = 1) and (unsigned(RxToD_DatReg.Day) = 1)) or -- at the beginning of January 
                                  ((unsigned(RxToD_DatReg.Month) = 7) and (unsigned(RxToD_DatReg.Day) = 1)) or -- at the beginning of July  
                                  ((unsigned(RxToD_DatReg.Month) = 4) and (unsigned(RxToD_DatReg.Day) = 1)) or -- at the beginning of April
                                  ((unsigned(RxToD_DatReg.Month) = 10) and (unsigned(RxToD_DatReg.Day) = 1))))) then  -- at the beginning of October
                                 SkipTaiConversion_ValReg <= '1';
                            else
                                TaiConversionState_StaReg <= ConvertYears_St;
                                GnssTime_Second_DatReg <= (others => '0');
                                TimeCounter_CntReg <= 1970; -- count the years since the beginning of TAI
                                Year_004_Counter_CntReg <= 2;
                                Year_100_Counter_CntReg <= 70;
                                Year_400_Counter_CntReg <= 370;
                                ToD_DatReg <= RxToD_DatReg;
                                ToDValid_ValReg <= ToDValid_ValReg;
                                LeapYear_DatReg <= '0';
                            end if;
                        end if;
                    end if;

                when ConvertYears_St => 
                    if (TimeCounter_CntReg < unsigned(ToD_DatReg.Year)) then 
                        TimeCounter_CntReg <= TimeCounter_CntReg + 1;
                        Year_400_Counter_CntReg <= Year_400_Counter_CntReg + 1;
                        Year_100_Counter_CntReg <= Year_100_Counter_CntReg + 1;
                        Year_004_Counter_CntReg <= Year_004_Counter_CntReg + 1;
                        if (Year_400_Counter_CntReg = 400) then -- leap year every 400 years
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerYear_Con + SecondsPerDay_Con),SecondWidth_Con)); 
                            Year_400_Counter_CntReg <= 1;
                            if (Year_100_Counter_CntReg = 100) then -- clear the other counters
                                Year_100_Counter_CntReg <= 1;
                            end if;
                            if (Year_004_Counter_CntReg = 4) then -- clear the other counters
                                Year_004_Counter_CntReg <= 1;
                            end if;                           
                        elsif (Year_100_Counter_CntReg = 100) then -- no leap year every 100 years
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerYear_Con),SecondWidth_Con)); 
                            Year_100_Counter_CntReg <= 1;
                            if (Year_004_Counter_CntReg = 4) then -- clear the other counters
                                Year_004_Counter_CntReg <= 1;
                            end if;                            
                        elsif (Year_004_Counter_CntReg = 4) then -- leap year every 4 years
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerYear_Con + SecondsPerDay_Con),SecondWidth_Con)); 
                            Year_004_Counter_CntReg <= 1;
                        else -- no leap year 
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerYear_Con),SecondWidth_Con));
                        end if;
                    else -- reached the current year
                        if (Year_400_Counter_CntReg = 400) then -- leap year every 400 years
                            LeapYear_DatReg <= '1';
                        elsif (Year_100_Counter_CntReg = 100) then -- no leap year every 100 years
                            LeapYear_DatReg <= '0';
                        elsif (Year_004_Counter_CntReg = 4) then -- leap year every 4 years
                            LeapYear_DatReg <= '1';
                        else -- no leap year 
                            LeapYear_DatReg <= '0';
                        end if;
                        TimeCounter_CntReg <= 1;
                        TaiConversionState_StaReg <= ConvertMonths_St;
                    end if;

                when ConvertMonths_St =>
                    if (TimeCounter_CntReg < unsigned(ToD_DatReg.Month)) then 
                        TimeCounter_CntReg <= TimeCounter_CntReg + 1;
                        if ((TimeCounter_CntReg = 2) and (LeapYear_DatReg = '1')) then -- at February check if this is a leap year
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerMonthArray_Con(TimeCounter_CntReg) + SecondsPerDay_Con),SecondWidth_Con)); 
                        else
                            GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerMonthArray_Con(TimeCounter_CntReg)), SecondWidth_Con)); 
                        end if;
                    else
                        TimeCounter_CntReg <= 1;
                        TaiConversionState_StaReg <= ConvertDays_St;
                    end if;

                when ConvertDays_St =>
                    if (TimeCounter_CntReg < unsigned(ToD_DatReg.Day)) then 
                        TimeCounter_CntReg <= TimeCounter_CntReg + 1;
                        GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerDay_Con), SecondWidth_Con)); 
                    else
                        TimeCounter_CntReg <= 0;
                        TaiConversionState_StaReg <= ConvertHours_St;
                    end if;

                when ConvertHours_St => 
                    if (TimeCounter_CntReg < unsigned(ToD_DatReg.Hour)) then 
                        TimeCounter_CntReg <= TimeCounter_CntReg + 1;
                        GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerHour_Con), SecondWidth_Con)); 
                    else
                        TimeCounter_CntReg <= 0;
                        TaiConversionState_StaReg <= ConvertMinutes_St;
                    end if;

                when ConvertMinutes_St => 
                    if (TimeCounter_CntReg < unsigned(ToD_DatReg.Minute)) then 
                        TimeCounter_CntReg <= TimeCounter_CntReg + 1;
                        GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + SecondsPerMinute_Con), SecondWidth_Con)); 
                    else
                        TimeCounter_CntReg <= 0;
                        TaiConversionState_StaReg <= CalcTai_St;
                    end if;

                when CalcTai_St => 
                    -- finally add the second field and the utc offset and depending which second we received an extra second
                    if (ReceiveCurrentTime_Gen = true) then 
                        GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + unsigned(UtcOffsetInfo_DatReg.CurrentTaiGnssOffset) + unsigned(ToD_DatReg.Second) + 1), SecondWidth_Con)); 
                    else -- then next second is provided in advance
                        GnssTime_Second_DatReg <= std_logic_vector(resize((unsigned(GnssTime_Second_DatReg)  + unsigned(UtcOffsetInfo_DatReg.CurrentTaiGnssOffset) + unsigned(ToD_DatReg.Second)), SecondWidth_Con)); 
                    end if;
                    TaiConversionState_StaReg <= TimeAdjust_St;

                when TimeAdjust_St => 
                    -- wait for the next seconds to change
                    -- or if a time jump happened we wait again for the next measurements for our adjustments
                    if (ClockTime_TimeJump_DatIn = '1') then
                        TaiConversionState_StaReg <= Idle_St;
                    elsif (ClockTime_Second_DatIn /= ClockTime_Second_DatReg) then
                        -- if we need to adjust
                        if (ClockTime_Second_DatIn /= GnssTime_Second_DatReg) then
                            TimeAdjustment_Second_DatReg <= GnssTime_Second_DatReg;
                            TimeAdjustment_Nanosecond_DatReg <= std_logic_vector(to_unsigned((ClockPeriod_Gen*3), 32)); -- it takes 3 clock cycles until clock is set
                            TimeAdjustment_ValReg <= '1';          
                        end if;
                        TaiConversionState_StaReg <= Idle_St;
                    end if;
            end case;
        end if;
    end process ConvertTime_Prc;
    
    -- Access configuration and monitoring registers via an AXI4L slave
    Axi_Prc : process(SysClk_ClkIn, SysRstN_RstIn) is
    variable TempAddress                            : std_logic_vector(31 downto 0) := (others => '0');
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
                
            Axi_Init_Proc(TodSlaveControl_Reg_Con, TodSlaveControl_DatReg);
            Axi_Init_Proc(TodSlaveStatus_Reg_Con, TodSlaveStatus_DatReg);
            Axi_Init_Proc(TodSlaveUartPolarity_Reg_Con, TodSlaveUartPolarity_DatReg);
            Axi_Init_Proc(TodSlaveVersion_Reg_Con, TodSlaveVersion_DatReg);
            Axi_Init_Proc(TodSlaveCorrection_Reg_Con, TodSlaveCorrection_DatReg);
            Axi_Init_Proc(TodSlaveUartBaudRate_Reg_Con, TodSlaveUartBaudRate_DatReg);
            Axi_Init_Proc(TodSlaveUtcStatus_Reg_Con, TodSlaveUtcStatus_DatReg);
            Axi_Init_Proc(TodSlaveTimeToLeapSecond_Reg_Con, TodSlaveTimeToLeapSecond_DatReg);
            Axi_Init_Proc(TodSlaveAntennaStatus_Reg_Con, TodSlaveAntennaStatus_DatReg);
            Axi_Init_Proc(TodSlaveSatNumber_Reg_Con, TodSlaveSatNumber_DatReg);
            if (UartPolarity_Gen = true) then
                TodSlaveUartPolarity_DatReg(TodSlaveUartPolarity_PolarityBit_Con) <= '1';
            else
                TodSlaveUartPolarity_DatReg(TodSlaveUartPolarity_PolarityBit_Con) <= '0';
            end if;
            TodSlaveUartBaudRate_DatReg(3 downto 0) <= std_logic_vector(to_unsigned(UartDefaultBaudRate_Gen, 4)); 
            
            AntennaInfoValid_ValOldReg <= '0';
            SatInfoValid_ValOldReg <= '0';
            AntennaFixValid_ValOldReg <= '0';
            UtcOffsetInfoValid_ValOldReg <= '0';
            RxToDValid_ValOldReg <= '0';
        elsif ((SysClk_ClkIn'event) and (SysClk_ClkIn = '1')) then
            AntennaInfoValid_ValOldReg <= AntennaInfoValid_ValReg;
            SatInfoValid_ValOldReg <= SatInfoValid_ValReg;
            AntennaFixValid_ValOldReg <= AntennaFixValid_ValReg;
            UtcOffsetInfoValid_ValOldReg <= UtcOffsetInfoValid_ValReg;
            RxToDValid_ValOldReg <= RxToDValid_ValReg;
        
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
                        Axi_Read_Proc(TodSlaveControl_Reg_Con, TodSlaveControl_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveStatus_Reg_Con, TodSlaveStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveUartPolarity_Reg_Con, TodSlaveUartPolarity_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveVersion_Reg_Con, TodSlaveVersion_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveCorrection_Reg_Con, TodSlaveCorrection_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveUartBaudRate_Reg_Con, TodSlaveUartBaudRate_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveUtcStatus_Reg_Con, TodSlaveUtcStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveTimeToLeapSecond_Reg_Con, TodSlaveTimeToLeapSecond_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveAntennaStatus_Reg_Con, TodSlaveAntennaStatus_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_Read_Proc(TodSlaveSatNumber_Reg_Con, TodSlaveSatNumber_DatReg, TempAddress, AxiReadDataData_DatReg, AxiReadDataResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;    
                    end if;

                when Write_St => 
                    if (((AxiWriteAddrValid_ValIn = '1') and (AxiWriteAddrReady_RdyReg = '1')) and
                        ((AxiWriteDataValid_ValIn = '1') and (AxiWriteDataReady_RdyReg = '1'))) then
                        TempAddress := std_logic_vector(resize(unsigned(AxiWriteAddrAddress_AdrIn), 32));
                        AxiWriteRespValid_ValReg <= '1';
                        AxiWriteRespResponse_DatReg <= Axi_RespSlvErr_Con;
                        Axi_Write_Proc(TodSlaveControl_Reg_Con, TodSlaveControl_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveStatus_Reg_Con, TodSlaveStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveUartPolarity_Reg_Con, TodSlaveUartPolarity_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveVersion_Reg_Con, TodSlaveVersion_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveCorrection_Reg_Con, TodSlaveCorrection_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveUartBaudRate_Reg_Con, TodSlaveUartBaudRate_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveUtcStatus_Reg_Con, TodSlaveUtcStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveTimeToLeapSecond_Reg_Con, TodSlaveTimeToLeapSecond_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveAntennaStatus_Reg_Con, TodSlaveAntennaStatus_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_Write_Proc(TodSlaveSatNumber_Reg_Con, TodSlaveSatNumber_DatReg, TempAddress, AxiWriteDataData_DatIn, AxiWriteRespResponse_DatReg);
                        Axi_AccessState_StaReg <= Resp_St;
                    end if; 

                when Resp_St =>
                    if (((AxiWriteRespValid_ValReg = '1') and (AxiWriteRespReady_RdyIn = '1')) or 
                        ((AxiReadDataValid_ValReg = '1') and (AxiReadDataReady_RdyIn = '1'))) then
                        Axi_AccessState_StaReg <= Idle_St;    
                    end if;
                    
                when others =>

            end case;  

            TodSlaveCorrection_DatReg <= (others => '0'); -- unused!
            
            if (Enable_Ena = '1') then 
                if ((UbxParseError_DatReg = '1') or (TsipParseError_DatReg = '1'))then 
                    TodSlaveStatus_DatReg(0) <= '1';
                end if;
                if ((UbxChecksumError_DatReg = '1') or (TsipChecksumError_DatReg = '1')) then 
                    TodSlaveStatus_DatReg(1) <= '1';
                end if;
                if (UartError_DatReg = '1') then 
                    TodSlaveStatus_DatReg(2) <= '1';
                end if;
                
                if ((UbxNavTimeLs_Timeout_EvtReg = '1') and (Enable_Ubx_Ena = '1'))then 
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_UtcOffsetValidBit_Con) <= '0';
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapInfoValidBit_Con) <= '0';
                elsif ((UtcOffsetInfoValid_ValReg = '1') and (UtcOffsetInfoValid_ValOldReg = '0') and (Enable_Ubx_Ena = '1')) then 
                    TodSlaveUtcStatus_DatReg(7 downto 0) <= UtcOffsetInfo_DatReg.CurrentUtcOffset;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_UtcOffsetValidBit_Con) <= UtcOffsetInfo_DatReg.CurrentUtcOffsetValid;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapAnnounceBit_Con) <= UtcOffsetInfo_DatReg.LeapAnnouncement;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_Leap59Bit_Con) <= UtcOffsetInfo_DatReg.Leap59;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_Leap61Bit_Con) <= UtcOffsetInfo_DatReg.Leap61;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapInfoValidBit_Con) <= UtcOffsetInfo_DatReg.LeapChangeValid; 
                    TodSlaveTimeToLeapSecond_DatReg <= UtcOffsetInfo_DatReg.TimeToLeapSecond;
                elsif ((TsipTiming_Timeout_EvtReg = '1') and (Enable_Tsip_Ena = '1'))then 
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_UtcOffsetValidBit_Con) <= '0';
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapInfoValidBit_Con) <= '0';
                elsif ((TsipAlarms_Timeout_EvtReg = '1') and (Enable_Tsip_Ena = '1'))then 
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapInfoValidBit_Con) <= '0';
                elsif ((((TsipTiming_MsgVal_ValReg = '1') and (TsipTiming_MsgValOld_ValReg = '0') and ( TsipAlarms_MsgVal_ValReg = '1')) -- if a new timing message is just received and alarms is already received
                    or (((TsipAlarms_MsgVal_ValReg = '1') and (TsipAlarms_MsgValOld_ValReg = '0') and ( TsipTiming_MsgVal_ValReg = '1')))) -- if a new alarms message is just received and timing is already received
                    and (Enable_Tsip_Ena = '1')) then 
                    TodSlaveUtcStatus_DatReg(7 downto 0) <= UtcOffsetInfo_DatReg.CurrentUtcOffset;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_UtcOffsetValidBit_Con) <= UtcOffsetInfo_DatReg.CurrentUtcOffsetValid;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapAnnounceBit_Con) <= UtcOffsetInfo_DatReg.LeapAnnouncement;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_Leap59Bit_Con) <= UtcOffsetInfo_DatReg.Leap59;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_Leap61Bit_Con) <= UtcOffsetInfo_DatReg.Leap61;
                    TodSlaveUtcStatus_DatReg(TodSlaveUtcStatus_LeapInfoValidBit_Con) <= UtcOffsetInfo_DatReg.TimeToLeapSecondValid;
                    TodSlaveTimeToLeapSecond_DatReg <= UtcOffsetInfo_DatReg.TimeToLeapSecond;
                end if;
                    
                if ((UbxHwMon_Timeout_EvtReg = '1') and (Enable_Ubx_Ena = '1')) then 
                    TodSlaveAntennaStatus_DatReg(29) <= '0';
                elsif ((AntennaInfoValid_ValReg = '1') and (AntennaInfoValid_ValOldReg = '0') and (Enable_Ubx_Ena = '1')) then 
                    TodSlaveAntennaStatus_DatReg(2 downto 0) <= AntennaInfo_DatReg.Status;
                    TodSlaveAntennaStatus_DatReg(4 downto 3) <= AntennaInfo_DatReg.JamState;
                    TodSlaveAntennaStatus_DatReg(12 downto 5) <= AntennaInfo_DatReg.JamInd;
                    TodSlaveAntennaStatus_DatReg(29) <= '1';
                elsif ((TsipAlarms_Timeout_EvtReg = '1') and (Enable_Tsip_Ena = '1'))then 
                    TodSlaveAntennaStatus_DatReg(29) <= '0';
                elsif ((TsipAlarms_MsgVal_ValReg = '1') and (TsipAlarms_MsgValOld_ValReg = '0') and (Enable_Tsip_Ena = '1')) then
                    TodSlaveAntennaStatus_DatReg(2 downto 0) <= AntennaInfo_DatReg.Status;
                    TodSlaveAntennaStatus_DatReg(4 downto 3) <= AntennaInfo_DatReg.JamState;
                    TodSlaveAntennaStatus_DatReg(12 downto 5) <= AntennaInfo_DatReg.JamInd;
                    TodSlaveAntennaStatus_DatReg(29) <= '1';
                end if;    

                if ((UbxNavStatus_Timeout_EvtReg = '1') and (Enable_Ubx_Ena = '1'))then 
                    TodSlaveAntennaStatus_DatReg(28) <= '0';
                elsif ((AntennaFixValid_ValReg = '1') and (AntennaFixValid_ValOldReg = '0') and (Enable_Ubx_Ena = '1')) then 
                    TodSlaveAntennaStatus_DatReg(16) <= AntennaFix_DatReg.GnssFixOk;
                    TodSlaveAntennaStatus_DatReg(24 downto 17) <= AntennaFix_DatReg.GnssFix;
                    TodSlaveAntennaStatus_DatReg(26 downto 25) <= AntennaFix_DatReg.SpoofDetState;
                    TodSlaveAntennaStatus_DatReg(28) <= '1';
                -- if any of the messages that contribute to the record timeouts, invalidate the record
                elsif (((TsipReceiver_Timeout_EvtReg = '1') or (TsipPosition_Timeout_EvtReg = '1') or (TsipAlarms_Timeout_EvtReg = '1'))and (Enable_Tsip_Ena = '1'))then 
                    TodSlaveAntennaStatus_DatReg(28) <= '0';
                elsif ((((TsipReceiver_MsgVal_ValReg = '1') and (TsipReceiver_MsgValOld_ValReg = '0') and (TsipAlarms_MsgVal_ValReg = '1') and (TsipPosition_MsgVal_ValReg = '1')) -- if a new Receiver message is just received and Alarms and Position are already received
                    or (((TsipAlarms_MsgVal_ValReg = '1') and (TsipAlarms_MsgValOld_ValReg = '0') and (TsipReceiver_MsgVal_ValReg = '1') and (TsipPosition_MsgVal_ValReg = '1')))  -- if a new Alarms message is just received and Receiver and Position are already received
                    or (((TsipPosition_MsgVal_ValReg = '1') and (TsipPosition_MsgValOld_ValReg = '0') and (TsipReceiver_MsgVal_ValReg = '1') and (TsipAlarms_MsgVal_ValReg = '1')))) -- if a new Position message is just received and Receiver and Alarms are already received
                       and (Enable_Tsip_Ena = '1')) then 
                    TodSlaveAntennaStatus_DatReg(16) <= AntennaFix_DatReg.GnssFixOk;
                    TodSlaveAntennaStatus_DatReg(24 downto 17) <= AntennaFix_DatReg.GnssFix;
                    TodSlaveAntennaStatus_DatReg(26 downto 25) <= AntennaFix_DatReg.SpoofDetState;
                    TodSlaveAntennaStatus_DatReg(28) <= '1';
                end if;

                if ((UbxNavSat_Timeout_EvtReg = '1') and (Enable_Ubx_Ena = '1'))then 
                    TodSlaveSatNumber_DatReg(16) <= '0';
                elsif ((SatInfoValid_ValReg = '1') and (SatInfoValid_ValOldReg = '0') and (Enable_Ubx_Ena = '1')) then 
                    TodSlaveSatNumber_DatReg(7 downto 0) <= SatInfo_DatReg.NumberOfSeenSats;
                    TodSlaveSatNumber_DatReg(15 downto 8) <= SatInfo_DatReg.NumberOfLockedSats;
                    TodSlaveSatNumber_DatReg(16) <= '1';
                elsif (((TsipSatellite_Timeout_EvtReg = '1') or (TsipAlarms_ClearSatellites_DatReg = '1')) and (Enable_Tsip_Ena = '1'))then 
                    TodSlaveSatNumber_DatReg(16) <= '0';
                elsif ((ClockTime_Nanosecond_DatReg(29) = '0' and ClockTime_Nanosecond_OldDatReg(29) = '1') and (Enable_Tsip_Ena = '1')) then -- a new second begins, store the TSIP satellite counters
                    TodSlaveSatNumber_DatReg(7 downto 0) <= SatInfo_DatReg.NumberOfSeenSats;
                    TodSlaveSatNumber_DatReg(15 downto 8) <= SatInfo_DatReg.NumberOfLockedSats;
                    TodSlaveSatNumber_DatReg(16) <= '1';
                end if;
            else 
                TodSlaveStatus_DatReg <= (others => '0');
                TodSlaveUtcStatus_DatReg <= (others => '0');
                TodSlaveTimeToLeapSecond_DatReg <= (others => '0');
                TodSlaveAntennaStatus_DatReg <= (others => '0');
                TodSlaveSatNumber_DatReg <= (others => '0');
            end if;

        end if;
    end process Axi_Prc;

    --*************************************************************************************
    -- Instantiations and Port mapping
    --*************************************************************************************
     
end architecture TodSlave_Arch;
