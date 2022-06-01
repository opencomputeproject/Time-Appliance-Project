# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0" -display_name {SMA Sources}]
  set_property tooltip {SMA Sources} ${Page_0}
  ipgui::add_static_text $IPINST -name "Selection options" -parent ${Page_0} -text {Selection 1: Use SMA ports 1&2 as inputs and ports 3&4 as outputs

Selection 2: Use SMA ports 3&4 as inputs and ports 1&2 as outputs}
  #Adding Group
  set SMA_Input_Sources [ipgui::add_group $IPINST -name "SMA Input Sources" -parent ${Page_0}]
  set_property tooltip {SMA Input Sources} ${SMA_Input_Sources}
  ipgui::add_param $IPINST -name "SmaInput1SourceSelect_Gen" -parent ${SMA_Input_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaInput2SourceSelect_Gen" -parent ${SMA_Input_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaInput3SourceSelect_Gen" -parent ${SMA_Input_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaInput4SourceSelect_Gen" -parent ${SMA_Input_Sources} -widget comboBox

  #Adding Group
  set Output_Sources [ipgui::add_group $IPINST -name "Output Sources" -parent ${Page_0}]
  set_property tooltip {Output Sources} ${Output_Sources}
  ipgui::add_param $IPINST -name "SmaOutput1SourceSelect_Gen" -parent ${Output_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaOutput2SourceSelect_Gen" -parent ${Output_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaOutput3SourceSelect_Gen" -parent ${Output_Sources} -widget comboBox
  ipgui::add_param $IPINST -name "SmaOutput4SourceSelect_Gen" -parent ${Output_Sources} -widget comboBox



}

proc update_PARAM_VALUE.SmaInput1SourceSelect_Gen { PARAM_VALUE.SmaInput1SourceSelect_Gen } {
	# Procedure called to update SmaInput1SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaInput1SourceSelect_Gen { PARAM_VALUE.SmaInput1SourceSelect_Gen } {
	# Procedure called to validate SmaInput1SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaInput2SourceSelect_Gen { PARAM_VALUE.SmaInput2SourceSelect_Gen } {
	# Procedure called to update SmaInput2SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaInput2SourceSelect_Gen { PARAM_VALUE.SmaInput2SourceSelect_Gen } {
	# Procedure called to validate SmaInput2SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaInput3SourceSelect_Gen { PARAM_VALUE.SmaInput3SourceSelect_Gen } {
	# Procedure called to update SmaInput3SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaInput3SourceSelect_Gen { PARAM_VALUE.SmaInput3SourceSelect_Gen } {
	# Procedure called to validate SmaInput3SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaInput4SourceSelect_Gen { PARAM_VALUE.SmaInput4SourceSelect_Gen } {
	# Procedure called to update SmaInput4SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaInput4SourceSelect_Gen { PARAM_VALUE.SmaInput4SourceSelect_Gen } {
	# Procedure called to validate SmaInput4SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaOutput1SourceSelect_Gen { PARAM_VALUE.SmaOutput1SourceSelect_Gen } {
	# Procedure called to update SmaOutput1SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaOutput1SourceSelect_Gen { PARAM_VALUE.SmaOutput1SourceSelect_Gen } {
	# Procedure called to validate SmaOutput1SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaOutput2SourceSelect_Gen { PARAM_VALUE.SmaOutput2SourceSelect_Gen } {
	# Procedure called to update SmaOutput2SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaOutput2SourceSelect_Gen { PARAM_VALUE.SmaOutput2SourceSelect_Gen } {
	# Procedure called to validate SmaOutput2SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaOutput3SourceSelect_Gen { PARAM_VALUE.SmaOutput3SourceSelect_Gen } {
	# Procedure called to update SmaOutput3SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaOutput3SourceSelect_Gen { PARAM_VALUE.SmaOutput3SourceSelect_Gen } {
	# Procedure called to validate SmaOutput3SourceSelect_Gen
	return true
}

proc update_PARAM_VALUE.SmaOutput4SourceSelect_Gen { PARAM_VALUE.SmaOutput4SourceSelect_Gen } {
	# Procedure called to update SmaOutput4SourceSelect_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.SmaOutput4SourceSelect_Gen { PARAM_VALUE.SmaOutput4SourceSelect_Gen } {
	# Procedure called to validate SmaOutput4SourceSelect_Gen
	return true
}


proc update_MODELPARAM_VALUE.SmaInput1SourceSelect_Gen { MODELPARAM_VALUE.SmaInput1SourceSelect_Gen PARAM_VALUE.SmaInput1SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaInput1SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaInput1SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaInput2SourceSelect_Gen { MODELPARAM_VALUE.SmaInput2SourceSelect_Gen PARAM_VALUE.SmaInput2SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaInput2SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaInput2SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaInput3SourceSelect_Gen { MODELPARAM_VALUE.SmaInput3SourceSelect_Gen PARAM_VALUE.SmaInput3SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaInput3SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaInput3SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaInput4SourceSelect_Gen { MODELPARAM_VALUE.SmaInput4SourceSelect_Gen PARAM_VALUE.SmaInput4SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaInput4SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaInput4SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaOutput1SourceSelect_Gen { MODELPARAM_VALUE.SmaOutput1SourceSelect_Gen PARAM_VALUE.SmaOutput1SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaOutput1SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaOutput1SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaOutput2SourceSelect_Gen { MODELPARAM_VALUE.SmaOutput2SourceSelect_Gen PARAM_VALUE.SmaOutput2SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaOutput2SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaOutput2SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaOutput3SourceSelect_Gen { MODELPARAM_VALUE.SmaOutput3SourceSelect_Gen PARAM_VALUE.SmaOutput3SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaOutput3SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaOutput3SourceSelect_Gen}
}

proc update_MODELPARAM_VALUE.SmaOutput4SourceSelect_Gen { MODELPARAM_VALUE.SmaOutput4SourceSelect_Gen PARAM_VALUE.SmaOutput4SourceSelect_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SmaOutput4SourceSelect_Gen}] ${MODELPARAM_VALUE.SmaOutput4SourceSelect_Gen}
}

