# Lesser DiSplay and KeYboard

## What is this project? 

This is an Arduino project to develop a general-purpose computer using an Arduino Mega 2560 (or 2560 mini), 7-segment LED displays (driven by MAX7319 chips), a matrix keypad, and status LEDs, all of which are off-the-shelf components. The computer is intended as a custom control panel of the game Kerbal Space Program, using the kRPC serial-based interface, but in principle the user can use it for whatever they want. 

This project is (obviously?) inspired by the DSKY interface originally developed for the Apollo Guidance Computer back in the 1960s. However, I'm in no way attempting to reconstruct or even emulate the real DSKY or AGC. This is merely a (rather clumsy) attempt to approximate the looks and feels of the original DSKY interface for playing KSP. 

With the LDSKY interface, I want to give KSP players the option to operate their in-game spaceships by either using only the LDSKY hardware interface, or combining the LDSKY with other custom control panels. The player may execute simple actions (such as enabling SAS or performing an azimuth change) by calling VERB/NOUN pairs, and perform complex sequences (such as a launch program) by starting a PROGRAM using VERB 37 (yes, that's an AGC referene). 

## How is the development going? 

As of now, the code backbone of the LDSKY project is complete and mostly bug free. The current code uses interrupt-based routines on top of the main loop to enable a non-blocking user interface, dynamic VERB and PROGRAM managing, error handling, and (very limited) multitasking. 

I don't have any documentation of the system at the moment, as I'm not even sure where this project is going at this point. Interested parties can dig into the code files, which have somewhat comments scattered throughout. For lists of VERBs and PROGRAMs, also refer to the code (for now). 

This is a picture of what the system looks like right now. Here you can see the keypad, 4 rows of segment LEDS (the first row displays PROGRAM, VERB and NOUN, and the rest are data display rows), as well as the UPLINK, KEYREL, OPRERR, and PGMERR lights. The KEYREL (key release light) is lit, indicating the program (02) running in the background is requesting to display something. 

![ghetto much?](https://i.imgur.com/aGGRWeA.jpg "LDSKY in development")

## Credits
The kRPC (kerbal Remote Procedure Call) project, which provides the Serial interface between Arduino boards and Kerbal Space Program. https://github.com/krpc/krpc 
