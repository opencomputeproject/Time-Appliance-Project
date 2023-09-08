##################
# Tests designed for SA53 Time Cards wihout oscillatord


oscpy="python3 ../../OSC/Microchip/SA53/timetickler.py"


read_SA53_int_value() {
	#command is $1
	val=$($oscpy get $1 | cut -d"=" -f 2 | sed 's/.$//')
	echo $(($val))
}

SA53_start_disc() {
	echo "Start disciplining"
	val=$($oscpy set PhaseMetering 0)
	# Phase Limit can have big impact 
	val=$($oscpy set PhaseLimit 20000)
	val=$($oscpy set Disciplining 1)
}

SA53_stop_disc() {
	echo "Stop disciplining"
	val=$($oscpy set Disciplining 0)
	val=$($oscpy set PhaseMetering 1)
}

SA53_get_phase() {
	val=$($oscpy get Phase | cut -d"=" -f 2 | sed 's/.$//')
	echo $val
}

file="self_holdover_log.csv"
read_SA53_status() {
	disc=$(read_SA53_int_value Disciplining)
	tau=$(read_SA53_int_value TauPps0)
	tuning=$(read_SA53_int_value DigitalTuning)
	ppsin=$(read_SA53_int_value PpsInDetected)
	phase=$(SA53_get_phase)
	efftuning=$(read_SA53_int_value EffectiveTuning) 
	echo "Disciplining = $disc"
	echo "TauPps0 = $tau"
	echo "DigitalTuning = $tuning"
	echo "PPS In Detected = $ppsin"
	echo "Phase = $phase"
	echo "EffectiveTuning = $efftuning"
	echo "$(date),$disc,$tau,$tuning,$efftuning,$ppsin,$phase" >> $file
}

timed_read_sa53_status() {
	#$1 is time to read status for in seconds
	echo "Reading SA53 status for $1 seconds"
	total_duration=$(($1))
	for (( i = 0; i <= $total_duration; i++ ))
	do
		if [ $(($i%5)) -eq 0 ];
		then
			read_SA53_status
		fi
		sleep 1
	done
	echo "Done reading SA53 status"
}

write_csv_header() {
	echo "Date,Disciplining,TauPps0,DigitalTuning,EffectiveTuning,PPS In Detected,Phase" > $file
}


sa53_holdover_test() {
	# $1 is how many seconds to run holdover for
	echo "Running holdover test for $1 seconds"
	total_duration=$(($1))

	#start holdover
	SA53_stop_disc

	timed_read_sa53_status $1

	echo "Holdover test done!"
	read_SA53_status
	SA53_start_disc
}

#read_SA53_status
#sa53_holdover_test "100"


########################################
# define self holdover tests with some parameters
# 1. Tau for disciplining period
# 2. Length of disciplining period
# 3. Length of holdover test
tau_tests=(50 100 200 300)
disc_length=($((50*5)) $((100*5)) $((200*5)) $((300*5)))
holdover_length=($((50*3)) $((100*3)) $((200*3)) $((300*3)))





########################################
# different test, trying to optimize steps to converge
# after GPS is lost
# hard code it for now think of logic later


degrade_sa53_method1() {
	######## Degrade method 1
	# disable disciplining, force max digitaltuning
	# will work eventually but not that quickly

	# 1. Disable disciplining
	SA53_stop_disc
	# 2. Force a bad DigitalTuning 
	echo "Purposefully degrading SA53 disciplining method 1"
	for (( i = 0; i <= 30; i++ ))
	do
		val=$($oscpy set DigitalTuning 20000000)
		val=$($oscpy set latch 1)
		echo "Degrade digital tuning $i"
	done

	# 3. Wait for Phase to exceed some value, units are nanoseconds
	threshold=120000.0
	max_wait_time=8000
	i=0
	while true; 
	do
		phase=$(SA53_get_phase)
		abs_val=$(echo "if ($phase < 0) -1*$phase else $phase" | bc)
		comparison_result=$(echo "$abs_val > $threshold" | bc)
		read_SA53_status
		if [ "$comparison_result" -eq 1 ]; then 
			echo "Phase = $phase, degraded enough, threshold $threshold"
			break
		else
			echo "Phase = $phase, not degraded enough, threshold $threshold"
		fi
		if [ "$i" -ge $max_wait_time ]; then
			echo "Max time exceeded waiting for degrade"
			break
		else
			echo "$i seconds elapsed waiting for degrade"
			sleep 5
		fi
		((i=i+5))
	done
	echo "Done degrading SA53"
}



fix_degrade_sa53_method1() {
	######## Degrade method 1
	# disable disciplining, force max digitaltuning
	# will work eventually but not that quickly

	# 1. Disable disciplining
	SA53_stop_disc
	# 2. Force a bad DigitalTuning 
	echo "Purposefully degrading SA53 disciplining method 1"
	for (( i = 0; i <= 30; i++ ))
	do
		val=$($oscpy set DigitalTuning -20000000)
		val=$($oscpy set latch 1)
		echo "Fix Degrade digital tuning $i"
	done
	SA53_start_disc

	echo "Done fix degrading SA53"
}


degrade_sa53_method2() {
	echo "Purposefully degrading SA53 disciplining method 2"
	# I think this method isn't as good. It will degrade the phase
	# but the frequency will still be disciplined by 1pps input 
	####### Degrade method 2
	# enable disciplining with small tau, 
	# but set cableDelay to make it calibrate to the wrong value
	# 1. enable disciplining with small tau and large cable delay
	SA53_start_disc
	val=$($oscpy set TauPps0 50)
	val=$($oscpy set CableDelay 200000)
	echo "Setting bad cabledelay to force SA53 to discipline to wrong value"
	# 2. Wait for it to discipline to what it thinks is a small phase
	max_wait_time=8000
	i=0
	threshold=50
	while true;
	do
		phase=$(SA53_get_phase)
		abs_val=$(echo "if ($phase < 0) -1*$phase else $phase" | bc)
		comparison_result=$(echo "$abs_val < $threshold" | bc)
		if [ "$comparison_result" -eq 1 ]; then 
			echo "Phase = $phase, degraded enough"
			break
		else
			echo "Phase = $phase, not degraded enough"
		fi
		if [ "$i" -ge $max_wait_time ]; then
			echo "Max time exceeded waiting for degrade"
			break
		else
			echo "$i seconds elapsed waiting for degrade"
			sleep 5
		fi
		((i=i+5))

	done	
}

short_read_phase() {
	# turn off disciplining for a very short time just to sample phase
	SA53_stop_disc
	timed_read_sa53_status 5
	SA53_start_disc

}

fix_test() {
	fix_degrade_sa53_method1
}

converge_test() {
	echo "Starting converge test!"
	write_csv_header

	read_SA53_status
	degrade_sa53_method1
	read_SA53_status

	# Phase has degraded to threshold, test different methods
	# of recovering it

	# 1. 600 seconds at tau=50
	# 1. 7200 seconds at Tau=500
	# 2. 5 Tau at Tau=10000
	val=$($oscpy set TauPps0 50)
	SA53_start_disc
	timed_read_sa53_status 600


	val=$($oscpy set TauPps0 500)
	timed_read_sa53_status 7200


	val=$($oscpy set TauPps0 10000)
	timed_read_sa53_status 50000
	sa53_holdover_test 86400

	####### This test completed 8-31-2023, passed with around 540ns
	# after changing phase limit to 20uS
	# 1. 600 seconds at tau=50
	# 1. 7200 seconds at Tau=500
	# 2. 10 Tau at Tau=10000
	return 0

	# Theory 2: 
	# 1. 600 seconds at Tau=50
	# 2. 4 Tau at Tau=10000
	val=$($oscpy set TauPps0 50)
	SA53_start_disc
	timed_read_sa53_status 600
	val=$($oscpy set TauPps0 10000)
	timed_read_sa53_status 40000
	sa53_holdover_test 86400


	return 0

	# Just monitor how long until it stabilizes
	val=$($oscpy set TauPps0 7000)
	SA53_start_disc
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 140000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase
	timed_read_sa53_status 14000
	short_read_phase

	


	return 0

	# THEORY: Leave tau at 7000 for 20 tau
	val=$($oscpy set TauPps0 7000)
	SA53_start_disc
	timed_read_sa53_status 140000
	sa53_holdover_test 86400

	return 0

	# Theory 2: 
	# 1. 2000 seconds at Tau=50
	# 2. 10 Tau at Tau=10000
	val=$($oscpy set TauPps0 50)
	SA53_start_disc
	timed_read_sa53_status 2000
	val=$($oscpy set TauPps0 10000)
	timed_read_sa53_status 100000
	#sa53_holdover_test 86400

	return 0

	# Theory 1: Leave tau at 10000 for 10 tau
	val=$($oscpy set TauPps0 10000)
	SA53_start_disc
	timed_read_sa53_status 100000
	sa53_holdover_test 86400




	# RESET BETWEEN TESTS
	read_SA53_status
	degrade_sa53_method1
	read_SA53_status

	# Theory 3:
	# 1. 7200 seconds at Tau=500
	# 2. 10 Tau at Tau=10000
	val=$($oscpy set TauPps0 500)
	SA53_start_disc
	timed_read_sa53_status 7200
	val=$($oscpy set TauPps0 10000)
	timed_read_sa53_status 100000
	sa53_holdover_test 86400


	return 0













	# Default oscillatord algorithm as of 5/15/2023 -> Tested, not that great!
	# 1. 600 seconds at Tau=50
	# 2. 7200 seconds at Tau=500
	# 3. 86400 seconds at Tau=10000
	val=$($oscpy set TauPps0 50)
	SA53_start_disc
	timed_read_sa53_status 600
	val=$($oscpy set TauPps0 500)
	timed_read_sa53_status 7200
	val=$($oscpy set TauPps0 10000)
	timed_read_sa53_status 86400
	
	sa53_holdover_test 86400

	return 0

	# Proposed Algorithm 1:
	# 1. If Abs(Phase) > 100uS , Go down to Tau=50 for 600 seconds
	# 2. Then go to Tau=10000 for 86400 seconds
	# 3. Holdover test

	#assume Phase is large, covered by degrade code path above
	#val=$($oscpy set TauPps0 50)
	#SA53_start_disc

	#timed_read_sa53_status 600
	#val=$($oscpy set TauPps0 10000)
	#timed_read_sa53_status 86400

	#sa53_holdover_test 86400

}


#fix_test

#degrade_sa53_method1


converge_test
