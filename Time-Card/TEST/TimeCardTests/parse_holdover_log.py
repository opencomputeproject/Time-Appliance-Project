import csv
from datetime import datetime

filename="self_holdover_log.csv"
date_format = "%a %b %d %H:%M:%S %Z %Y"


csvfile = open(filename, newline='')
reader = csv.reader(csvfile)
headers = next(reader, None)


column = {}
date_key = ""
discipline_key = ""
tau_key = ""
phase_key = ""
phase_index = 0

for index,h in enumerate(headers):
    column[h] = []
    if ( h == "Date" ):
        date_key = h
    elif ( h == "Disciplining"):
        discipline_key = h
    elif ( h == "TauPps0"):
        tau_key = h
    elif ( h == "Phase"):
        phase_key = h
        phase_index = index

for row in reader:
    for h, v in zip(headers,row):
        column[h].append(v)
        
csvfile.close()

# here column is the whole parsed csv data as a dictionary based on keys



# find row index where disciplining changes
disc_change = [i for i in range(0,len(column[discipline_key])) if column[discipline_key][i]!=column[discipline_key][i-1]]
disc_change_values = [column[discipline_key][i] for i in disc_change]
disc_change_dates = [column[date_key][i] for i in disc_change]

# find row index where tau changes
tau_change = [i for i in range(0,len(column[tau_key])) if column[tau_key][i]!=column[tau_key][i-1]]
tau_change_values = [column[tau_key][i] for i in tau_change]
tau_change_dates = [datetime.strptime(column[date_key][i], date_format) for i in tau_change]
tau_durations = [ (tau_change_dates[i]-tau_change_dates[i-1]).total_seconds() for i in range(1, len(tau_change_dates)) ]

#include the last range of time for the last tau, how long it was at last tau
tau_durations.append( (datetime.strptime(column[date_key][-1],date_format) - tau_change_dates[-1]).total_seconds() )

#print(tau_change_values)
#print(tau_change_dates)
#print(tau_durations)



change_times = list(set(disc_change + tau_change))
change_times.sort()
print(change_times)
for count,val in enumerate(change_times):
    # add 1 so you can just go to the line in the csv file
    print(f"{count}, index {change_times[count]+2}: Tau={column[tau_key][val]} , Disciplining={column[discipline_key][val]}")
    disc = int(column[discipline_key][val])
    #phase is only relevant when not in disciplining mode!

    if ( count != len(change_times)-1 ):
        start = datetime.strptime(column[date_key][change_times[count+1]], date_format)
        end = datetime.strptime(column[date_key][change_times[count]], date_format)
        first_nonzero_phase = next((value for value in column[phase_key][change_times[count]:change_times[count+1]] if float(value) != 0), None)
        last_nonzero_phase = next((value for value in reversed(column[phase_key][change_times[count]:change_times[count+1]]) if float(value) != 0), None)
        if ( disc == 0 ):
            print(f"         Start phase at {column[date_key][val]} = {first_nonzero_phase}")
            print(f"         End phase at {column[date_key][change_times[count+1]]} = {last_nonzero_phase}")
    else:
        end = datetime.strptime(column[date_key][change_times[count]], date_format)
        start = datetime.strptime(column[date_key][-1], date_format)
        first_nonzero_phase = next((value for value in column[phase_key][change_times[count]:-1] if float(value) != 0.0), None)
        last_nonzero_phase = next((value for value in reversed(column[phase_key][change_times[count]:-1]) if float(value) != 0.0), None)
        if ( disc == 0 ):
            print(f"         Start phase at {column[date_key][val]} = {first_nonzero_phase}")
            print(f"         End phase at {column[date_key][-1]} = {last_nonzero_phase}") #not exact but close enough


    duration = start - end

    print(f"         Duration: {round(duration.total_seconds(),0)} seconds, approx {round(duration.total_seconds()/3600,3)} hours")


exit()

# Print stuff with respect to Tau changing
for num,i in enumerate(tau_change_values):
    print(f"Num: Tau={i} for {tau_durations[num]} seconds")
    print(f"    Start phase at {tau_change_dates[num]}: {column[phase_key][tau_change[num]]}") 
    if num == len(tau_change_values)-1:
        # last entry, print last phase
        print(f"    End phase at {column[date_key][-1]}: {column[phase_key][-1]}")
        pass
    else:
        print(f"    End phase at {tau_change_dates[num+1]}: {column[phase_key][tau_change[num+1]]}")








