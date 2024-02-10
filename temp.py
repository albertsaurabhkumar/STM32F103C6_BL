import serial
import os
import sys
import binascii

try:
    ser = serial.Serial("COM3",timeout=45)  # open first serial port
except(OSError,serial.SerialException):
    print("No Port Availble")

print(ser.portstr)      # check which port was really used

with open("./../STM32F103C6_App/build/bin/STM32F103C6_APP.bin", "rb") as f:
        tSizeInt = len(f.read())
        totalsize = str(hex(tSizeInt))
        print("Size of the Binary file is: "+totalsize)
        # fw_image = bytes(f.read())
f.close()

dataTxCompleted =False
currentState = 'ReqDownload'

def recvAck() -> bytes:
    return ser.read(1)

while True:
    match currentState:
        case 'ReqDownload':
            print("Sending Download seq..")
            ser.write(0x56.to_bytes(1,'little'))
            ser.write(tSizeInt.to_bytes(2,'little'))                      # Request the download sequence to synchronize whith the server
            currentState = 'waitForAck'

        case 'waitForAck':
            print("Waiting For Ack...")            # Request the download sequence to synchronize whith the server

            if (recvAck() == b'U') & (dataTxCompleted == False):
                print("Ack Received and Sending the data...")
                currentState = 'sendData'
            else:
                currentState = 'done'

        case 'sendData':                            # Send the data as client <--> server are synchronised
            print("Sending the data...")
            with open("./../STM32F103C6_App/build/bin/STM32F103C6_APP.bin", "rb") as f:
                 numofbytes=0
                 while True:
                      data = f.read(8)
                      if data:
                           numofbytes+=len(data)
                           print(str(hex(numofbytes))+"("+str(len(data))+") : "+totalsize)
                           ser.write(b'1')
                           ser.write(len(data).to_bytes(1, 'little'))
                           if len(data)<8:
                            ser.write(data)
                            print("Padding "+ str(8-len(data))+" bytes")
                            for i in range(8-len(data)):
                             ser.write(b'0')
                           else:
                            ser.write(data)

                           ser.write(b'a')
                           if numofbytes % 32 == 0:
                                i=ser.read(1)
                                if i == b'U':
                                     #print("Ackment Received continuing the send data")
                                     continue
                                else:
                                     print("Ackment not received Terminating transfer "+str(hex(numofbytes)))
                                     break
                      else:
                           break
            f.close()

            currentState = 'done'
            # dataTxCompleted = True
    
        case 'done':
            print("Transfer Done Exiting...")
            break

# while data:
#     print(data)
#     data = fw_image.to_bytes(4,"little")

#     ser.write(i.encode('hex')))      # write a string
# ser.close()             # close port