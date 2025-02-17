# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "CableDelay_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "ClockPeriod_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "HighResFreqMultiply_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "InputDelay_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "InputPolarity_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.CableDelay_Gen { PARAM_VALUE.CableDelay_Gen } {
	# Procedure called to update CableDelay_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.CableDelay_Gen { PARAM_VALUE.CableDelay_Gen } {
	# Procedure called to validate CableDelay_Gen
	return true
}

proc update_PARAM_VALUE.ClockPeriod_Gen { PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to update ClockPeriod_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ClockPeriod_Gen { PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to validate ClockPeriod_Gen
	return true
}

proc update_PARAM_VALUE.HighResFreqMultiply_Gen { PARAM_VALUE.HighResFreqMultiply_Gen } {
	# Procedure called to update HighResFreqMultiply_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.HighResFreqMultiply_Gen { PARAM_VALUE.HighResFreqMultiply_Gen } {
	# Procedure called to validate HighResFreqMultiply_Gen
	return true
}

proc update_PARAM_VALUE.InputDelay_Gen { PARAM_VALUE.InputDelay_Gen } {
	# Procedure called to update InputDelay_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.InputDelay_Gen { PARAM_VALUE.InputDelay_Gen } {
	# Procedure called to validate InputDelay_Gen
	return true
}

proc update_PARAM_VALUE.InputPolarity_Gen { PARAM_VALUE.InputPolarity_Gen } {
	# Procedure called to update InputPolarity_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.InputPolarity_Gen { PARAM_VALUE.InputPolarity_Gen } {
	# Procedure called to validate InputPolarity_Gen
	return true
}


proc update_MODELPARAM_VALUE.ClockPeriod_Gen { MODELPARAM_VALUE.ClockPeriod_Gen PARAM_VALUE.ClockPeriod_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ClockPeriod_Gen}] ${MODELPARAM_VALUE.ClockPeriod_Gen}
}

proc update_MODELPARAM_VALUE.CableDelay_Gen { MODELPARAM_VALUE.CableDelay_Gen PARAM_VALUE.CableDelay_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.CableDelay_Gen}] ${MODELPARAM_VALUE.CableDelay_Gen}
}

proc update_MODELPARAM_VALUE.InputDelay_Gen { MODELPARAM_VALUE.InputDelay_Gen PARAM_VALUE.InputDelay_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.InputDelay_Gen}] ${MODELPARAM_VALUE.InputDelay_Gen}
}

proc update_MODELPARAM_VALUE.InputPolarity_Gen { MODELPARAM_VALUE.InputPolarity_Gen PARAM_VALUE.InputPolarity_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.InputPolarity_Gen}] ${MODELPARAM_VALUE.InputPolarity_Gen}
}

proc update_MODELPARAM_VALUE.HighResFreqMultiply_Gen { MODELPARAM_VALUE.HighResFreqMultiply_Gen PARAM_VALUE.HighResFreqMultiply_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.HighResFreqMultiply_Gen}] ${MODELPARAM_VALUE.HighResFreqMultiply_Gen}
}

