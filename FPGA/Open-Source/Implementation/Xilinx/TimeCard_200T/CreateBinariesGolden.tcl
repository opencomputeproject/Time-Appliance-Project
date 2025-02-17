# ##########################################################################################
# Project: Time Card 200T
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

# current time
set SystemTime [clock seconds]

# set the current synth run
current_run -synthesis [get_runs synth_golden]
# set the current impl run
current_run -implementation [get_runs impl_golden]

# run synthese
reset_run synth_golden
launch_runs synth_golden -jobs 8
wait_on_run synth_golden

# run implementation and bitstream
reset_run impl_golden
launch_runs impl_golden -to_step write_bitstream -jobs 8
wait_on_run impl_golden

set TimestampDate [clock format $SystemTime -format %Y_%m_%d]
set TimestampTime [clock format $SystemTime -format %H_%M_%S]
set Timestamp "$TimestampDate $TimestampTime Golden"
set BinaryFolder "$ScriptFolder/Binaries/$Timestamp"

file mkdir $BinaryFolder

# date specific
file copy -force $ScriptFolder/TimeCard/TimeCard.runs/impl_golden/TimeCardTop.bit $BinaryFolder/Golden_TimeCardOS_200T.bit
file copy -force $ScriptFolder/TimeCard/TimeCard.runs/impl_golden/TimeCardTop.bin $BinaryFolder/Golden_TimeCardOS_200T.bin
write_hwdef -force -file $BinaryFolder/Golden_TimeCardOS_200T.hdf

# latest always here
file copy -force $ScriptFolder/TimeCard/TimeCard.runs/impl_golden/TimeCardTop.bit $ScriptFolder/Binaries/Golden_TimeCardOS_200T.bit
file copy -force $ScriptFolder/TimeCard/TimeCard.runs/impl_golden/TimeCardTop.bin $ScriptFolder/Binaries/Golden_TimeCardOS_200T.bin
write_hwdef -force -file $ScriptFolder/Binaries/Golden_TimeCardOS_200T.hdf