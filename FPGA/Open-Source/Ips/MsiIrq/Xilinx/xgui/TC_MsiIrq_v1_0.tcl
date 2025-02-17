# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  set LevelInterrupt_Gen [ipgui::add_param $IPINST -name "LevelInterrupt_Gen" -parent ${Page_0}]
  set_property tooltip {Level Interrupt  Bitmask} ${LevelInterrupt_Gen}
  ipgui::add_param $IPINST -name "NumberOfInterrupts_Gen" -parent ${Page_0}


}

proc update_PARAM_VALUE.LevelInterrupt_Gen { PARAM_VALUE.LevelInterrupt_Gen } {
	# Procedure called to update LevelInterrupt_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.LevelInterrupt_Gen { PARAM_VALUE.LevelInterrupt_Gen } {
	# Procedure called to validate LevelInterrupt_Gen
	return true
}

proc update_PARAM_VALUE.NumberOfInterrupts_Gen { PARAM_VALUE.NumberOfInterrupts_Gen } {
	# Procedure called to update NumberOfInterrupts_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.NumberOfInterrupts_Gen { PARAM_VALUE.NumberOfInterrupts_Gen } {
	# Procedure called to validate NumberOfInterrupts_Gen
	return true
}


proc update_MODELPARAM_VALUE.NumberOfInterrupts_Gen { MODELPARAM_VALUE.NumberOfInterrupts_Gen PARAM_VALUE.NumberOfInterrupts_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.NumberOfInterrupts_Gen}] ${MODELPARAM_VALUE.NumberOfInterrupts_Gen}
}

proc update_MODELPARAM_VALUE.LevelInterrupt_Gen { MODELPARAM_VALUE.LevelInterrupt_Gen PARAM_VALUE.LevelInterrupt_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.LevelInterrupt_Gen}] ${MODELPARAM_VALUE.LevelInterrupt_Gen}
}

