:: found options with C:\Users\julianstj\Desktop\WorkNotes\Software\arduino-cli board details -b arduino:mbed_giga:giga with it plugged in
C:\Users\julianstj\Desktop\WorkNotes\Software\arduino-cli.exe compile RCB_WWVB\RCB_WWVB.ino -b arduino:mbed_giga:giga --board-options split=50_50,target_core=cm7
C:\Users\julianstj\Desktop\WorkNotes\Software\arduino-cli.exe upload RCB_WWVB -b arduino:mbed_giga:giga --port COM14 --board-options split=50_50,target_core=cm7
C:\Users\julianstj\Desktop\WorkNotes\Software\arduino-cli.exe upload RCB_WWVB -b arduino:mbed_giga:giga --port COM15 --board-options split=50_50,target_core=cm7
:: run tera term, program waits for serial to open to start
"C:\Program Files (x86)\teraterm\ttermpro.exe" /C=14