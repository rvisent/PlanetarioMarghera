# Planetario controller

Includes the main thread with the state machine and the secondary thread handling communication with the 3 microcontrollers

## Structure
Main.c initializes shared memory, semaphores, creates thread_serial and launches everything
Pla.c is the main thread
SerPla.c contains code for the auxiliary thread

## Build instructions
Builds with
./build.sh

## Options
You may wish to change, according to the test environment:
* PlaDefs.h
  #define ROOT_READONLY	1			// 1:root+readonly; 0:normal PI
  If 1 assumes read only root and remounts it rw when necessary to store settings (see Pla.c)

* SerPla.c
  #define COM_PORT "/dev/ttyAMA0" // on RPI
  May be different in other installations
  For example, in my VirtualBox machine, I have
  #define COM_PORT "/dev/ttyS0" // on virtual machine
