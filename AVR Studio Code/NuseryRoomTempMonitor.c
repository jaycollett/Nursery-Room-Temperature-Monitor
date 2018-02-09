// CORE is running at 1.0Mhz 
#define F_CPU 1000000

/****************************************************************
 * Specify includes for the program
 ****************************************************************/
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <math.h>

/****************************************************************
 * Define all consts
 ****************************************************************/
#define LED_RED 	PINB0
#define LED_BLUE 	PINB2
#define LED_GREEN 	PINB1


/****************************************************************
 * Method to call _delay_ms multiple times to ensure a proper long delay
 * Assumptions: Clock is set correctly and delay.h is included
 * Input: int delay in seconds
 ****************************************************************/
void delay_seconds(int Seconds){
	int i = 0;
	int timesToLoop;
	timesToLoop = (Seconds * 10);
	for(i=0;i<timesToLoop;i++){
		_delay_ms(100);
	}
}


/****************************************************************
 * Method to blink the LED on and off pausing 200ms between
 * Assumptions: All LED output pins will be on PORTB
 * Input: int blinks: number of times to blink LED
 * 		  int ledPin: the output pin number to use for LED
 ****************************************************************/
void blinkLEDs(int blinks,int ledPin){
	int t;
	for(t=0;t<blinks;t++){
		PORTB |= (1<<ledPin);
		_delay_ms(200);
		PORTB &= ~(1<<ledPin);
		_delay_ms(200);
	}
}


/****************************************************************
 * Method to initalize the ADC on the ATTINY85
 * Assumptions: ADC readings will only be on PINB3
 *				AVR will have a freq of 1.0Mhz
 *				Will use VCC as reference
 * Input: NONE
 ****************************************************************/
void ADC_init(void){
	ADMUX = (1<<MUX1); // set PB4 as input
	ADCSRA = (1<<ADPS1) | (1<<ADPS0); // set adc to divide by 8 1.0Mhz / 8 = 125khz
}


/**********************************************************************
 * Method to enable ADC, read value of pin, disable ADC, return value
 * Assumptions: ADC_init() has been called to cofigure ADC
 *				Always want to sample 8 times and average
 *				Always enable and disable ADC with each read
 * Input: NONE
 **********************************************************************/
int ADC_read(void)
{
	int i;
	int ADC_temp;
	
	int ADCr = 0;
	
	ADCSRA = (1<<ADEN); // enable the ADC
	
	//do a throw-away readout first
	ADCSRA |= (1<<ADSC); // do single conversion
	
	// wait for conversion done, ADIF flag active
	while(!(ADCSRA & 0x10));
	
	// do the ADC conversion 8 times for better accuracy
	for(i=0;i<8;i++)
	{
		ADCSRA |= (1<<ADSC); // do single conversion
		// wait for conversion done, ADIF flag active
		while(!(ADCSRA & 0x10));

		ADC_temp = ADCL; // read out ADCL register
	
		ADC_temp += (ADCH << 8); // read out ADCH register
	
		// accumulate result (8 samples) for later averaging
		ADCr += ADC_temp;
	}

	ADCr = ADCr >> 3; // average the 8 samples
	ADCSRA = (0<<ADEN); // disable the ADC

	return ADCr;
}


/****************************************************************
 * Standard/Required main method
 ****************************************************************/
int main(void){

	// setup our output pins, we take care of setting up the input analog pin
	// for the ADC in the ADC_init method.
	DDRB = (1<<LED_RED) | (1<<LED_BLUE) | (1<<LED_GREEN);

	// set out output pins HIGH so we don't turn on the LEDs
	PORTB = (1<<LED_BLUE) | (1<<LED_RED) | (1<<LED_GREEN);

	float KelvinC = 273.15;
	float adcValue;
	float _kelvin;
	float _fahrenheit;

	ADC_init();

	// loop forever
	while(1){
		adcValue = 0;

		// get ten sequential readings to help smooth out temp readings
		// may implement a low pass filter here...
		for(int i = 0; i < 16; i++){
			adcValue += ADC_read(); // read temp sensor value
			delay_seconds(1);
		}
		adcValue = adcValue/16;

		// convert our ADC value into C or F, I need F.
		// 5.134 is the actual input voltage for my uC as 
		// measured on my DMM.
		_kelvin = (((adcValue / 1023) * 5.134) * 100);
		_fahrenheit = ((_kelvin - KelvinC) * (1.8)) + 32;

		// we want the room to be between 65 and 71 degrees fahrenheit.

		if(_fahrenheit > 71){
			PORTB |= (1<<LED_BLUE) | (1<<LED_GREEN); // set blue and green led high to turn them off
			if(_fahrenheit >= 74){
				blinkLEDs(10,LED_RED);	// blink red led 10 times
			}
			PORTB &= ~(1<<LED_RED); // turn on red led
		}
		if(_fahrenheit < 65){
			PORTB |= (1<<LED_RED) | (1<<LED_GREEN); // set green and red high to turn them off
			if(_fahrenheit <= 62){
				blinkLEDs(10,LED_BLUE);	// blink blue led 10 times
			}
			PORTB &= ~(1<<LED_BLUE);// turn on blue led
		}
		if( (_fahrenheit >= 65) && (_fahrenheit <= 71))
		{
			// turn on all LEDs, white for "all is ok"
			PORTB &= ~(1<<LED_GREEN);
			PORTB &= ~(1<<LED_RED);
			PORTB &= ~(1<<LED_BLUE);
		}

	}
}
