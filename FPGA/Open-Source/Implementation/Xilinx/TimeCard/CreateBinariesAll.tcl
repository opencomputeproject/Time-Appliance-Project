# ##########################################################################################
# Project: Time Card
#
# Author: Thomas Schaub, NetTimeLogic GmbH
#
# License: Copyright (c) 2022, NetTimeLogic GmbH, Switzerland, <contact@nettimelogic.com>
# All rights reserved.
#
# THIS PROGRAM IS FREE SOFTWARE: YOU CAN REDISTRIBUTE IT AND/OR MODIFY
# IT UNDER THE TERMS OF THE GNU LESSER GENERAL PUBLIC LICENSE AS
# PUBLISHED BY THE FREE SOFTWARE FOUNDATION, VERSION 3.
#
# THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
# WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED WARRANTY OF
# MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. SEE THE GNU
# LESSER GENERAL LESSER PUBLIC LICENSE FOR MORE DETAILS.
#
# YOU SHOULD HAVE RECEIVED A COPY OF THE GNU LESSER GENERAL PUBLIC LICENSE
# ALONG WITH THIS PROGRAM. IF NOT, SEE <http://www.gnu.org/licenses/>.
#
# ##########################################################################################

set ScriptFile [file normalize [info script]]
set ScriptFolder [file dirname $ScriptFile]

cd $ScriptFolder

source "$ScriptFolder/CreateBinariesGolden.tcl"
source "$ScriptFolder/CreateBinaries.tcl"

write_cfgmem -format BIN -interface SPIx4 -size 16 -loadbit "up 0x00000000 $ScriptFolder/Binaries/Golden_TimeCardOS.bit up 0x0400000 $ScriptFolder/Binaries/TimeCardOS.bit" -file $ScriptFolder/Binaries/Factory_TimeCardOS.bin -force

# Creating Header
open_bd_design $ScriptFolder/TimeCard/TimeCard.srcs/sources_1/bd/TimeCard/TimeCard.bd
set VID [get_property CONFIG.VENDOR_ID [get_bd_cells /axi_pcie_0]]
set DID [get_property CONFIG.DEVICE_ID [get_bd_cells /axi_pcie_0]]
exec "$ScriptFolder/Additional Files/tft" -input $ScriptFolder/Binaries/TimeCardOS.bin -output $ScriptFolder/Binaries/TimeCardOS_Gotham.bin -vendor $VID -device $DID -apply
