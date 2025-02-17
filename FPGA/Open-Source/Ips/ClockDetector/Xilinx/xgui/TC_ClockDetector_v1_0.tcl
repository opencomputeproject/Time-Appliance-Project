# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "ClockSelect_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "PpsSelect_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.ClockSelect_Gen { PARAM_VALUE.ClockSelect_Gen } {
	# Procedure called to update ClockSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ClockSelect_Gen { PARAM_VALUE.ClockSelect_Gen } {
	# Procedure called to validate ClockSelect_Gen
	return true
}

proc update_PARAM_VALUE.PpsSelect_Gen { PARAM_VALUE.PpsSelect_Gen } {
	# Procedure called to update PpsSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.PpsSelect_Gen { PARAM_VALUE.PpsSelect_Gen } {
	# Procedure called to validate PpsSelect_Gen
	return true
}


proc update_MODELPARAM_VALUE.ClockSelect_Gen { MODELPARAM_VALUE.ClockSelect_Gen PARAM_VALUE.ClockSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ClockSelect_Gen}] ${MODELPARAM_VALUE.ClockSelect_Gen}
}

proc update_MODELPARAM_VALUE.PpsSelect_Gen { MODELPARAM_VALUE.PpsSelect_Gen PARAM_VALUE.PpsSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.PpsSelect_Gen}] ${MODELPARAM_VALUE.PpsSelect_Gen}
}

