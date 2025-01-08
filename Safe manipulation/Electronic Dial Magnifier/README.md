# Dial magnifier


This is an encoder that sits on the dial of a safe lock to aid in manipulation. It can read down to 0.088 degrees (1/40th of an increment). 



Use:



Slide the legs onto the front of the safe
Slide the encoder onto one of the legs and make sure it's centered on the dial before adjusting the second leg to match.
Slide the encoder onto the dial and tighten the screws on the legs.
If there's not enough friction to spin the encoder then you may need to simply tape it onto the dial. It doesn't take much to get it spinning well with the dial.



Turn the dial exactly to 0 and turn on the device. The encoder will automatically center on zero so don't touch the dial until the number 0 appears on the screen. The encoder can be used as is or you can connect to it's wifi for greater capabilities. Just connect to the wifi “Encoder”, no password, and go to 192.168.0.1 in a web browser. The web page is password protected with a username of “root” and password of “toor” (no quotes in either). You can plot points to be graphed live and even download the graph as an image. The web page also shows the current value on the encoder in case you didn't get a screen.



BOM: 



1x Esp32s3 mini: https://a.co/d/dx1Epni



1x 1.28in Screen: https://a.co/d/e5kYErv



1x (30.5mmx30mm) 1,000mah battery: https://a.co/d/a7W6ZNP



1x USB-C chage/discharge board: https://a.co/d/149T6ac



1x On/off switch: https://a.co/d/1uQwlYJ



1x AS5600 magnetic encoder (with specially polarized magnet: https://a.co/d/66bdv7Q



1x Bar magnet (the standard 10x60x5 should be fine to glue in too): https://a.co/d/07pE4ml



2x m3 bolts (1x >10mm 1x >6mm): https://a.co/d/1uQwlYJ



Some spare wires



Wiring: 



Charge/discarge board: 

Battery +/- to the battery +/- wires.
Out + to one of the on/off switch prongs.
Out - to esp32 ground pin.



Encoder: 

Encoder dir pin to esp ground. 



Screen: 





Assembly guide:



Insert the magnet holder into the body and then insert the magnet (the magnet makes it hard to put in and out).
Solder wires to the AS5600 board (wires coming out the back!) and screw it into the body (chip side down). Different brands even of the same aesthetic seem to have slightly different dimensions so you make need to lightly file two of the sides of the pcb to fit.
Solder each part together and put them in the gripper.



The code for the encoder uses Rob Tillaart's AS5600 library https://github.com/RobTillaart/AS5600/tree/master



The code for the screen uses Adafruit's GC9A01A library https://github.com/adafruit/Adafruit_GC9A01A/tree/main

