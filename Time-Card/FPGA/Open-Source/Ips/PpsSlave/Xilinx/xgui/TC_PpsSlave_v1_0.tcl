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
  #Adding Group
  set Offset_Servo_Factors [ipgui::add_group $IPINST -name "Offset Servo Factors" -parent ${Page_0} -display_name {Offset Servo Coefficients}]
  set_property tooltip {Offset Servo Coefficients} ${Offset_Servo_Factors}
  ipgui::add_param $IPINST -name "OffsetDivI_Gen" -parent ${Offset_Servo_Factors}
  ipgui::add_param $IPINST -name "OffsetDivP_Gen" -parent ${Offset_Servo_Factors}
  ipgui::add_param $IPINST -name "OffsetMulI_Gen" -parent ${Offset_Servo_Factors}
  set OffsetMulP_Gen [ipgui::add_param $IPINST -name "OffsetMulP_Gen" -parent ${Offset_Servo_Factors}]
  set_property tooltip {Offset Multiply PFactor [0-1024]} ${OffsetMulP_Gen}

  #Adding Group
  set Servo_Factors [ipgui::add_group $IPINST -name "Servo Factors" -parent ${Page_0} -display_name {Drft Servo Coefficients}]
  set_property tooltip {Drft Servo Coefficients} ${Servo_Factors}
  ipgui::add_param $IPINST -name "DriftDivI_Gen" -parent ${Servo_Factors}
  ipgui::add_param $IPINST -name "DriftDivP_Gen" -parent ${Servo_Factors}
  ipgui::add_param $IPINST -name "DriftMulP_Gen" -parent ${Servo_Factors}
  set DriftMulI_Gen [ipgui::add_param $IPINST -name "DriftMulI_Gen" -parent ${Servo_Factors}]
  set_property tooltip {Drift Multiply I Factor [0-1024]} ${DriftMulI_Gen}



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

proc update_PARAM_VALUE.DriftDivI_Gen { PARAM_VALUE.DriftDivI_Gen } {
	# Procedure called to update DriftDivI_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DriftDivI_Gen { PARAM_VALUE.DriftDivI_Gen } {
	# Procedure called to validate DriftDivI_Gen
	return true
}

proc update_PARAM_VALUE.DriftDivP_Gen { PARAM_VALUE.DriftDivP_Gen } {
	# Procedure called to update DriftDivP_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DriftDivP_Gen { PARAM_VALUE.DriftDivP_Gen } {
	# Procedure called to validate DriftDivP_Gen
	return true
}

proc update_PARAM_VALUE.DriftMulI_Gen { PARAM_VALUE.DriftMulI_Gen } {
	# Procedure called to update DriftMulI_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DriftMulI_Gen { PARAM_VALUE.DriftMulI_Gen } {
	# Procedure called to validate DriftMulI_Gen
	return true
}

proc update_PARAM_VALUE.DriftMulP_Gen { PARAM_VALUE.DriftMulP_Gen } {
	# Procedure called to update DriftMulP_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DriftMulP_Gen { PARAM_VALUE.DriftMulP_Gen } {
	# Procedure called to validate DriftMulP_Gen
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

proc update_PARAM_VALUE.OffsetDivI_Gen { PARAM_VALUE.OffsetDivI_Gen } {
	# Procedure called to update OffsetDivI_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OffsetDivI_Gen { PARAM_VALUE.OffsetDivI_Gen } {
	# Procedure called to validate OffsetDivI_Gen
	return true
}

proc update_PARAM_VALUE.OffsetDivP_Gen { PARAM_VALUE.OffsetDivP_Gen } {
	# Procedure called to update OffsetDivP_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OffsetDivP_Gen { PARAM_VALUE.OffsetDivP_Gen } {
	# Procedure called to validate OffsetDivP_Gen
	return true
}

proc update_PARAM_VALUE.OffsetMulI_Gen { PARAM_VALUE.OffsetMulI_Gen } {
	# Procedure called to update OffsetMulI_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OffsetMulI_Gen { PARAM_VALUE.OffsetMulI_Gen } {
	# Procedure called to validate OffsetMulI_Gen
	return true
}

proc update_PARAM_VALUE.OffsetMulP_Gen { PARAM_VALUE.OffsetMulP_Gen } {
	# Procedure called to update OffsetMulP_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.OffsetMulP_Gen { PARAM_VALUE.OffsetMulP_Gen } {
	# Procedure called to validate OffsetMulP_Gen
	return true
}

proc update_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to update Sim_Gen when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Sim_Gen { PARAM_VALUE.Sim_Gen } {
	# Procedure called to validate Sim_Gen
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

proc update_MODELPARAM_VALUE.DriftMulP_Gen { MODELPARAM_VALUE.DriftMulP_Gen PARAM_VALUE.DriftMulP_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DriftMulP_Gen}] ${MODELPARAM_VALUE.DriftMulP_Gen}
}

proc update_MODELPARAM_VALUE.DriftDivP_Gen { MODELPARAM_VALUE.DriftDivP_Gen PARAM_VALUE.DriftDivP_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DriftDivP_Gen}] ${MODELPARAM_VALUE.DriftDivP_Gen}
}

proc update_MODELPARAM_VALUE.DriftMulI_Gen { MODELPARAM_VALUE.DriftMulI_Gen PARAM_VALUE.DriftMulI_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DriftMulI_Gen}] ${MODELPARAM_VALUE.DriftMulI_Gen}
}

proc update_MODELPARAM_VALUE.DriftDivI_Gen { MODELPARAM_VALUE.DriftDivI_Gen PARAM_VALUE.DriftDivI_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DriftDivI_Gen}] ${MODELPARAM_VALUE.DriftDivI_Gen}
}

proc update_MODELPARAM_VALUE.OffsetMulP_Gen { MODELPARAM_VALUE.OffsetMulP_Gen PARAM_VALUE.OffsetMulP_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OffsetMulP_Gen}] ${MODELPARAM_VALUE.OffsetMulP_Gen}
}

proc update_MODELPARAM_VALUE.OffsetDivP_Gen { MODELPARAM_VALUE.OffsetDivP_Gen PARAM_VALUE.OffsetDivP_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OffsetDivP_Gen}] ${MODELPARAM_VALUE.OffsetDivP_Gen}
}

proc update_MODELPARAM_VALUE.OffsetMulI_Gen { MODELPARAM_VALUE.OffsetMulI_Gen PARAM_VALUE.OffsetMulI_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OffsetMulI_Gen}] ${MODELPARAM_VALUE.OffsetMulI_Gen}
}

proc update_MODELPARAM_VALUE.OffsetDivI_Gen { MODELPARAM_VALUE.OffsetDivI_Gen PARAM_VALUE.OffsetDivI_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.OffsetDivI_Gen}] ${MODELPARAM_VALUE.OffsetDivI_Gen}
}

proc update_MODELPARAM_VALUE.Sim_Gen { MODELPARAM_VALUE.Sim_Gen PARAM_VALUE.Sim_Gen } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Sim_Gen}] ${MODELPARAM_VALUE.Sim_Gen}
}

