result=$(lspci | grep Meta | wc -l)
echo "Test Time Card Detect Start"
if [ "$result" -eq "1" ];
then
	echo "Test Time card Detect: Pass!"
else
	result=$(lspci)
	echo $result
	echo "Test Time card Detect: FAIL!!!"
	exit
fi

result=$(modprobe ptp_ocp)
result=$(dmesg | grep ptp_ocp | wc -l)
echo "Test Time Card Modprobe Start"
if [ "$result" -ge "7" ];
then
	echo "Test Time Card Modprobe: Pass!"
else
	result=$(dmesg | grep ptp_ocp)
	echo $result
	echo "Test Time Card Modprobe: FAIL!!"
	exit
fi

gnss_tty=$(dmesg | grep ptp_ocp | grep GNSS: | cut -d" " -f 10)
gnss_tty_count=$(echo $gnss_tty | wc -l)

echo "Test Time Card GNSS serial Start"
if [ "$gnss_tty_count" -eq "1" ];
then
	echo "Test Time Card GNSS serial: Pass!"
else
	result=$(dmesg | grep ptp_ocp)
	echo $result
	echo "Test Time Card GNSS serial: FAIL!!!"
	exit
fi


ubx_log=$(ubxtool -f $gnss_tty -s 115200 -p MON-VER | grep extension | wc -l)

echo "Test Time Card GNSS TimeLS Start"
if [ "$ubx_log" -ge "2" ];
then
	echo "Test Time Card GNSS TimeLS: Pass!"
else
	result=$(ubxtool -f $gnss_tty -s 115200 -p MON-VER)
	echo $result
	echo "Test Time Card GNSS TimeLS: FAIL!!!"
	exit
fi



echo "Time Card testing all passed!"
