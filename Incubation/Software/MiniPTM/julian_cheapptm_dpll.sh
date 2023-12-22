i2c_bus=""

init_dpll_access() {
	# setup i2c1
	#ignore=$(raspi-gpio set 2,3 a0)
	# try to read from mux on dpll board
	val=$(i2cget -f -y $i2c_bus 0x70)

	# validate it comes back 0x0, add later

	# setup i2c mux to access DPLL
	ignore=$(i2cset -f -y $i2c_bus 0x70 0x8)

	# now should be able to talk to DPLL
	val=$(i2cget -f -y $i2c_bus 0x58 0x0)
}


####### Interact with DPLL in one byte i2c mode, since that's the default upon power up
# two byte i2c mode more efficient, can change to that later if running into throughput

# cache current base addr written to DPLL to save i2c transactions
cur_base_addr=""

# $1 is module base address, format 0xc000 , two byte value
# $2 is register address, format 0x00 , one byte
# $3 is register value, format 0x00, one byte
write_single_dpll_reg() {
	full_addr=""
	baseaddr_lower=""
	baseaddr_upper=""
	if [ "$cur_base_addr" = "$1" ]; then
		#echo "Skipping rewrite baseaddr"
		full_addr=$(($1 + $2))
		baseaddr_lower=$(printf "0x%x" $(($full_addr & 0xff)))
		baseaddr_upper=$(printf "0x%x" $(( ($full_addr & 0xff00) >> 8)) )
	else
		full_addr=$(($1 + $2))
		baseaddr_lower=$(printf "0x%x" $(($full_addr & 0xff)))
		baseaddr_upper=$(printf "0x%x" $(( ($full_addr & 0xff00) >> 8)) )
		val=$(i2ctransfer -f -y $i2c_bus w5@0x58 0xfc $baseaddr_lower $baseaddr_upper 0x10 0x20)
		cur_base_addr=$1
	fi	
	val=$(i2ctransfer -f -y $i2c_bus w2@0x58 $baseaddr_lower $3)
       	echo "Write DPLL register, module $1, addr $2 = $3"	
}

# $1 is module base address, format 0xc000 , two byte value
# $2 is register address, format 0x00 , one byte
read_single_dpll_reg() {
	full_addr=""
	baseaddr_lower=""
	baseaddr_upper=""
	if [ "$cur_base_addr" = "$1" ]; then
		#echo "Skipping rewrite baseaddr"
		full_addr=$(($1 + $2))
		baseaddr_lower=$(printf "0x%x" $(($full_addr & 0xff)))
		baseaddr_upper=$(printf "0x%x" $(( ($full_addr & 0xff00) >> 8)) )
	else
		full_addr=$(($1 + $2))
		baseaddr_lower=$(printf "0x%x" $(($full_addr & 0xff)))
		baseaddr_upper=$(printf "0x%x" $(( ($full_addr & 0xff00) >> 8)) )
		val=$(i2ctransfer -f -y $i2c_bus w5@0x58 0xfc $baseaddr_lower $baseaddr_upper 0x10 0x20)
		cur_base_addr=$1
	fi	
	val=$(printf "0x%x" $(i2ctransfer -f -y $i2c_bus w1@0x58 $baseaddr_lower r1) )
	echo $val
	#echo "Read DPLL register, module $1, reg $2 = $val"	
}

# $1 is mode for sma1, either "input" or "output"
set_sma1_mode() {
	# DPLL GPIO 0
	if [ "$1" = "input" ];
	then
		echo "Set SMA1 to input"
	else
		echo "Set SMA1 to output"
	fi

	# set GPIO as 3.3V
	write_single_dpll_reg 0xc8c0 0x0 0x1

	# set GPIO_FUNCTION to disabled, bit 0=0 
	# set gpio_control_dir to output, bit 2=1
	# set gpio_cmos_mode to cmos, bit 1=0
	write_single_dpll_reg 0xc8c2 0x10 0x4

	# set output level , bit 0, read modify write
	val=$(read_single_dpll_reg 0xc160 0x0)
	#echo "GPIO0-7 drive level register = $val"
	if [ "$1" = "input" ];
	then
		val=$(printf "0x%x" $(($val | 0x1)))
	else
		val=$(printf "0x%x" $(($val & 0xfe)))
	fi
	write_single_dpll_reg 0xc160 0x0 $val

}

# $1 is mode for sma2, either "input" or "output"
set_sma2_mode() {
	# DPLL GPIO 1
	if [ "$1" = "input" ];
	then
		echo "Set SMA2 to input"
	else
		echo "Set SMA2 to output"
	fi

	# set GPIO as 3.3V
	write_single_dpll_reg 0xc8c0 0x0 0x1

	# set GPIO_FUNCTION to disabled, bit 0=0 
	# set gpio_control_dir to output, bit 2=1
	# set gpio_cmos_mode to cmos, bit 1=0
	write_single_dpll_reg 0xc8d4 0x10 0x4

	# set output level , bit 0, read modify write
	val=$(read_single_dpll_reg 0xc160 0x0)
	#echo "GPIO0-7 drive level register = $val"
	if [ "$1" = "input" ];
	then
		val=$(printf "0x%x" $(($val | 0x2)))
	else
		val=$(printf "0x%x" $(($val & 0xfd)))
	fi
	write_single_dpll_reg 0xc160 0x0 $val
}


read_clock_input_statuses() {
	# status block, starts page 30, base addr 0xc03c
	for i in {0..15..1}
	do
		offset=$(printf "0x%x" $(($i + 8)) ) # register address for input i mon status
		val=$(read_single_dpll_reg 0xc03c $offset)
		echo "----DPLL IN$i Monitor Status = $val-----"
		echo "  1. LOS Live = $(( ($val & 0x1) ))" 
		echo "  2. Activity = $(( ($val & 0x2) >> 1))"
	done
}

read_tod_info() {
	for i in {0..3..1}
	do
		:
		#offset=$(printf "0x%x" $(($i + 
	done
}

# $1 is sfp to read from, either 1 or 2
read_sfp_status() {
	# need to change mux
	if [ "$1" -eq 1 ]; then
		ignore=$(i2cset -f -y $i2c_bus 0x70 0x1)
	else
		ignore=$(i2cset -f -y $i2c_bus 0x70 0x2)
	fi

	# try to read from SFP, don't care value 
	val=$(i2cget -f -y $i2c_bus 0x50 0x0 2>1 | grep "0x" | wc -l)
	if [ "$val" -eq 0 ]; then
		echo "SFP1 not inserted!"
		# set it back to DPLL access
		ignore=$(i2cset -f -y $i2c_bus 0x70 0x8)
		return
	fi	
	# read back vendor name
	val=$(i2ctransfer -f -y $i2c_bus w1@0x50 0x14 r16)

	stringarray=($val)
	# print the characters one by one until 0x20
	echo -n "Vendor name:"
	for letter in "${stringarray[@]}"; do
		if [ "$letter" = "0x20" ]; then
			:
		else
			temp=$(printf %x $(($letter)))
			echo -n -e "\x$temp" 
		fi
	done
	echo ""
	# set it back to DPLL access
	ignore=$(i2cset -f -y $i2c_bus 0x70 0x8)

	return 

	#### COME BACK TO THIS IF INTERESTED LATER
	# read back connection type 
	val=$(i2ctransfer -f -y $i2c_bus w1@0x50 0x9 r1)
	echo -n "Transmission media: $val"
	if [ "$val" -eq "0x80" ]; then
		echo "Twin Axial Pair"
	elif ["$val" -eq "0x40" ]; then
		echo "Twisted pair"
	elif ["$val" -eq "0x20" ]; then
		echo "Miniature Coax"
	elif ["$val" -eq "0x10" ]; then
		echo "Video Coax"
	elif ["$val" -eq "0x8" ]; then
		echo "Multimode 62.5um"
	elif ["$val" -eq "0x4" ]; then
		echo "Multimode 50um"
	elif ["$val" -eq "0x1" ]; then
		echo "Single mode"
	fi
}









# $1 is the file, a .tcs file
verify_config_file_tcs() {
	ignore=$(cat $1 | grep -A99999999 -m1 -e 'Page.Byte' | tail -n+2 | sed '/Data Fields/q' | head -n -2 > parse_file.txt)
	count=0
	while IFS= read -r line
	do
		echo "$line"
		addr=""
		val=""
		for word in $line
		do
			#echo $word
			# 2 characters, hex string for value, like a5
			# 5 characters, address string, like C0.05
			char_count=$(echo -n $word | wc -c)
			if [ "$char_count" -eq "2" ]; then
				#echo " Val = $word"
				val=$word
			elif [ "$char_count" -eq "5" ]; then
				addr=${word:0:2}${word:3:4}
				#echo " Addr = 0x$addr"
			fi 	
		done
		addr_len=$(echo -n $addr | wc -c)
		val_len=$(echo -n $val | wc -c)
		if [ "$addr_len" -eq "4" ] && [ "$val_len" -eq "2" ]; then
			#echo "Address 0x$addr = 0x$val"
			read_val=$(read_single_dpll_reg "0x$addr" "0x00")
			
			#echo "Read Addr = 0x$addr , Read_val = $read_val, val = $val"
			if [ "$(($read_val))" -eq "$((0x$val))" ];
			then
				#echo "Verified 0x$addr = $read_val"
				echo -n ""
			else
				echo "Address 0x$addr read $read_val, expected 0x$val"
			fi
		fi

	done < "parse_file.txt"


	ignore=$(rm -f parse_file.txt)
}


# $1 is the register address as a string, like 0xC031
# $2 is the data as a string, could be multiple bytes, like 000001111
write_config_file_line_dpll() {
	cur_addr=$1
	data=$2
	data_len=$(echo -n $data | wc -c)

	# do single register writes, less efficient but more portable
	while [ "$data_len" -gt "2" ];
	do
		cur_data=$(echo $data | head -c 2)
		write_single_dpll_reg "$cur_addr" "0x00" "0x$cur_data"

		# handle the loop, cut down the data string
		data=$(echo $data | cut -c 3-)  
		data_len=$(echo -n $data | wc -c)
		cur_addr=$(printf "0x%x" $(($cur_addr+1)))

	done
}

# $1 is the file , an exported file from Timing Commander, Format=Generic, Filter=All, Protocol=i2c, AddressType=Two Byte addresses
write_config_file() {
	ignore=$(cat $1 | grep Offset > parse_file.txt)
	count=0
	while IFS= read -r line
	do
		echo "$line"
		found_offset=0
		found_data=0
		offset_str=""
		for word in $line
		do
			#echo $word
			
			if [ "$found_offset" -eq "1" ]; then
				#echo "This is offset!"
				found_offset=0
				#offset string is a hex string like C031,
				offset_str=$(echo "0x$word" | head -c 6)
				#echo "Offset val = $offset_str"
			fi
			if [ "$found_data" -eq "1" ]; then
				#echo "This is data!"
				data=$(echo $word | cut -c 3-)
				#echo "Data = $data"
				found_data=0
				write_config_file_line_dpll $offset_str $data
			fi

			if [ "$word" = "Offset:" ]; then
				#echo "Found offset!"
				found_offset=1
				found_data=0
			elif [ "$word" = "Data:" ]; then
				#echo "Found data!"
				found_offset=0
				found_data=1
			fi


		done
	done < "parse_file.txt"
	ignore=$(rm -f parse_file.txt)
}


# $1 is the file, a .tcs file , DONT USE THIS METHOD
write_config_file_tcs() {
	ignore=$(cat $1 | grep -A99999999 -m1 -e 'Page.Byte' | tail -n+2 | sed '/Data Fields/q' | head -n -2 > parse_file.txt)
	count=0
	while IFS= read -r line
	do
		echo "$line"
		addr=""
		val=""
		for word in $line
		do
			#echo $word
			# 2 characters, hex string for value, like a5
			# 5 characters, address string, like C0.05
			char_count=$(echo -n $word | wc -c)
			if [ "$char_count" -eq "2" ]; then
				#echo " Val = $word"
				val=$word
			elif [ "$char_count" -eq "5" ]; then
				addr=${word:0:2}${word:3:4}
				#echo " Addr = 0x$addr"
			fi 	
		done
		addr_len=$(echo -n $addr | wc -c)
		val_len=$(echo -n $val | wc -c)
		if [ "$addr_len" -eq "4" ] && [ "$val_len" -eq "2" ]; then
			echo "Write Address 0x$addr = 0x$val"
			#write_single_dpll_reg "0x$addr" "0x0" "0x$val"
		fi
	done < "parse_file.txt"

	ignore=$(rm -f parse_file.txt)
}


######## OLD VERSION
# $1 is the file, a .tcs file
#write_config_file() {
#	ignore=$(sed -n '/Page\.Byte#/,$p' "$1" | sed '/Data/q' | head -n -2 | tail -n +2 > parse_file.txt)
#	count=0
#	while read -r line
#	do
#		#echo "$count: $line"
#		IFS=" "
#		read -a strarr <<< "$line"
#		#${strarr[0]} = register as a string, like CF.80
#		# ${strarr[2]} = value as a string as hex, like a5
#		upper=$(echo ${strarr[0]} | cut -d"." -f 1)
#		lower=$(echo ${strarr[0]} | cut -d"." -f 2)
#		value=$(echo "0x${strarr[2]}")	
#		echo "Count $count, Write 0x${upper}${lower} = $value"	
#		write_single_dpll_reg 0x${upper}${lower} 0x00 $value
#		temp_val=$(read_single_dpll_reg 0x${upper}${lower} 0x0)
#		echo "0x${upper}${lower} = $temp_val"
#		count=$(($count + 1))
#	done < parse_file.txt
#	rm -f parse_file.txt
#}


dpll_input_base=("0xc1b0" "0xc1d0"
	"0xc200" "0xc210" "0xc220"
	"0xc230" "0xc240" "0xc250"
	"0xc620" "0xc280" "0xc290"
	"0xc2a0" "0xc2b0" "0xc2c0"
	"0xc2d0"
)


read_dpll_status() {
	# figure out which clock inputs are configured to be used
	# of the inputs that are used, print their expected input frequencies, and if they see signal
	echo ""
}




if [ "$#" -ge 2 ]; then

	echo "Dpll_program , bus number $1, action $2, numargs=$#"
	i2c_bus="$1"
	init_dpll_access
	if [ "$2" = "program" ]; then
		echo "Programming config file $3!"
		write_config_file  "$3"
	elif [ "$2" = "verify" ]; then
		echo "Verifying config file!"
		verify_config_file  "$3"
	elif [ "$2" = "status" ]; then
		echo "Reading DPLL input status"
		read_clock_input_statuses
	elif [ "$2" = "sma1" ]; then
		echo "Setting SMA1 to $3"
		set_sma1_mode $3
	elif [ "$2" = "sma2" ]; then
		echo "Setting SMA2 to $3"
		set_sma2_mode $3
	elif [ "$2" = "sfp1" ]; then
		echo "Read SFP1 info"
		read_sfp_status 1
	elif [ "$2" = "sfp2" ]; then
		echo "Read SFP2 info"
		read_sfp_status 2
	elif [ "$2" = "write" ]; then
		if [ "$#" -ne 5 ]; then
			echo "Write register needs 5 arguments!"
			exit
		fi
		echo "Write register base $3 offset $4 = $5"
		write_single_dpll_reg "$3" "$4" "$5"
		
	elif [ "$2" = "read" ]; then
		if [ "$#" -ne 4 ]; then
			echo "Read register needs 4 arguments!"
			exit
		fi
		echo -n "Read register base $3 offset $4 = "
		read_single_dpll_reg "$3" "$4"
	elif [ "$2" = "writeb" ]; then
		if [ "$#" -ne 5 ]; then
			echo "Write register needs 5 arguments!"
			exit
		fi
		i2c_bus="1"
		init_dpll_access
		echo "Bus 1 Write register base $3 offset $4 = $5"
		write_single_dpll_reg "$3" "$4" "$5"

		i2c_bus="22"
		init_dpll_access
		echo "Bus 22 Write register base $3 offset $4 = $5"
		write_single_dpll_reg "$3" "$4" "$5"
		
	elif [ "$2" = "readb" ]; then
		if [ "$#" -ne 4 ]; then
			echo "Read register needs 4 arguments!"
			exit
		fi
		i2c_bus="1"
		init_dpll_access
		echo -n "Bus 1 Read register base $3 offset $4 = "
		read_single_dpll_reg "$3" "$4"

		i2c_bus="22"
		init_dpll_access
		echo -n "Bus 22 Read register base $3 offset $4 = "
		read_single_dpll_reg "$3" "$4"

	fi	

	exit
else
	echo "Not enough arguments"
	echo "dpll_program.sh <i2c bus number> <action> <optional action argument>"
	echo "Actions:"
	echo "	1. program , writes tcs config file"
	echo "	2. verify , writes tcs config file"
	echo "  3. status , reads DPLL input status"
	echo "  4. sma1 , control sma1 mode, optional argument either output or input"
	echo "  5. sma2 , control sma2 mode, optional argument either output or input"
	echo "  6. sfp1 , read sfp1 info"
	echo "  7. sfp2 , read sfp2 info"
	echo "  8. write, write register, <0xregaddr> <0xval>"
	echo "  9. read, read register, <0xregbase> <0xregoffset>"
	echo "  10. writeb, write register to i2c buses 1 and 22, <0xregbase> <0xregoffset> <0xval>"
	echo "  11. readb, read register from i2c buses 1 and 22, <0xregbase> <0xregoffset>"
	echo "Exiting"
	exit
fi

exit

#scratch test
#write_single_dpll_reg 0xcf50 0x0 0xa5
#write_single_dpll_reg 0xcf50 0x1 0x5a
#write_single_dpll_reg 0xcf50 0x2 0xde

#read_single_dpll_reg 0xcf50 0x0  
#read_single_dpll_reg 0xcf50 0x1 
#read_single_dpll_reg 0xcf50 0x2  
