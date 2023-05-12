# -*- coding: utf-8 -*-
"""
Created on Thu Oct 03 13:38:08 2019

@author: PMinciu
This file reads an AD9546 json file, extracts the value of the configuration 
registers, writes them into the AD9546  and then starts it
State of board jumpers:
  - P508 jumpers on Communicate via USB side
  - P507 jumpers on Communicate via USB side
At the end, it reads back the downlaoded registers and if they do not match the
    target value, it visualizes them
  - register 0xD40 is a status register, so it's normal to be outlined
  - registers 0x2C04 to 0x2C07 are not used by the AD9546
"""


import sys, struct
import xml.etree.ElementTree as ET
#from bitarray import bitarray as BitArray

import os


import sys, time


#this is the procedure to read the json file and extract the register values
for fn in ['TimeDrive_AD9546_RX.json']:


    # Read specified input configuration file
    raw     = [x.strip('\n').strip('\r') for x in open(fn,'r').readlines()]
    address = []
    data    = []
    reg_value = []
    fn_input = fn.split(".")[0]
    for index_raw in range(2072, 3571):
       addr = int(raw[index_raw][5:11],16)
       value= int(raw[index_raw][15:19],16)
       address.append(addr)
       reg_value.append(value)
       #this is a combination between addresses and register values
       data.append((addr, value))

    #save all the registers values into a file
    filen = fn.replace("json", "h")
    outfile = open(filen, "w")


    outfile.write("const PROGMEM uint16_t %s_dpll_reg_count = 1354;\n" %  fn_input )
    outfile.write("const PROGMEM uint16_t %s_dpll_regs[] = {" % fn_input)

    for i in range(0, 1354, 1):
      outfile.write("0x%x," % address[i])

    outfile.write("};\n")

    outfile.write("const PROGMEM uint8_t %s_dpll_vals[] = {" % fn_input)

    for i in range(0, 1354, 1):
      outfile.write("0x%x," % reg_value[i])

    outfile.write("};")

quit()
#  _str += struct.pack("6s", str(hex(address[i])))
#  _str += struct.pack("4s", "    ")
#  _str += struct.pack("4s", str(hex(reg_value[i])))
#  _str += struct.pack("c", "\n")
#f = open(root + filen.split('.')[0] + '.txt', 'w')
#f.writelines([_str])
#f.close()
#pass

#read all registers that have been written and verify their values coincide with what was written
#if the read back value is not equal to the value we wanted to write, print it
#for index in range (0,1354):
#  read_reg_value=d.read(address[index])      #read the register back
#  if int(read_reg_value) != data[index][1]:    
#    print ('index: ',index, 'reg addr: ', hex(address[index]), 'reg value: ', 
#           hex(read_reg_value), 'target reg value: ', hex(data[index][1]))






