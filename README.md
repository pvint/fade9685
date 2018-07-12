# fade9685 

Basic CLI application for controlling LEDs over I2C on a Raspbery Pi or similar device
(Most testing is being done with a Pi Zero Wireless)


## DEPENDENCIES

libPCA9685 by edlins
See https://github.com/edlins/libPCA9685

## COMPILATION
	mkdir build && cd build
	cmake ..
	make
	make install  (as root)

## USAGE
```
root@uniform /usr/local/src/fade9685/build [2]$ ./fade9685 -h
Usage:
 fade9685 [options]
Options:
  -h    help, show this screen and quit
  -R    Reset the PCA9685
  -f    Frequency in Hz (24-1526)
  -d    Set Duty Cycle Instantly (0 - 100)
  -l    Fade to Luminosity (0 - 100)
  -s    Step (Larger value fades more quickly)
  -b    Bus number (default 1)
  -a    Address (Default 0x42)
  -c    Channel (0 - 15) Can be repeated for multiple channels or -1 for all
  -v    Show verbose outbut (0-5, 0 = NONE, 5 = DEBUG)
  -D    Enable libPCA9685 debugging
```

## EXAMPLES
To reset the device and set frequency:
```
./fade9685 -b 1 -a 0x41 -f 1000 -R
```

To set two outputs instantly to a specific duty cycle:
```
./fade9685 -b 1 -a 0x41 -c 0 -c 3 -d 50
```

To fade outputs to a certain luminosity level (0-100%): ** this might change in future revisions
```
./fade9685 -b 1 -a 0x41 -c 0 -c 3 -c 14 -c 15 -l 40
```

Fading to accurate amounts with a float is OK too:
```
./fade9685 -b 1 -a 0x41 -c 3 -l 0.6
```

To fade all outputs to a desired level use -c -1:
```
./fade9685 -b 1 -a 0x41 -c -1 -l 90
```

Note that when fading outputs you can control the fade rate with -s

-s 30 or so gives a nice rate with a Pi Zero W.
