# MiniPTM design
A PCIe card with multiple timing features
* I225 as PCIe Bridge
* RJ45 for I225 as a normal Ethernet NIC
* M2 GNSS for PPS disciplining of I225 or DPLL based on BOM options
* Renesas 8A34001E DPLL
	* Connected to two SFP ports with diff pair clock out and clock in lines
	* Controlled by I225 through GPIO as I2C
	* DPLL supports Clock / 1PPS / TOD / User data over a single diff pair.
	* Intended to be used a cheap option for two way time and frequency transfer to high precision over fiber
 * SiT5501 as high stability oscillator for DPLL
