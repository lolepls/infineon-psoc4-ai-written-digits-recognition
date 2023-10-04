import csv
import os
import serial

def create_csv_file(filename):
    with open(filename, 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(['label'] + [f'pixel_{i}' for i in range(784)])

def append_to_csv(filename, label, data):
    with open(filename, 'a', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow([label] + data)

def main():
    filename = "fine_tuning_dataset.csv"
    if not os.path.isfile(filename):
        create_csv_file(filename)

    ser = serial.Serial('COM5', 115200)
    
    current_label = 0
    MAXNUMBER = 30
    acquired = 0

    try:
        while True:
            line = ser.readline().decode().strip()
            
            if(line=="###"):
                print("End of acquisition!")
                return
            
            line = line.split("*")
            label = line[1]
            data_str = line[2]
            
            label = int(label)
            
            if acquired == MAXNUMBER:
                acquired = 0
                current_label = current_label + 1
            
            data_str = data_str.split(',')
            data = []
            
            for value in data_str:
                data.append(value)
            
            append_to_csv("fine_tuning_dataset.csv", label, data)
            
            acquired = acquired + 1
            
            print("Data for label " + str(label) + "written. Sample remaining: " + str(MAXNUMBER-acquired))
                

    except KeyboardInterrupt:
        print("Process interrupted by user.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()