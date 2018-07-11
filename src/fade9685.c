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
#include <math.h>
#include <argp.h>

#include <PCA9685.h>

unsigned int verbose = 0;

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
	printf("  -d\tSet Duty Cycle Instantly (0 - 100)\n");
	printf("  -l\tFade to Luminosity (0 - 100)\n");
	printf("  -r\tMake luminosity human friendly using Rec 709 visual gradient (0 - 100)\n");
	printf("  -s\tStep (Larger value fades more quickly)\n");
	printf("  -b\tBus number (default 1)\n");
	printf("  -a\tAddress (Default 0x42)\n");
	printf("  -c\tChannel (0 - 15) Can be repeated for multiple channels or -1 for all\n");
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
	{ "luminosity", 'l', "LUMINOSITY", 0, "Luminosity (0 - 100)" },
	{ "rec709", 'r', 0, 0, "Use Rec709 to make luminosity eye-friendly" },
	{ "step", 's', "STEP", 0, "Fade Rate step" },
	{ "channel", 'c', "CHANNEL", 0, "Channel (0-15 or -1 for all)" },
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
	unsigned int frequency, channels, bus, address, verbose, reset, rec709;
	int step;
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
			// Adjust luminosity % to real value
			// TODO: Add the human perception adjustment
			// Do this right... 
			//arguments->luminosity = arguments->luminosity / 100.0f * 4095;
			break;
		case 'r':
			arguments->rec709 = 1;
		case 's':
			arguments->step = atoi( arg );
			break;
		// --channel can be used multiple times and values will be bitwise ANDed to the channels variable
		case 'c':
			if ( atoi( arg ) == -1 )
				arguments->channels = 0xffff;
			else
				arguments->channels = arguments->channels | ( 1 << atoi ( arg ) );
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



/**********   PWM Functions *********/


unsigned int getChannelReg( unsigned int channels, unsigned int channelNum )
{
	int c = channels & ( 1 << channelNum );
	return c * 4 + _PCA9685_BASEPWMREG;
}

unsigned int luminosityToVal( float lum )
{
	unsigned int l = lum / 100.0f * 4095;
	return l;
}


/***** Adjust luminosity % to a value that our eyes agree with using Rec 709 adjustment  *****/
/**	Reasoning: Our eyes do not perceive the brightness in a linear fashion
 * 	and the difference between 1% and 2% appears greater than the difference between 90% and 100%
**/
unsigned int luminosityToVisualVal( float lum )
{
	// I like the math when the lum is 0-1 rather than %
	lum = lum / 100.0f;

	unsigned int l;
	if ( lum < 0.081f )
		lum = lum / 4.5;
	else
		lum = pow( (( lum + 0.099f ) / 1.099f ), ( 1.0f / 0.45f ) );

	l = lum / 100.0f * 4095.0f;

	return l;
}


// fadePWM  Fade channels to a given luminosity   TODO: Add Rec 709 luminosity adjustment
// For fist tests luminosity is simply 0-4095
int fadePWM( unsigned int fd, unsigned int address, unsigned int channels, float luminosity, unsigned int step )
{
	// REDO
	// 1. Get all current values
	// 2. check which is farthest from setpoint
	// 3. increase/decrease each until they are at the setpoint (start loop at "farthest")
	
	unsigned int oldOnVal, oldOffVal, channelReg;
	unsigned int lowestVal;
	unsigned int highestVal;
	unsigned int lowestChan, highestChan, newSetPoint;
	int ret;
	unsigned int currentVals [ _PCA9685_CHANS ];

	unsigned int setOnVals[_PCA9685_CHANS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	//newSetPoint = (unsigned int) ( luminosity /100.0f * 4095.0f );
	newSetPoint = luminosityToVal(luminosity);
	lowestVal = newSetPoint;
	highestVal = newSetPoint;

	// Get current values
	ret = PCA9685_getPWMVals( fd, address, &setOnVals, &currentVals );

	// Find the value that is farthest from setpoint
	for ( unsigned int i = 0; i < _PCA9685_CHANS; i++ )
	{
		if ( channels & ( 1 << i ) )
		{
			if ( currentVals[i] > highestVal )
			{
				highestVal = currentVals[i];
				highestChan = i;
			}

			if ( currentVals[i] < lowestVal )
			{
				lowestVal = currentVals[i];
			lowestChan = i;
			}
		} //if channels
	}


	// fade them in/out. Need to account for situations like setting to 50% when some are higher and some are lower
	// Find the ones that are most greater and lesser
	unsigned int farthest = highestVal;
	if ( ( newSetPoint - lowestVal ) > (highestVal - newSetPoint ) )
	{
		farthest = lowestVal;
	}

	// fade all channels towards the target
	fprintf( stderr,"channels: %d  newSetPoint: %d  farthest: %d   %d\n", channels, newSetPoint, farthest, abs(newSetPoint - farthest));
	for ( unsigned int n = 0; n <= abs( newSetPoint - farthest ); n += step )
	{
		//fprintf( stderr, "\n%d\n", n);
		//
		// check and modify current settings from arrays
		// then setAll on each iteration
		//
		// Loop through each output, setting value if its value is farther than the current setting
		for ( unsigned int i = 0; i < _PCA9685_CHANS; i++ )
		{
			if ( channels & ( 1 << i ) )
			{
				if ( ( currentVals[i] - luminosity ) > 0 )
				{
					if ( currentVals[i] >= step )
						currentVals[i] -= step;
					else
						currentVals[i] = luminosity;
				}

				if( ( luminosity - currentVals[i] ) > 0 )
				{
					if ( currentVals[i] <= ( 4095 - step ) )
						currentVals[i] += step;
					else
						currentVals[i] = luminosity;
				}
			}  // if channels
			//fprintf ( stderr, "%d\t", currentVals[i]);
		} // for i

		// TODO: Set each one at final value - Is it required??


		// Set all outputs
		ret = PCA9685_setPWMVals(fd, address, setOnVals, currentVals);

				
	}  // for


}	// fadePWM

int setDutyCycle( unsigned int fd, unsigned int address, unsigned int channels, float dutycycle )
{
	// set the duty cycle of channels instantly
	unsigned int o = (int) ( dutycycle / 100.0f * 4095.0f );
	unsigned int bit = 1;
	unsigned int setOnVals[_PCA9685_CHANS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int setOffVals[_PCA9685_CHANS];
	unsigned int channelReg, offVal;
	int ret;


	// Get the current values 
	ret = PCA9685_getPWMVals( fd, address, &setOnVals, &setOffVals );

	for ( unsigned int i = 0; i < _PCA9685_CHANS; i++ )
	{
		if ( channels & ( 1 << i ) )
		{
			setOffVals[i] = o;
		}
	}

	ret = PCA9685_setPWMVals( fd, address, &setOnVals, &setOffVals );

	return 0;

}  // setDutyCycle


int main(int argc, char **argv) {
	_PCA9685_DEBUG = 0;
	_PCA9685_MODE1 = 0x00 | _PCA9685_ALLCALLBIT;
	int opt, ret, fd;
	unsigned int onVal = 0;
	unsigned int offVal = 0;
	unsigned int oldOnVal, oldOffVal;
	unsigned int channels = 0;	// Channel register is channel * 4 + 6
	unsigned int channelReg;
	unsigned int bus = 1;
	unsigned int address = 0x40;
	unsigned int reset = 0;
	unsigned int frequency = _PCA9685_MAXFREQ;
	unsigned int rec709 = 0;
	int step = 1;
	float luminosity, dutycycle;

	char msg[256];
	int val;

	struct arguments arguments;

	/* Default values. */
	arguments.dutycycle = -1.0f;
	arguments.luminosity = -1.0f;
	arguments.rec709 = 0;
	arguments.frequency = 1526;
	arguments.step = 20;
	arguments.channels = 0;
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
	step = arguments.step;
	channels = arguments.channels;
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
	

	if ( luminosity >= 0.0f )
	{
		ret = fadePWM( fd, address, channels, luminosity, step );
		exit(1);
	}

	if ( dutycycle >= 0.0f )
	{
		// When setting duty cycle, set instantly
		ret = setDutyCycle( fd, address, channels, dutycycle );
		exit(0);
	}

	// Set the duty cycle on and off values
	offVal = dutycycle;

	snprintf( msg, 256, "Bitmask of channels: %04x\n", channels );
	printLog( msg, verbose, 4 );

	// DEPRECATED channelReg = channel * 4 + _PCA9685_BASEPWMREG;

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
