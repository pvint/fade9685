// LED Fader application
// uses libPCA9685


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>

#include <PCA9685.h>

int frequency = _PCA9685_MAXFREQ;	// Default to max
int dutycycle = 50;
int delay = 0;
unsigned char channel = 0;

int initHardware(int adpt, int addr, int freq) {
	int fd;
	int ret;

	// setup the I2C bus device 
	fd = PCA9685_openI2C(adpt, addr);
	if (fd < 0) {
		fprintf(stderr, "initHardware(): PCA9685_openI2C() returned ");
		fprintf(stderr, "%d for adpt %d at addr %x\n", fd, adpt, addr);
		return -1;
	} // if
	// initialize the pca9685 device
	ret = PCA9685_initPWM(fd, addr, freq);
	if (ret != 0) {
		fprintf(stderr, "initHardware(): PCA9685_initPWM() returned %d\n", ret);
		return -1;
	} // if

	return fd;
}  // initHardware


void print_usage(char *name) {
	printf("Usage:\n");
	printf("  %s [options] adapter address\n", name);
	printf("Options:\n");
	printf("  -h\thelp, show this screen and quit\n");
	printf("  -f\tFrequency in Hz (24-1526)\n");
	printf("  -d\tDuty Cycle (0 - 4095)\n");
	printf("  -s\tFade speed (Sets delay in ms between steps)\n");
	

} // print_usage

void intHandler(int dummy) {
	//cleanup();
	fprintf(stdout, "Caught signal, exiting (%d)\n", dummy);
	exit(0);
} // intHandler 

int main(int argc, char **argv) {
	_PCA9685_DEBUG = 1;
	_PCA9685_MODE1 = 0x00 | _PCA9685_ALLCALLBIT;
	int c, ret, fd;
	unsigned int onVal = 0;
	unsigned int offVal = 0;

	opterr = 0;
	while ((c = getopt (argc, argv, "hfds")) != -1)
	{
		switch (c)
		{
			case 'f':  // Frequency
				frequency = atoi(optarg);
			case 'd':  // Duty Cycle
				dutycycle = atoi(optarg);
			case 's':  // Speed (delay in ms)
				delay = atoi(optarg);
			case 'c':  // Channel   TODO: Allow multiple channels
				channel = atoi(optarg);
			case 'h':  // help mode
				print_usage(argv[0]);
				exit(0);
		} //switch
	} //while

	if ((argc - optind) != 2) {
		print_usage(argv[0]);
		exit(-1);
	}  // if argc

	long ladpt = strtol(argv[optind], NULL, 16);
	long laddr = strtol(argv[optind + 1], NULL, 16);
	if (ladpt > INT_MAX || ladpt < 0) {
		fprintf(stderr, "ERROR: adapter number %ld is not valid\n", ladpt);
		exit(-1);
	} // if ladpt
	int adpt = ladpt;

	if (laddr > INT_MAX || laddr < 0) {
		fprintf(stderr, "ERROR: address 0x%02lx is not valid\n", laddr);
		exit(-1);
	} // if laddr
	unsigned char addr = laddr;


	// register the signal handler to catch interrupts
	signal(SIGINT, intHandler);

	// initialize the I2C bus adpt and a PCA9685 at addr with freq
	fd = initHardware(adpt, addr, 999);

	if (fd < 0) {
		fprintf(stderr, "main(): initHardware() returned ");
		fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, adpt, addr);
		return -1;
	} //if
	

	// PWM init
	//PCA9685_initPWM(fd, addr, frequency);

	// Set the duty cycle on and off values
	offVal = 0xFFF * dutycycle;

	//ret = PCA9685_setPWMVal(fd, addr, channel, onVal, offVal);
	ret = PCA9685_setPWMVal(fd, addr, 0, 0, 0xef);

	int val;
	val = PCA9685_getPWMVal(fd, addr, 0, &onVal, &offVal);
	fprintf(stderr, "On: %03x   Off: %03x\n", onVal, offVal);
	
	return 0;
} // main
