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

int initHardware(unsigned int adpt, unsigned int addr, unsigned int freq, unsigned int reset) {
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
	if ( reset == 1 )
	{
		ret = PCA9685_initPWM(fd, addr, freq);
		if (ret != 0) {
			fprintf(stderr, "initHardware(): PCA9685_initPWM() returned %d\n", ret);
			return -1;
		} // if
	} // if reset

	return fd;
}  // initHardware

void printLog(char *msg, unsigned int verbose, unsigned int level)
{
	if ( level > verbose )
		return;

	fprintf( stderr, "%d: %s\n", level, msg );
	return;
}

void print_usage(char *name) {
	printf("Usage:\n");
	printf(" %s [options]\n", name);
	printf("Options:\n");
	printf("  -h\thelp, show this screen and quit\n");
	printf("  -R\tReset the PCA9685\n");
	printf("  -f\tFrequency in Hz (24-1526)\n");
	printf("  -d\tDuty Cycle (0 - 4095)\n");
	printf("  -l\tLuminosity (0-100)\n");
	printf("  -r\tFade rate (Sets delay in ms between steps. -1 means instant)\n");
	printf("  -b\tBus number (default 1)\n");
	printf("  -a\tAddress (Default 0x42)\n");
	printf("  -c\tChannel (0 - 15)\n");
	printf("  -v\tShow verbose outbut (0-5, 0 = NONE, 5 = DEBUG)\n");
	printf("  -D\tEnable libPCA9685 debugging\n");

} // print_usage

// argp
const char *argp_program_version = "fade9685";
const char *argp_program_bug_address = "pjvint@gmail.com";

static char doc[] = "fade9685 - simple CLI application to control PWM with a PCA9685 device on I2C";
/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 [STRING...]";


static struct argp_option options[] = {
	{ "reset", 'R', 0, 0, "Reset PCA9685" },
	{ "frequency", 'f', "FREQUENCY", 0, "Frequency" },
	{ "dutycycle", 'd', "DUTYCYCLE", 0, "Duty Cycle (%)" },
	{ "luminosity", 'l', "LUMINOSITY", 0, "Luminosity (0.0 - 100.0)" },
	{ "rate", 's', "rate", 0, "Fade Rate Delay (0=instant)" },
	{ "channel", 'c', "CHANNEL", 0, "Channel (0-16)" },
	{ "bus", 'b', "BUS", 0, "Bus number" },
	{ "address", 'a', "ADDRESS", 0, "Address (ie 0x40)" },
	{ "verbose", 'v', "VERBOSITY", 0, "Verbose output" },
	{ "debug", 'D', 0, 0, "libPCA9685 debugging" },
	{ "help", 'h', 0, 0, "Show help" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];                /* arg1 & arg2 */
	unsigned int frequency, channel, bus, address, verbose, reset;
	int rate;
	float dutycycle, luminosity;
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
		case 'l':
			arguments->luminosity = atof( arg );
			break;
		case 'r':
			arguments->rate = strtol( arg, NULL, 10 );
		case 's':
			arguments->rate = atoi( arg );
			break;
		// --channel can be used multiple times and values will be bitwise ANDed to the channel variable
		case 'c':
			arguments->channel = arguments->channel | ( 1 << atoi ( arg ) );
			break;
		case 'b':
			arguments->bus = atoi( arg );
			break;
		case 'a':
			arguments->address = strtoul( arg, NULL, 16 );
			break;
		case 'v':
			arguments->verbose = strtoul( arg, NULL, 10 );
			break;
		case 'D':
			_PCA9685_DEBUG = 1;
			break;
		case 'R':
			arguments->reset = 1;
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
	_PCA9685_DEBUG = 0;
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
	unsigned int reset = 0;
	float luminosity, dutycycle;

	char msg[256];
	int val;

	struct arguments arguments;

	/* Default values. */
	arguments.dutycycle = 0.0f;
	arguments.luminosity = 0.0f;
	arguments.frequency = 1526;
	arguments.rate = 0;
	arguments.channel = 0;
	arguments.bus = 1;
	arguments.address = 0x40;
	arguments.verbose = 0;
	arguments.reset = 0;

	/* Parse our arguments; every option seen by parse_opt will
	be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);
		
	// TODO Put proper error checks in
	frequency = arguments.frequency;
	dutycycle = arguments.dutycycle;
	luminosity = arguments.luminosity;
	rate = arguments.rate;
	channel = arguments.channel;
	bus = arguments.bus;
	address = arguments.address;
	verbose = arguments.verbose;
	reset = arguments.reset;


	// register the signal handler to catch interrupts
	signal(SIGINT, intHandler);

	// initialize the I2C bus adpt and a PCA9685 at addr with freq
	fd = initHardware( bus, address, frequency, reset );

	if (fd < 0) {
		fprintf(stderr, "main(): initHardware() returned ");
		fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, bus, address);
		return -1;
	} //if
	

	// Set the duty cycle on and off values
	offVal = dutycycle;

	snprintf( msg, 256, "Bitmask of channels: %04x\n", channel );
	printLog( msg, verbose, 4 );

	channelReg = channel * 4 + _PCA9685_BASEPWMREG;

	// Get the existing value
	val = PCA9685_getPWMVal(fd, address, channelReg, &oldOnVal, &oldOffVal);

	snprintf( msg, 256, "Current on value: %03x  Current off value: %03x", oldOnVal, oldOffVal );
	printLog( msg, verbose, 3 );
	
	// TODO MUST HAVE: Before doing loop to fade in/out, must have ability to use multiple channels
	// at the same time. And preferably, multiple devices  TODO
	snprintf( msg, 256, "Setting on value: %03x  Setting off value: %03x", onVal, offVal );
	printLog( msg, verbose, 5 );

	ret = PCA9685_setPWMVal(fd, address, channelReg, onVal, offVal);

	val = PCA9685_getPWMVal(fd, address, channelReg, &onVal, &offVal);
	fprintf(stderr, "On: %03x   Off: %03x\n", onVal, offVal);
	
	return 0;
} // main
