**IMPORTANT**


**This is a preliminary (experimental) version of the TimeCard with the [LitePcie](https://github.com/enjoy-digital/litepcie) core as the PCIe to AXIMM bridge.**

**!!! The current driver does not support this version !!!**


The Version with LitePCIe has different Memory-Mapped Offsets (+ 0x0200_0000) as well as different MSI-Numbers (+ 32).



The LitePCIe Core supports MSI-X as well as PTM (with a correct driver).
More details about PTM and LitePCIe can be found here: https://github.com/enjoy-digital/litepcie_ptm_test

**PRELIMINARY DRIVER **

https://github.com/kevin-schaerer/Time-Appliance-Project/blob/aefa6490b97df89ece0de7bfebf71b46903dd040/Time-Card/DRV/ptp_ocp.c
