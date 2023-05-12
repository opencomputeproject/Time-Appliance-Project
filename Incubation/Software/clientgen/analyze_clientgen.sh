

echo "Client count, client duration (seconds), max time between syncs, time spent with all grants grants, time spent sending grants" > clientgen_analysis.csv


for f in *clients*duration.log; do
	client_count=$(echo $f | cut -d"_" -f 1)
	duration=$(echo $f | cut -d"_" -f 3)
	echo "Processing $f file.. client_count=$client_count duration=$duration "




	maxtime_raw=$(cat $f | grep "Time Between Syncs" -A 15 | grep "Max:")
	max_val="0.0"
	while IFS= read -r line; do
		value=$(echo $line | cut -d" " -f 2 | cut -d"s" -f 1)
		#echo "$value"
		#echo "... Value = $(echo $line | cut -d" " -f 2) ..."
		if (( $(echo "$value > $max_val" | bc -l) )); then
			max_val=$value
		fi
	done <<< "$maxtime_raw"
	echo "Found max Time Between Syncs value $max_val"


	grantime_raw_done=$(cat $f | grep "TotalClientDelayRespGrant" | grep "rate 0" | wc -l)	
	grantime_raw_done=$(($grantime_raw_done-1))

	grantime_raw_notdone=$(cat $f | grep "TotalClientDelayRespGrant" | grep -v "rate 0" | wc -l)	
	echo "Found $grantime_raw_done intervals that didn't send grants, $grantime_raw_notdone intervals did send grants"
	echo ""

	echo "$client_count,$duration,$max_val,$grantime_raw_done,$grantime_raw_notdone" >> clientgen_analysis.csv

	#echo $maxtime_raw
done
