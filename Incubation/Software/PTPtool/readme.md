Here is PTPtool to modify the ptp POSIX clock

to get PTPtool independently run the following commands
```
cd~
mkdir PTPtool
cd PTPtool
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Incubation/Software/PTPtool/Makefile
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Incubation/Software/PTPtool/ptptool.cpp
```
Use the following command for compiling:
```
make
```
or use the full command
```
gcc -Wall -lrt ptptool.c -o ptptool
```
optionally, to install
```
sudo mv ptptool /usr/bin/
```
