# Dial magnifier

![alt text](https://github.com/LockManipulator/Locksport/blob/main/Safe%20manipulation/Electronic%20Dial%20Magnifier/Images/encoder_safe_front.jpg?raw=true)

This is an encoder that sits on the dial of a safe lock to aid in manipulation. It can read down to 0.088 degrees (1/40th of an increment). 



## Use:



1. Slide the base onto the front of the safe
2. Slide the encoder onto the metal rod and make sure it's centered on the dial before tightening the screw onto the rod.
3. If there's not enough friction to spin the encoder then you may need to simply tape it onto the dial. It doesn't take much to get it spinning well with the dial.



Turn the dial exactly to 0 and turn on the device. The encoder will automatically center on zero so don't touch the dial until the number 0 appears on the screen. The encoder can be used as is or you can connect to it's wifi for greater capabilities. Just connect to the wifi “Encoder”, no password, and go to 192.168.0.1 in a web browser. The web page is password protected with a username of “root” and password of “toor” (no quotes in either). The web page will also shows the current value on the encoder in case you didn't get a screen.

- Graph is broken currently. Will fix soon.

- Will add features to change color of text and background, font, etc.



## BOM: 



1 of each print file



1x Esp32s3 mini: https://a.co/d/dx1Epni  



1x 1.28in Screen: https://a.co/d/e5kYErv  



1x (30.5mmx30mm) 1,000mah battery: https://a.co/d/a7W6ZNP  



1x USB-C chage/discharge board: https://a.co/d/149T6ac  



1x On/off switch: https://a.co/d/1uQwlYJ  



1x AS5600 magnetic encoder (with specially polarized magnet: https://a.co/d/66bdv7Q  



1x Bar magnet (the standard 10x60x5 should be fine to glue in too): https://a.co/d/07pE4ml  



1x 6.35mmx100mm rod (or print one): https://a.co/d/d7H9jlQ  



2x m3 heat insert: https://a.co/d/cIcD6mn  



7x m3 bolts (1x 10mm, 2x 20mm, 4x 6mm) and 1 m3 nut: https://a.co/d/1uQwlYJ  



Some spare wires



## Wiring: 


The wires for the screen and sensor will need to be run through the hole in the bottom of the gripper before soldering. Please test your on/off switch to make sure it's off before wiring it up. Mine is off when the button is clicked in and on when the button is out.



### Charging board: 

Battery '+/-' pads to battery '+/-' wires.

Out '+' pad to one of the on/off switch prongs.

Out '-' to esp32 ground pin.



### On/off switch: 

One prong to battery '+' pad and one prong to esp32 5v pin.



### Encoder: 

VCC to esp 3v pin

OUT to esp pin 12

GND to esp ground

DIR to esp pin 5

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


1. Insert the two m3 heat inserts into the gripper lid.
2. Insert the magnet holder into the body and then insert the magnet (the magnet makes it hard to put in and out).
3. Solder wires to the AS5600 board (wires coming out the back!) and screw it into the body (chip side down) with the 4 6mm bolts. Different brands even of the same aesthetic seem to have slightly different dimensions so you make need to lightly file two of the sides of the pcb to fit.
4. Solder each part together and put them in the gripper or put them in first and then solder.
5. Screw bar magnet into the bottom of the base and then insert the rod into the top.
6. Clamp the gripper onto the main encoder body, using the m3x10mm bolt and a nut.

There's just enough space for a piece of thick double sided tape under the battery if you wish. Everything else should be a press fit but you can always use a bit of glue if it's too loose.

## Arduino IDE settings:

Board: ESP32S3 Dev Module  
USB CDC on boot - Enabled  
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
Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)  
PSRAM: QSPI PSRAM  
Upload Mode: UART0/Hardware CDC  
Upload Speed: 921600  
USB Mode: Hardware CDC and JTAG  


The code for the encoder uses Rob Tillaart's AS5600 library https://github.com/RobTillaart/AS5600/tree/master



The code for the screen uses Adafruit's GC9A01A library https://github.com/adafruit/Adafruit_GC9A01A/tree/main

