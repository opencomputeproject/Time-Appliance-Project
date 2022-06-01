# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "VersionNumber_Gen" -parent ${Page_0}
  ipgui::add_param $IPINST -name "VersionNumber_Golden_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.VersionNumber_Gen { PARAM_VALUE.VersionNumber_Gen } {
	# Procedure called to update VersionNumber_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.VersionNumber_Gen { PARAM_VALUE.VersionNumber_Gen } {
	# Procedure called to validate VersionNumber_Gen
	return true
}

proc update_PARAM_VALUE.VersionNumber_Golden_Gen { PARAM_VALUE.VersionNumber_Golden_Gen } {
	# Procedure called to update VersionNumber_Golden_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.VersionNumber_Golden_Gen { PARAM_VALUE.VersionNumber_Golden_Gen } {
	# Procedure called to validate VersionNumber_Golden_Gen
	return true
}


proc update_MODELPARAM_VALUE.VersionNumber_Gen { MODELPARAM_VALUE.VersionNumber_Gen PARAM_VALUE.VersionNumber_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.VersionNumber_Gen}] ${MODELPARAM_VALUE.VersionNumber_Gen}
}

proc update_MODELPARAM_VALUE.VersionNumber_Golden_Gen { MODELPARAM_VALUE.VersionNumber_Golden_Gen PARAM_VALUE.VersionNumber_Golden_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.VersionNumber_Golden_Gen}] ${MODELPARAM_VALUE.VersionNumber_Golden_Gen}
}

