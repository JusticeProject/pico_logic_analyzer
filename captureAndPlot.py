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

def plotData(filename, secsPerSample, numPins):
    fd = open(filename, "r")

    for i in range(0, numPins):
        dataListStr = fd.readline()
        dataListStr = dataListStr.replace("\n", "")
        dataListInt = [int(s) for s in dataListStr]
        timeList = [i*secsPerSample for i in range(0, len(dataListInt))]
        y = np.array(dataListInt)
        x = np.array(timeList)
        plt.plot(x, y)

    fd.close()
    plt.xlabel("Time [s]")
    plt.ylabel("Value")
    plt.show()

###################################################################################################

def printReplyFromPico(ser, numLines):
    for i in range(0, numLines):
        print(ser.readline().decode(encoding="utf-8").rstrip("\r\n"))

###################################################################################################

def configureSampleRate_us(ser, microseconds):
    ser.write(b"u")
    ser.write(microseconds.encode(encoding="utf-8"))
    printReplyFromPico(ser, 2)

###################################################################################################

def configureEdge(ser, triggerLogicHigh):
    if triggerLogicHigh:
        ser.write(b"o")
    else:
        ser.write(b"z")
    printReplyFromPico(ser, 1)

###################################################################################################

def configureNumPins(ser, numPinsStr):
    ser.write(b"p")
    ser.write(numPinsStr.encode(encoding="utf-8"))
    printReplyFromPico(ser, 1)

###################################################################################################

def doCapture(ser, numPins):
    ser.write(b"c")
    fd = open("data1.txt", "w")

    for i in range(0, numPins):
        data = ser.readline()
        print(f"read {len(data)} bytes")
        data_str = data.decode(encoding="utf-8") # convert from bytes to string
        data_str = data_str.rstrip("\r\n")
        print(f"have {len(data_str)} samples of data")
        n = fd.write(data_str + "\n")
        print(f"wrote {n} bytes of data to data1.txt")

    fd.close()

###################################################################################################

#if len(sys.argv) > 1:
#    if sys.argv[1] == "debug":

ser = serial.Serial("/dev/ttyACM0", 115200)

# TODO: check args for us/ns
usPerSample = "56"
for i in range(0, 6-len(usPerSample)):
    usPerSample = "0" + usPerSample

# TODO: check args for logic 1/0
triggerLogicHigh = False

# TODO: check args for number of pins, add 0 if needed to make it two digits, verify it's a power of 2
numPins = 2
numPinsStr = str(numPins)
if len(numPinsStr) == 1:
    numPinsStr = "0" + numPinsStr

configureSampleRate_us(ser, "000056")
configureNumPins(ser, numPinsStr)
configureEdge(ser, triggerLogicHigh)
doCapture(ser, numPins)

secsPerSample = int(usPerSample) / 1000000.0
plotData("data1.txt", secsPerSample, numPins)
