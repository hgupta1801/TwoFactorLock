# TwoFactorLock
This project uses an Arduino UNO R3 to use the RFID sensor and keypad in order to rotate a servo.

Arduino code requires you import <Adafruit_MPR121.h> and <MFRC522.h>
Use the STL files to 3D Print an actuaor for a micro-servo
Use the box file to laser print a demo box to attach everything to

Things to change:

Change authorizedUID on line 11 for your paticular RFID device. 
Run this code and it will print the RFID UID as well.

Change the correctPin on line 27 to your desired pin.
