// multiserver.cpp
//
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/multi_server
// make
// sudo ./multi_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon
// Contributed by Gernot Lenz based on sample multiserver by Charles-Henri Hallard

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <RHGenericDriver.h>
#include <RH_PI-GATE.h>

#include "GateDefinitions.h"

// Our 433 gate Configuration
#define RF433_NODE_ID    1
#define RF433_FREQUENCY  433.00

// Our 868 gate Configuration
#define RF868_NODE_ID    1
#define RF868_FREQUENCY  868.00

// Our send timer
#define timer_MS_Gate1  30000
#define timer_MS_Gate2  30000

// Gate index table
enum Gate_idx {
	GATE433 = 0, GATE868 = 1
};

// Gates table index
#define GATE_COUNT 2
uint8_t IRQ_pins[GATE_COUNT] = { RF433_IRQ_PIN, RF868_IRQ_PIN };
uint8_t RST_pins[GATE_COUNT] = { RF433_RST_PIN, RF868_RST_PIN };
uint8_t CSN_pins[GATE_COUNT] = { RF433_CS_PIN, RF868_CS_PIN };
uint8_t TXE_pins[GATE_COUNT] = { RF433_TXE_PIN, RF868_TXE_PIN };

const char * Gate_name[] = { "1 RF_433", "2 RF_868" };
float Gate_freq[GATE_COUNT] = { RF433_FREQUENCY, RF868_FREQUENCY };
uint8_t Gate_id[GATE_COUNT] = { RF433_NODE_ID, RF868_NODE_ID };

// Pointer table on radio driver
RHGenericDriver * drivers[GATE_COUNT];

// Create an instance of a driver for 3 modules
// In our case RadioHead code does not use IRQ
// callback, bcm2835 does provide such function,
// but keep line state providing function to test
// Rising/falling/change on GPIO Pin
RH_SX1276 rf433(RF433_CS_PIN, RF433_IRQ_PIN, RF433_RST_PIN);
RH_SX1276 rf868(RF868_CS_PIN, RF868_IRQ_PIN, RF868_RST_PIN, RF868_TXE_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = 0;

/* ======================================================================
 Function: sig_handler
 Purpose : Intercept CTRL-C keyboard to close application
 Input   : signal received
 Output  : -
 Comments: -
 ====================================================================== */
void sig_handler(int sig) {
	printf("\nBreak received, exiting!\n");
	force_exit = true;
}
/* ======================================================================
 Function: returns the filename minus the extension
 Purpose : pretty
 Input   : filename and ext
 Output  : filename without ext
 Comments: -
 ====================================================================== */
char * get_basefilename(char * fname) {
#define MAXFNAME 128

	char * ext;
	int i, j;
	ext = (char *)malloc(sizeof(char) * MAXFNAME);

	for ( i = strlen(fname)+1; i > 0; i--)
	{
		if (fname[i] == '.')
		{
			for (j = 0; j < i; j++)
			{
				ext[j] = fname[j];
			}
			ext[i] = '\0';
			i = 0;
		}
	}
	return ext;
}

/* ======================================================================
 Function: getReceivedData
 Purpose : Get module received data and display it
 Input   : Module Index from modules table
 Output  : -
 Comments: -
 ====================================================================== */
void getReceivedData(uint8_t index) {
	RHGenericDriver * driver = drivers[index];
	if (driver->available()) {
		// RH_SX1276_MAX_MESSAGE_LEN is > RH_RF69_MAX_MESSAGE_LEN,
		// So we take the maximum size to be able to handle all
		uint8_t buf[RH_SX1276_MAX_MESSAGE_LEN];
		uint8_t len = sizeof(buf);
		uint8_t from = driver->headerFrom();
		uint8_t to = driver->headerTo();
		uint8_t id = driver->headerId();
		uint8_t flags = driver->headerFlags();
		;
		int8_t rssi = driver->lastRssi();

		if (driver->recv(buf, &len)) {
			time_t timer;
			char time_buff[16];
			struct tm* tm_info;

			time(&timer);
			tm_info = localtime(&timer);

			strftime(time_buff, sizeof(time_buff), "%H:%M:%S", tm_info);
			printf("%s Gate %s [len %02d] from #%d to #%d %ddB: ", time_buff, Gate_name[index], len, from, to, rssi);
			printbuffer(buf, len);
		} else {
			printf("receive failed");
		}
		printf("\n");
	}
}

/* ======================================================================
 Function: initRadioModule
 Purpose : initialize a gate
 Input   : Gate Index from gates table
 Output  : -
 Comments: -
 ====================================================================== */
bool initRadioModule(uint8_t index) {
	RHGenericDriver * driver = drivers[index];

	// Gate Information
	printf("%s (CS=GPIO%d, IRQ=GPIO%d, RST=GPIO%d, TXE=GPIO%d)", Gate_name[index], CSN_pins[index], IRQ_pins[index], RST_pins[index], TXE_pins[index]);

	if (!driver->init()) {
		fprintf( stderr, "\n%s init failed, Please verify wiring/module\n", Gate_name[index]);
	} else {
		// Since we'll check IRQ line with bcm_2835 Rising edge detection
		// In case radio already have a packet, IRQ is high and will never
		// go to low until cleared, so never fire IRQ LOW to HIGH again
		// Except if we clear IRQ flags and discard packet if any!
		// ==> This is now done before reset
		driver->available();
		// now we can enable rising edge detection
		bcm2835_gpio_ren(IRQ_pins[index]);

		// set Node ID
		driver->setThisAddress(Gate_id[index]); // filtering address when receiving
		driver->setHeaderFrom(Gate_id[index]);  // Transmit From Node

		// Get all frame, we're in demo mode
		driver->setPromiscuous(true);

		// We need to check module type since generic driver does
		// not expose driver specific methods
		switch (index) {
		case GATE433:
			// check your country max power useable, in EU it's +14dB
			((RH_SX1276 *) driver)->setTxPower(14, false);
			// Adjust Frequency
			((RH_SX1276 *) driver)->setFrequency(Gate_freq[index]);

			//rf868.setCADTimeout(10000);
#ifdef DEBUG
			((RH_SX1276 *) driver)->printRegisters();
#endif
			break;
		case GATE868:
			// check your country max power useable, in EU it's +14dB
			((RH_SX1276 *) driver)->setTxPower(14, false);
			// Adjust Frequency
			((RH_SX1276 *) driver)->setFrequency(Gate_freq[index]);

			//rf868.setCADTimeout(10000);
#ifdef DEBUG
			((RH_SX1276 *) driver)->printRegisters();
#endif
			break;
		}

		printf(" OK!, NodeID=%d @ %3.2fMHz\n", Gate_id[index], Gate_freq[index]);

		return true;
	}

	return false;
}

/* ======================================================================
 Function: main
 Purpose : not sure ;)
 Input   : command line parameters
 Output  : -
 Comments: -
 ====================================================================== */
int main(int argc, const char* argv[]) {
	unsigned long timer_set[GATE_COUNT] = { 0, 0 };

	// caught CTRL-C to do clean-up
	signal(SIGINT, sig_handler);

	// Display app name
	printf("%s\n", get_basefilename(basename(strdup((char *) __FILE__))));
	for (uint8_t i = 0; i < strlen(get_basefilename(basename(strdup((char *) __FILE__)))); i++) {
		printf("=");
	}
	printf("\n");

	// Init GPIO bcm
	if (!bcm2835_init()) {
		fprintf( stderr, "bcm2835_init() Failed\n\n");
		return 1;
	}

	//save driver instances pointer
	drivers[GATE433] = &rf433;
	drivers[GATE868] = &rf868;

	// configure all gates I/O CS pins to 1 before anything else
	// to avoid any problem with SPI sharing
	for (uint8_t i = 0; i < GATE_COUNT; i++) {
		// CS Ping as output and set to 1
		pinMode(CSN_pins[i], OUTPUT);
		digitalWrite(CSN_pins[i], HIGH);
	}

	// configure all gates I/O pins
	for (uint8_t i = 0; i < GATE_COUNT; i++) {
		// configure all gates
		if (!initRadioModule(i)) {
			force_exit = true;
		}
	}

	// All init went fine, continue specific init if any
	if (!force_exit) {
		// Set all gates in receive mode
		printf("Set all modules in receive mode\n");
		rf433.setModeRx();
		rf868.setModeRx();
		printf("Listening and sending packets...\n");
	}

	uint8_t data433[] = "## 433 alive ##";
	uint8_t len433 = sizeof(data433);
	uint8_t data868[] = "## 868 alive ##";
	uint8_t len868 = sizeof(data868);
	timer_set[0] = millis();

	// Begin the main loop code
	// ========================
	while (!force_exit) {

		// Loop thru gates
		for (uint8_t idx = 0; idx < GATE_COUNT; idx++) {

			// Rising edge fired ?
			if (bcm2835_gpio_eds(IRQ_pins[idx])) {
				// Now clear the eds flag by setting it to 1
				bcm2835_gpio_set_eds(IRQ_pins[idx]);

				getReceivedData(idx);
			} // has IRQ

			getReceivedData(idx);
		} // For Modules

		// Send message in 433Mhz net
		if (millis() - timer_set[0] > timer_MS_Gate1) {
			rf433.setHeaderTo(1); // set receiver address
			bool has_sent_out = rf433.send(data433, len433);
			if (has_sent_out)
				rf433.waitPacketSent();
			timer_set[0] = millis();
		} // timer expired

		// Send message in 868Mhz net
		if (millis() - timer_set[1] > timer_MS_Gate2) {
			rf868.setHeaderTo(1);  // set receiver address
			bool has_sent_out = rf868.send(data868, len868);
			if (has_sent_out)
				rf868.waitPacketSent();
			timer_set[1] = millis();
		} // timer expired

		// Let OS doing other tasks
		// For timed critical appliation receiver, you can reduce or delete
		// this delay, but this will charge CPU usage, take care and monitor
		bcm2835_delay(5);
	}

	// We're here because we need to exit, do it clean
	printf("\n%s, done!\n", basename(strdup((char *) __FILE__)));
	bcm2835_close();
	return 0;
}
