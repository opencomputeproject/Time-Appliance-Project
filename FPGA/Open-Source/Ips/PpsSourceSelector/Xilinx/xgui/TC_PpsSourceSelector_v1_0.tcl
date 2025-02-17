# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "ClockClkPeriodNanosecond_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "PpsAvailableThreshold_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.ClockClkPeriodNanosecond_Gen { PARAM_VALUE.ClockClkPeriodNanosecond_Gen } {
	# Procedure called to update ClockClkPeriodNanosecond_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ClockClkPeriodNanosecond_Gen { PARAM_VALUE.ClockClkPeriodNanosecond_Gen } {
	# Procedure called to validate ClockClkPeriodNanosecond_Gen
	return true
}

proc update_PARAM_VALUE.PpsAvailableThreshold_Gen { PARAM_VALUE.PpsAvailableThreshold_Gen } {
	# Procedure called to update PpsAvailableThreshold_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.PpsAvailableThreshold_Gen { PARAM_VALUE.PpsAvailableThreshold_Gen } {
	# Procedure called to validate PpsAvailableThreshold_Gen
	return true
}


proc update_MODELPARAM_VALUE.ClockClkPeriodNanosecond_Gen { MODELPARAM_VALUE.ClockClkPeriodNanosecond_Gen PARAM_VALUE.ClockClkPeriodNanosecond_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ClockClkPeriodNanosecond_Gen}] ${MODELPARAM_VALUE.ClockClkPeriodNanosecond_Gen}
}

proc update_MODELPARAM_VALUE.PpsAvailableThreshold_Gen { MODELPARAM_VALUE.PpsAvailableThreshold_Gen PARAM_VALUE.PpsAvailableThreshold_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.PpsAvailableThreshold_Gen}] ${MODELPARAM_VALUE.PpsAvailableThreshold_Gen}
}

