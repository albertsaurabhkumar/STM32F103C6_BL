import sys
import os
import serial


# file = open("build/bin/STM32F103C6_BL.bin","rb");

with open("build/bin/STM32F103C6_BL.bin", "rb") as f:
    fw_image = bytearray(f.read())
    f.close()

# Printing data by iterating with while loop
# while data:
#     print(data)
    data = (hex.from_bytes(fw_image.read(1), byteorder="little"))
# Close the binary file