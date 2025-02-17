# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "OutputPolarity_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.OutputPolarity_Gen { PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to update OutputPolarity_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OutputPolarity_Gen { PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to validate OutputPolarity_Gen
	return true
}

proc update_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to update Sim_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to validate Sim_Gen
	return true
}


proc update_MODELPARAM_VALUE.Sim_Gen { MODELPARAM_VALUE.Sim_Gen PARAM_VALUE.Sim_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Sim_Gen}] ${MODELPARAM_VALUE.Sim_Gen}
}

proc update_MODELPARAM_VALUE.OutputPolarity_Gen { MODELPARAM_VALUE.OutputPolarity_Gen PARAM_VALUE.OutputPolarity_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OutputPolarity_Gen}] ${MODELPARAM_VALUE.OutputPolarity_Gen}
}

