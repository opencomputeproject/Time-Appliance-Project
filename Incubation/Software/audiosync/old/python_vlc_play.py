import time, vlc
import sys

print("Start python synchronized play")

#print(f"Got arg count {len(sys.argv)} args {sys.argv}")
#if len(sys.argv) > 1:
#    time_arg = sys.argv[1]
#    parsed = time.strptime(time_arg, "%H:%M:%S")
#    print(f"Starting play back at time {parsed}")

# gets the time in ns
now = time.clock_gettime_ns(time.CLOCK_REALTIME)

# get the time in seconds
now_sec = time.clock_gettime(time.CLOCK_REALTIME)

# get second in range 0 to 59
floor_minute = int(now_sec % 60)
    

#print(f"Got time now {now/1000000000} now_sec {now_sec} floor_minute {floor_minute}")


source = "Beethoven - Moonlight Sonata (FULL).mp3"

vlc_instance = vlc.Instance()

player = vlc_instance.media_player_new()

media = vlc_instance.media_new(source)

player.set_media(media)




##############
# wait for second to tick to zero, hacky way, sync on the minute
while 1:
    # get the time in seconds
    now_sec = time.clock_gettime(time.CLOCK_REALTIME)

    # get second in range 0 to 59
    floor_minute = int(now_sec % 60)
    
    if floor_minute == 0:
        break

player.play()

time.sleep(10)


