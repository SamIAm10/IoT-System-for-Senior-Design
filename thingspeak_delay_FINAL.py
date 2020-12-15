import serial, time, requests, json, datetime, csv
from dateutil import parser

seconds_to_run = 60*60 # amount of time for script to capture data

thingspeak_request = "https://api.thingspeak.com/channels/985176/feeds/last.json"

# lists to keep track of data
timestamp_data = []
delay_data = []
error_data = []

# set up the serial line
ser = serial.Serial('COM4', 9600)
time.sleep(2)

# read data from serial input, match it with data in ThingSpeak cloud, and find total delay
end_time = time.time() + seconds_to_run
while time.time() < end_time:
    b = ser.readline()
    string_n = b.decode()
    string = string_n.rstrip()
    if string == "New data!": # if new sensor data is captured, calculate the delay
        now_datetime = datetime.datetime.utcnow()
        timestamp_data.append(now_datetime)
        now = now_datetime.timestamp()
        t_end = time.time() + 10
        while 1:
            response_json = requests.get(thingspeak_request).json()
            last_data_time = parser.isoparse(response_json['created_at']).replace(tzinfo = None).timestamp()
            delay = last_data_time - now
            if -8 <= delay <= 8:
                last_data_time = datetime.datetime.utcnow().timestamp()
                delay = last_data_time - now
                delay_data.append(delay)
                error_data.append(0)
                print("Total delay:", delay, "s")
                break
            elif time.time() >= t_end: # if data in ThingSpeak doesn't match, mark down an error
                delay_data.append(None)
                error_data.append(1)
                print("ERROR! Missing data.")
                break

# write data to csv file
with open('delay_data.csv', 'w+', newline = '') as file:
    writer = csv.writer(file)
    writer.writerow(['Datetime of new data', 'Total delay (seconds)', 'Error occurred?'])
    writer.writerows(zip(timestamp_data, delay_data, error_data))
