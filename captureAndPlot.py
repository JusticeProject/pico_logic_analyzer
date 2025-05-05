# to use the serial library and without needing sudo:
# pip install pyserial
# sudo usermod -a -G dialout <username>
# need to reboot afterwards

# may need to install PyQt to use matplotlib
# pip install PyQt6   (or later version if available)

import serial
import time
import sys
import numpy as np
import matplotlib.pyplot as plt

###################################################################################################

def plotData(filename, secsPerSample):
    fd = open(filename, "r")
    dataListStr = fd.read()
    fd.close()

    dataListInt = [int(s) for s in dataListStr]
    timeList = [i*secsPerSample for i in range(0, len(dataListInt))]

    #print(f"length of adcIntList = {len(adcIntList)} first = {adcIntList[0]} last = {adcIntList[len(adcIntList)-1]}")
    #print(f"length of timeList = {len(timeList)}")

    y = np.array(dataListInt)
    x = np.array(timeList)

    plt.plot(x, y)
    plt.xlabel("Time [s]")
    plt.ylabel("Value")
    #plt.axis([0, x[-1], 0, 1])
    plt.show()

###################################################################################################

def configureSampleRate_us(ser, microseconds):
    print(f"configuring sample rate with {microseconds} microseconds")
    ser.write(b"u")
    print("reply from pico...")
    ser.write(microseconds.encode(encoding="utf-8"))
    print(ser.readline().decode(encoding="utf-8").rstrip("\r\n"))
    print(ser.readline().decode(encoding="utf-8").rstrip("\r\n"))
    print("...end of reply")

###################################################################################################

def doCapture(ser):
    ser.write(b"c")
    data = ser.readline()
    print(f"read {len(data)} bytes")
    data_str = data.decode(encoding="utf-8") # convert from bytes to string
    data_str = data_str.rstrip("\r\n")
    print(f"have {len(data_str)} samples of data")
    
    fd = open("data1.txt", "w")
    n = fd.write(data_str)
    fd.close()
    print(f"wrote {n} bytes of data to data1.txt")

###################################################################################################

#if len(sys.argv) > 1:
#    if sys.argv[1] == "debug":

ser = serial.Serial("/dev/ttyACM0", 115200)

usPerSample = "56"
print(f"entered {usPerSample} microseconds")
for i in range(0, 6-len(usPerSample)):
    usPerSample = "0" + usPerSample
print(f"will use {usPerSample} when sending to pico")

configureSampleRate_us(ser, "000056")
doCapture(ser)

secsPerSample = int(usPerSample) / 1000000.0
plotData("data1.txt", secsPerSample)
