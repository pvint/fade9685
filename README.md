fade9685 

# Basic CLI application for controlling LEDs over I2C on a Raspberyy Pi or similar device
# (Most testing is being done with a Pi Zero Wireless)


DEPENDENCIES

# libPCA9685 by edlins
	See https://github.com/edlins/libPCA9685

COMPILATION
	mkdir build && cd build
	cmake ..
	make

USAGE
	pi@uniform:/usr/local/src/fade9685/build $ ./fade9685  -h
	Usage:
	 fade9685 [options]
	Options:
	  -h    help, show this screen and quit
	  -R    Reset the PCA9685
	  -f    Frequency in Hz (24-1526)
	  -d    Duty Cycle (0 - 100)
	  -l    Luminosity (0-100)
	  -r    Fade rate (Sets delay in ms between steps. -1 means instant)
	  -b    Bus number (default 1)
	  -a    Address (Default 0x42)
	  -c    Channel (0 - 15)
	  -v    Show verbose outbut (0-5, 0 = NONE, 5 = DEBUG)
	  -D    Enable libPCA9685 debugging

EXAMPLES
	To reset the device and set frequency:
		./fade9685 -b 1 -a 0x41 -f 1000 -R

	To set two outputs instantly to a specific duty cycle:
		./fade9685 -b 1 -a 0x41 -c 0 -c 3 -d 50

	To fade outputs to a certain luminosity level (0-4095): ** this might change in future revisions
		./fade9685 -b 1 -a 0x41 -c 0 -c 3 -c 14 -c 15 -l 4000 
