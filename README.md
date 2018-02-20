NAME: CSE 438 Assignment-2 Part-2
________________________________________________________________________________________________________________________________________

AUTHORS:  Team-1

Vishwakumar Dhaneshbhai Doshi (1213322381)
Nisarg Trivedi (1213314867)
________________________________________________________________________________________________________________________________________

INCLUDED IN REPOSITORY:

-> RGBLed.c (Driver Program Source code)
-> User_prog.c(User Program Source code)
-> Makefile
-> README
________________________________________________________________________________________________________________________________________

ABOUT: 

This project demonstrates using GPIO pins in kernel on Galileo Gen-2 board to interface LEDs. Corresponding Linux gpio pins are set to select direction and select IO pin via multiplexing on board. PWM logic is used to control the intensity of LEDs.There are total 7 patterns and these patterns change every 0.5s. Program terminates on a mouse click. 
________________________________________________________________________________________________________________________________________

SYSTEM REQUIREMENTS:

-> Linux machine for host. Compilation must be done on this machine and not on the board.
-> LINUX KERNEL : Minimum version of 2.6.19 is expected.
-> SDK: iot-devkit-glibc-x86_64-image-full-i586-toolchain-1.7.2
-> GCC:  i586-poky-linux-gcc
-> Intel Galileo Gen2 board
-> USB mouse
-> Bread-board,wires and Red-Green-Blue LEDs.
________________________________________________________________________________________________________________________________________

SETUP:

-> If your SDK is installed in other location than default one, change the path accordingly in makefile.

-> You must boot Galileo from SD card with linux version 3.19.

-> Open 2 terminal windows, one will be used for host commands and other will be used to send command to Galileo board.

-> Connect the FTDI cable and on one terminal type:

           sudo screen /dev/ttyUSB0 115200

 to communicate with board. This will be used for board comminication and is referred to as screen terminal, while other window is used for host commands. 

-> Connect ethernet cable and choose board ip address 192.168.1.100 and host ip address 192.168.1.105 (last number can be any from 0-255 but different from Host's number) by typing:

	ifconfig enp2s0 192.168.1.105 netmask 255.255.0.0 up (on host terminal)

	ifconfig enp0s20f6 192.168.1.100 netmask 255.255.0.0 up (on screen terminal)

-> You are required to know the /dev/input/event# number corresponding to mouse in board(generally event2 or event3). For that in screen terminal type:

	      cd /dev/input
      	ls by-path 

and note the event number corresponding to mouse event and change the path in mDevice variable led.h library accordingly.

-> Connect RED,GREEN and BLUE LEDs to any 3 digital io pins on Galileo board.
________________________________________________________________________________________________________________________________________

COMPILATION:

-> On the host, extract all files in zip file in one directory, open directory in terminal in which files are present and type make in terminal.

-> To copy files to Galileo board type in host terminal:

      scp /home/vishva/gpio/RGBLed.ko root@192.168.1.100:/home/vishva (only an example use your user name and corresponding path name)

      scp /home/vishva/part2/User_prog root@192.168.1.100:/home/vishva

________________________________________________________________________________________________________________________________________

EXECUTION:

-> Go to relevent directory in screen terminal and,
-> type in screen terminal:

	    insmod RGBLed.ko

then type:

	    ./User_prog "Desired PWM 0-100" "RED pin_number" "GREEN pin_number" "BLUE pin_number" 

 (without quotation marks and integer values within) to run the code.

 eg.) ./User_prog 50 0 3 10

Range of values to be entered:

-> PWM - 0-100
-> LED pin numbers - 0-13
________________________________________________________________________________________________________________________________________

EXPECTED OUTPUT:

-> On running the object code, the on time and off time of LEDs will be displayed and start glowing with the intensity specified by the user with the pattern R-G-B-RG-RB-GB-RGB with each pattern running for 0.5seconds.

-> If any invalid input is given or out of range value is given, it produces Error.

-> A parallel thread will check for mouse click event and the program will terminate on detection of a mouse click after completion of current running pattern.

