# UBX RCB-F9T GNSS module configurations

There are different kind of ublox modules available.
- ZED-F9T-00B-01
- ZED F9T-10B-01

Please check the exact version on the chip and use also the correct/latest UBX U-Center version to avoid compatibility issues.
In the subfolder are example base configurations available which should work with the Time-Card. They have the correct messages enabled and the correct settings for the Pulse Per Second. However, depending on the use-case there might be changes required.

# Required Messages
- NAV_TIME_UTC
- NAV_TIME_LS

# Optional Messages (for additional status information)
- NAV_STATUS
- MON_HW
- NAV_SAT

