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
#include <argp.h>

#include <PCA9685.h>

int frequency = _PCA9685_MAXFREQ;	// Default to max
int dutycycle = 50;
int rate = 0;
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

void printLog(char *msg, unsigned int verbose, unsigned int level)
{
	if ( level > verbose )
		return;

	fprintf( stderr, "%d: $s", level, msg );
	return;
}

void print_usage(char *name) {
	printf("Usage:\n");
	printf("  %s [options] adapter address\n", name);
	printf("Options:\n");
	printf("  -h\thelp, show this screen and quit\n");
	printf("  -f\tFrequency in Hz (24-1526)\n");
	printf("  -d\tDuty Cycle (0 - 4095)\n");
	printf("  -r\tFade speed (Sets delay in ms between steps)\n");
	printf("  -b\tBus number (default 1)\n");
	printf("  -a\tAddress (Default 0x42)\n");
	printf("  -c\tChannel (0 - 15)\n");
	printf("  -v\tShow verbose outbut (0-5, 0 = NONE, 5 = DEBUG)\n");

} // print_usage

// argp
const char *argp_program_version = "fade9685";
const char *argp_program_bug_address = "pjvint@gmail.com";

static char doc[] = "fade9685 - simple CLI application to control PWM with a PCA9685 device on I2C";
/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 [STRING...]";


static struct argp_option options[] = {
	{ "frequency", 'f', "FREQUENCY", 0, "Frequency" },
	{ "dutycycle", 'd', "DUTYCYCLE", 0, "Duty Cycle (%)" },
	{ "rate", 's', "rate", 0, "Fade Rate Delay (0=instant)" },
	{ "channel", 'c', "CHANNEL", 0, "Channel (0-16)" },
	{ "bus", 'b', "BUS", 0, "Bus number" },
	{ "address", 'a', "ADDRESS", 0, "Address (ie 0x40)" },
	{ "verbose", 'v', "VERBOSITY", 0, "Verbose output" },
	{ "help", 'h', 0, 0, "Show help" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];                /* arg1 & arg2 */
	unsigned int frequency, rate, channel, bus, address, verbose;
	float dutycycle;
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'f':
			arguments->frequency = strtoul( arg, NULL, 10 );
			break;
		case 'd':
			arguments->dutycycle = atof( arg );
			break;
		case 's':
			arguments->rate = atoi( arg );
			break;
		case 'c':
			arguments->channel = atoi (arg );
			break;
		case 'b':
			arguments->bus = atoi( arg );
			break;
		case 'a':
			arguments->address = strtoul( arg, NULL, 16 );
			break;
		case 'v':
			arguments->verbose = 1;
			break;
		case 'h':
			print_usage( "fade9685" );
			exit( 0 );
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
			{
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


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
	unsigned int oldOnVal, oldOffVal;
	unsigned int channel = 0;	// Channel register is channel * 4 + 6
	unsigned int channelReg;
	unsigned int bus = 1;
	unsigned int address = 0x40;
	unsigned int verbose = 0;

	char msg[256];
	int val;

	struct arguments arguments;

	/* Default values. */
	arguments.dutycycle = 0.0f;
	arguments.frequency = 1526;
	arguments.rate = 0;
	arguments.channel = 0;
	arguments.bus = 1;
	arguments.address = 0x40;
	arguments.verbose = 0;

	/* Parse our arguments; every option seen by parse_opt will
	be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);
		
	// TODO Put proper error checks in
	frequency = arguments.frequency;
	dutycycle = arguments.dutycycle;
	rate = arguments.rate;
	channel = arguments.channel;
	bus = arguments.bus;
	address = arguments.address;
	verbose = arguments.verbose;
	


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

	// Get the existing value
	val = PCA9685_getPWMVal(fd, address, channelReg, &oldOnVal, &oldOffVal);

	snprintf( msg, 256, "Current on value: %03x  Current off value: %03x", oldOnVal, oldOffVal );
	printLog( msg, verbose, 3 );
	
	// TODO MUST HAVE: Before doing loop to fade in/out, must have ability to use multiple channels
	// at the same time. And preferably, multiple devices  TODO
	ret = PCA9685_setPWMVal(fd, address, channel, onVal, offVal);

	val = PCA9685_getPWMVal(fd, address, channelReg, &onVal, &offVal);
	fprintf(stderr, "On: %03x   Off: %03x\n", onVal, offVal);
	
	return 0;
} // main
