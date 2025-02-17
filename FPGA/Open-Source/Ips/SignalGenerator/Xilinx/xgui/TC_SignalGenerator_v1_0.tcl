# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "CableDelay_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "ClockPeriod_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "HighResFreqMultiply_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "OutputDelay_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "OutputPolarity_Gen" -parent ${Page_0}


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

proc update_PARAM_VALUE.OutputDelay_Gen { PARAM_VALUE.OutputDelay_Gen } {
	# Procedure called to update OutputDelay_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OutputDelay_Gen { PARAM_VALUE.OutputDelay_Gen } {
	# Procedure called to validate OutputDelay_Gen
	return true
}

proc update_PARAM_VALUE.OutputPolarity_Gen { PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to update OutputPolarity_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OutputPolarity_Gen { PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to validate OutputPolarity_Gen
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

proc update_MODELPARAM_VALUE.OutputDelay_Gen { MODELPARAM_VALUE.OutputDelay_Gen PARAM_VALUE.OutputDelay_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OutputDelay_Gen}] ${MODELPARAM_VALUE.OutputDelay_Gen}
}

proc update_MODELPARAM_VALUE.OutputPolarity_Gen { MODELPARAM_VALUE.OutputPolarity_Gen PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OutputPolarity_Gen}] ${MODELPARAM_VALUE.OutputPolarity_Gen}
}

proc update_MODELPARAM_VALUE.HighResFreqMultiply_Gen { MODELPARAM_VALUE.HighResFreqMultiply_Gen PARAM_VALUE.HighResFreqMultiply_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.HighResFreqMultiply_Gen}] ${MODELPARAM_VALUE.HighResFreqMultiply_Gen}
}

