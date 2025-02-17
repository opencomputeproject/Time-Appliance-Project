# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "ClockPeriod_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "ReceiveCurrentTime_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "UartDefaultBaudRate_Gen" -parent ${Page_0} -widget comboBox
  ipgui::add_param $IPINST -name "UartPolarity_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.ClockPeriod_Gen { PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to update ClockPeriod_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ClockPeriod_Gen { PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to validate ClockPeriod_Gen
	return true
}

proc update_PARAM_VALUE.ReceiveCurrentTime_Gen { PARAM_VALUE.ReceiveCurrentTime_Gen } {
	# Procedure called to update ReceiveCurrentTime_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ReceiveCurrentTime_Gen { PARAM_VALUE.ReceiveCurrentTime_Gen } {
	# Procedure called to validate ReceiveCurrentTime_Gen
	return true
}

proc update_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to update Sim_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to validate Sim_Gen
	return true
}

proc update_PARAM_VALUE.UartDefaultBaudRate_Gen { PARAM_VALUE.UartDefaultBaudRate_Gen } {
	# Procedure called to update UartDefaultBaudRate_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.UartDefaultBaudRate_Gen { PARAM_VALUE.UartDefaultBaudRate_Gen } {
	# Procedure called to validate UartDefaultBaudRate_Gen
	return true
}

proc update_PARAM_VALUE.UartPolarity_Gen { PARAM_VALUE.UartPolarity_Gen } {
	# Procedure called to update UartPolarity_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.UartPolarity_Gen { PARAM_VALUE.UartPolarity_Gen } {
	# Procedure called to validate UartPolarity_Gen
	return true
}


proc update_MODELPARAM_VALUE.ClockPeriod_Gen { MODELPARAM_VALUE.ClockPeriod_Gen PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ClockPeriod_Gen}] ${MODELPARAM_VALUE.ClockPeriod_Gen}
}

proc update_MODELPARAM_VALUE.UartDefaultBaudRate_Gen { MODELPARAM_VALUE.UartDefaultBaudRate_Gen PARAM_VALUE.UartDefaultBaudRate_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.UartDefaultBaudRate_Gen}] ${MODELPARAM_VALUE.UartDefaultBaudRate_Gen}
}

proc update_MODELPARAM_VALUE.UartPolarity_Gen { MODELPARAM_VALUE.UartPolarity_Gen PARAM_VALUE.UartPolarity_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.UartPolarity_Gen}] ${MODELPARAM_VALUE.UartPolarity_Gen}
}

proc update_MODELPARAM_VALUE.ReceiveCurrentTime_Gen { MODELPARAM_VALUE.ReceiveCurrentTime_Gen PARAM_VALUE.ReceiveCurrentTime_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ReceiveCurrentTime_Gen}] ${MODELPARAM_VALUE.ReceiveCurrentTime_Gen}
}

proc update_MODELPARAM_VALUE.Sim_Gen { MODELPARAM_VALUE.Sim_Gen PARAM_VALUE.Sim_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Sim_Gen}] ${MODELPARAM_VALUE.Sim_Gen}
}

