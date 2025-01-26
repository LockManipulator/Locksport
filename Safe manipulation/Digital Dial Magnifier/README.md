# Dial magnifier

![alt text](https://github.com/LockManipulator/Locksport/blob/main/Safe%20manipulation/Digital%20Dial%20Magnifier/Images/DDM-Front.jpg?raw=true)

This is an encoder that sits on the dial of a safe lock to aid in manipulation. It can read down to 0.088 degrees (1/40th of an increment). Although you'll generally get 1/10 of an increment from play in the dial, dial ring, build tolerances, etc. The screen colors can be customized as well as if you want a border or animated pointer to show. It also has wifi you can connect to for automated graphing. Just connect to the wifi "DDM" with the password "magnifier" and go to 192.168.0.1 in a web browser. NOTE: I don't have input sanitization yet so don't enter letters when it should be numbers and vice versa or it might crash.

<img src="https://github.com/LockManipulator/Locksport/blob/main/Safe%20manipulation/Digital%20Dial%20Magnifier/Images/DDM-WebGraph.jpg?raw=true" width="400">

## Features:

1. Precise dial readings.
2. Customizable colors and styles.
3. Saveable graph of contact points. 
4. Can run off either the 1,000mah (at least several hours) battery or USB c.
5. Has pass through charging.
6. Compact.
7. Cheap ($10-$20 depending where you buy).


## To do

1. Input sanitization.
2. Try to automatically detect contact point by feeling change in acceleration as the user spins past.
3. Guided manipulation.


## Use


1. Put a small piece of double sided tape in the middle of the dial.
2. Slide the magnetic base on to the front of the safe.
3. Slide the main body onto the rod and make sure it's both centered over the dial and level before sticking it on. I check to make sure the base is the right distance away then look at the angle of the magnifier on the rod to get it centered.
4. Make sure the body is level before you tighten the screws onto the rod. The design is supposed to allow for a bit of rotation on the rod to account for it not being centered on the dial perfectly. This is ok if you see some wiggle. The screws are there to keep the body level although aren't 100% necessary.
5. The magnifier will automatically think it's on 0 so turn the dial to 0 before turning it on.


## Tips and troubleshooting

The magnifier must be as centered as possible and the body must be level. I noticed that if I mess up on this, the double sided tape holding the magnifier on will curl up and push the magnifier off. The coupler between the magnet and dial has just enough bend and clearance in the body to allow for some slop without affecting the accuracy.

I've noticed on some locks, the front of dial is just a plastic cover which may be loose. This will lead to error depending on which direction you are turning. Also, there can be a piece of plastic on the inside edge of the dial ring between the dial and dial ring. This has the opening index on it and will give errors in reading/dialing if it's loose.


## BOM: 


The below links are simply the exact items I bought. It will most likely be cheaper if you find a listing for smaller quantities. 



1 of each print file (I print in PETG) except print 4x Encoder-CouplerMiddle in TPU



1x Esp32s3 mini: https://a.co/d/dx1Epni  



1x 1.28in Round TFT Screen: https://a.co/d/e5kYErv  



1x Battery (1,000mah, max 36mmx31mmx11mm) : https://a.co/d/a7W6ZNP  



1x USB-C charge/discharge board: https://a.co/d/149T6ac  



1x On/off switch (12mmx8mmx8mm): https://a.co/d/0qDNLT6



1x AS5600 magnetic encoder (with diametrically polarized magnet): https://a.co/d/66bdv7Q  



1x Bar magnet (60mmx13.5mmx5mm but the standard 60x10x5 should be fine to glue in too): https://a.co/d/07pE4ml  



1x 6.35mmx100mm rod (or print one): https://a.co/d/d7H9jlQ  



2x m3 heat insert (sized to fit in 5.2mm diameter hole): https://a.co/d/cIcD6mn



6x m3 bolt (2x >=15mm, 4x 6mm-10mm): https://a.co/d/1uQwlYJ  



Wires: You probably want red, black, and at least 4 other colors as well. I don't recommend anything thicker than 22 gauge due to the space. 



## Wiring: 


Please test your on/off switch to make sure it's off before wiring it up. Mine is off when the button is clicked in and on when the button is out.



### Charging board: 

Battery '+/-' pads to battery '+/-' wires.

Out '+' pad to one of the on/off switch prongs.

Out '-' to esp32 ground pin.



### On/off switch: 

One prong to battery '+' pad and the other prong to esp32 5v pin.



### Encoder: 

VCC to esp 3v pin

OUT to esp pin 5

GND to esp ground

DIR to esp ground

SDA to esp pin 8

SCL to esp pin 9



### Screen: 

VCC: Esp32 5v pin

GND: Esp32 ground pin

SCL: to esp pin 2

SDA: to esp pin 3

DC: to esp pin 10

CS: to esp pin 11

RST: to esp pin 12



## Assembly guide:


1. Insert the two m3 heat inserts into the gripper lid from inside the rod hole.
2. Solder one wire to each leg of the button and press it into place.
3. Glue the 3 coupler parts together and insert into the body before putting the magnet in (the magnet makes it hard to put in and out).
4. Solder wires to the AS5600 board (wires coming out the back!) and screw it into the body (chip side down, wire side up) with the 4 6mm bolts. Different brands even of the same aesthetic seem to have slightly different dimensions so you make need to lightly file a side of the pcb to fit.
5. Solder the headers onto the esp32 facing up. Clip the bottom flush and after inserting into the body, clip the headers near the encoder so they don't stick up past the bolts holing the encoder down.
6. Put the charging board into the body and solder the battery to it.
7. Wire up the encoder and charging board to the esp.
8. Solder the screen wires to the esp, put the screen in, then solder the wires to the screen. 
9. Put the lid on. The tab on the flat end goes first then lower the rounded end down. 
10. Screw the bar magnet into the bottom of the base and then insert the rod into the top.


## Arduino IDE settings:

Board: ESP32S3 Dev Module  
USB CDC on boot: Enabled  
CPU Frequency: 240MHz (WiFi)  
Core Debug Level: None  
USB DFU On Boot: Disabled  
Erase All Flash Before Sketch Upload: Disabled  
Events Run On: Core 1  
Flash Mode: QIO 80MHz  
Flash Size: 4MB (32MB)  
JTAG Adapter: Disabled  
Arduino Runs On: Core 1  
USB Firmware MSC On Boot: Disabled  
Partition Scheme: Default 4MB with SPIFFS (1.2MB APP/1.5MB SPIFFS)
PSRAM: QSPI PSRAM  
Upload Mode: UART0/Hardware CDC  
Upload Speed: 921600  
USB Mode: Hardware CDC and JTAG  



If you're using Arduino IDE above version 1.8 (99.9% you are if you use Arduino IDE) then do this to upload the web page into the esp on board filesystem:



1. Make sure your arduino sketch project folder has a folder called 'data' with the web page files in it. The 'data' folder should be in the same directory as encoder.ino.
2. Download the latest .vsix file from https://github.com/earlephilhower/arduino-littlefs-upload/releases (where these instructions come from).
3. Put it in ~/.arduinoIDE/plugins/ on Mac and Linux or C:\Users\<username>\.arduinoIDE\plugins\ on Windows (you may need to make this directory yourself beforehand)
4. Restart the IDE.
5. [Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32". If there's an error of not being able to use the serial port, close all serial monitors and plotters and try again.



The code for the encoder uses Rob Tillaart's AS5600 library https://github.com/RobTillaart/AS5600/tree/master



The code for the screen uses Adafruit's GC9A01A library https://github.com/adafruit/Adafruit_GC9A01A/tree/main



Wifi libraries



https://github.com/mathieucarbou/AsyncTCP



https://github.com/mathieucarbou/ESPAsyncWebServer


