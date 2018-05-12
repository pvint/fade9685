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
	printf("  -b\tBus number (default 1)\n");
	printf("  -a\tAddress (Default 0x42)\n");
	printf("  -c\tChannel (0 - 15)\n");

} // print_usage

void intHandler(int dummy) {
	//cleanup();
	fprintf(stdout, "Caught signal, exiting (%d)\n", dummy);
	exit(0);
} // intHandler 

int main(int argc, char **argv) {
	_PCA9685_DEBUG = 1;
	_PCA9685_MODE1 = 0x00 | _PCA9685_ALLCALLBIT;
	int opt, ret, fd;
	unsigned int onVal = 0;
	unsigned int offVal = 0;
	unsigned int channel = 0;	// Channel register is channel * 4 + 6
	unsigned int channelReg;
	unsigned int bus = 1;
	unsigned int address = 0x40;

	opterr = 0;

	static struct option long_options[] = 
	{
		{ "frequency", 1, 0, 'f' },
		{ "dutycycle", required_argument, 0, 'd' },
		{ "speed", required_argument, 0, 's' },
		{ "channel", required_argument, 0, 'c' },
		{ "bus", required_argument, 0, 'b' },
		{ "address", required_argument, 0, 'a' },
		{ "help", 0, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	// I think I hate getopt - let's try argp TODO
	while ((opt = getopt_long( argc, argv, "fdscba", long_options, NULL)) != EOF)
	{
		switch ( opt )
		{
			case 'f':  // Frequency
				if ( optarg == NULL )
				{

					fprintf( stderr, "Option -%c requires an argument\n", opt );
					exit( 1 );
				}
				frequency = atoi(optarg);
				fprintf(stderr, "ZZZ %d", frequency);
				if ( ( frequency < _PCA9685_MINFREQ ) || ( frequency > _PCA9685_MAXFREQ ) )
				{
					fprintf( stderr, "Frequency must be %d - %d.\n", _PCA9685_MINFREQ, _PCA9685_MAXFREQ );
					exit( 1 );
				}
				break;
			case 'd':  // Duty Cycle
				if ( optarg == NULL )
				{
					fprintf( stderr, "Option -%c requires an argument\n", opt );
					exit( 1 );
				}
				dutycycle = atoi(optarg);
				if ( ( dutycycle < 0 ) || ( dutycycle > 4095 ) )
				{
					fprintf( stderr, "Duty cycle must be 0 - 4095\n" );
				}
				break;
			case 's':  // Speed (delay in ms)
                                if ( optarg == NULL )
                                {
                                        fprintf( stderr, "Option -%c requires an argument\n", opt );
                                        exit( 1 );
                                }
				delay = atoi(optarg);
				break;
			case 'c':  // Channel   TODO: Allow multiple channels
                                if ( optarg == NULL )
                                {
                                        fprintf( stderr, "Option -%c requires an argument\n", opt );
                                        exit( 1 );
                                }
				channel = atoi(optarg);
				if ( ( channel < 0 ) || ( channel >= _PCA9685_CHANS ) )
				{
					fprintf( stderr, "Channel must be 0 - %d\n", _PCA9685_CHANS - 1 );
				}
				break;
			case 'b':  // Bus (default 1)
                                if ( optarg == NULL )
                                {
                                        fprintf( stderr, "Option -%c requires an argument\n", opt );
                                        exit( 1 );
                                }
				bus = atoi( optarg );
				break;
			case 'a':
                                if ( optarg == NULL )
                                {
                                        fprintf( stderr, "Option -%c requires an argument\n", opt );
                                        exit( 1 );
                                }
				address = atoi( optarg );
				break;
			case 'h':  // help mode
				print_usage(argv[0]);
				exit(0);
		} //switch
	} //while



	// register the signal handler to catch interrupts
	signal(SIGINT, intHandler);

	// initialize the I2C bus adpt and a PCA9685 at addr with freq
	fd = initHardware( bus, address, frequency );

	if (fd < 0) {
		fprintf(stderr, "main(): initHardware() returned ");
		fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, bus, address);
		return -1;
	} //if
	

	// PWM init
	PCA9685_initPWM(fd, address, frequency);

	// Set the duty cycle on and off values
	offVal = dutycycle;

	channelReg = channel * 4 + _PCA9685_BASEPWMREG;
	ret = PCA9685_setPWMVal(fd, address, channel, onVal, offVal);
	//ret = PCA9685_setPWMVal(fd, addr, opthannelReg, 0, 4000);

	int val;
	val = PCA9685_getPWMVal(fd, address, channelReg, &onVal, &offVal);
	fprintf(stderr, "On: %03x   Off: %03x\n", onVal, offVal);
	
	return 0;
} // main
