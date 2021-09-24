client_counts=(1 
	10 
	100 
	1000 
	10000 
	20000 
	35000 
	50000 
	75000 
	100000
	120000  
	130000 )
	#140000 
	#175000)
end_ips=("2401:db00:eef0:1120:3520:0:1401:eb14" 
	"2401:db00:eef0:1120:3520:0:1401:eb1e"
	"2401:db00:eef0:1120:3520:0:1401:eb78"
	"2401:db00:eef0:1120:3520:0:1401:eefc"
	"2401:db00:eef0:1120:3520:0:1402:1224"
	"2401:db00:eef0:1120:3520:0:1402:3934"
	"2401:db00:eef0:1120:3520:0:1402:73cc"
	"2401:db00:eef0:1120:3520:0:1402:ae64"
	"2401:db00:eef0:1120:3520:0:1403:100c"
	"2401:db00:eef0:1120:3520:0:1403:71b4"
	"2401:db00:eef0:1120:3520:0:1403:bfd4"
	"2401:db00:eef0:1120:3520:0:1403:e6e4"
	"2401:db00:eef0:1120:3520:0:1404:0df4"
	"2401:db00:eef0:1120:3520:0:1404:96ac")

client_durations=(5 8 12 16 22)



index=0
for count in ${client_counts[@]}; do
	echo "End ip ${end_ips[$index]} client_count ${client_counts[$index]}"

	for dur in ${client_durations[@]}; do
		echo "Duration $dur"

		file_name="${client_counts[$index]}_clients_${dur}_duration.log"
		echo "File $file_name"

		# substitute the IP address in 
		# hacky, using fixed line number to replace IP address, line 7
		ip_replace="        \"ClientIPEnd\": \"${end_ips[$index]}\","
		$(sed -i "7s/.*/$ip_replace/" clientgen_config.json)

		# substitute the duration
		#hacky, duration on line 12
		dur_replace="        \"DurationSec\": $dur,"
		$(sed -i "12s/.*/$dur_replace/" clientgen_config.json)

		# run clientgen and pipe results to file	
		./clientgen > $file_name

	done
	index=$(($index+1))
done

