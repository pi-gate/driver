// sx1276_server.cpp
//
// Example program showing how to use RH_RF95 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM95 module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/pi-gate
// make
// sudo ./sx1276_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <RH_PI-GATE.h>


// Now we include GateDefinitions.h so this will expose defined 
// constants with CS/IRQ/RESET/on board LED pins definition
#include "GateDefinitions.h"

// Our pi-gate Configuration 
//#define RF_FREQUENCY  433.00
#define RF_FREQUENCY  868.00
#define RF_GATEWAY_ID   1

// Create an instance of a driver
// for 433MHz
//RH_SX1276 rf433(RF433_CS_PIN, RF433_IRQ_PIN, RF433_RST_PIN);

// for 868MHz
RH_SX1276 rf1276(RF868_CS_PIN, RF868_IRQ_PIN, RF868_RST_PIN, RF868_TXE_PIN);


//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
  printf("\n%s Break received, exiting!\n", __BASEFILE__);
  force_exit=true;
}

//Main Function
int main (int argc, const char* argv[] ){
  unsigned long led_blink = 0;

  signal(SIGINT, sig_handler);
  printf( "%s\n", __BASEFILE__);


  if (!bcm2835_init()) {
    fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
    return 1;
  }

    printf( "433Mhz Gate CS=GPIO%d", RF433_CS_PIN);
    printf( ", IRQ=GPIO%d", RF433_IRQ_PIN );
    printf( ", RST=GPIO%d", RF433_RST_PIN );
    printf( ", TXE=GPIO%d", RF433_TXE_PIN );
    
    printf( "868Mhz Gate CS=GPIO%d", RF868_CS_PIN);
    printf( ", IRQ=GPIO%d", RF868_IRQ_PIN );
    printf( ", RST=GPIO%d", RF868_RST_PIN );
    printf( ", TXE=GPIO%d", RF868_TXE_PIN );


  if (!rf1276.init()) {
    fprintf( stderr, "\nSX1276 module init failed, Please verify wiring/module\n" );
  } else {
    // Since we may check IRQ line with bcm_2835 Rising edge detection
    // In case radio already have a packet, IRQ is high and will never
    // go to low so never fire again 
    // Except if we clear IRQ flags and discard one if any by checking
    rf1276.available();

    // Now we can enable Rising edge detection
    bcm2835_gpio_ren(RF868_IRQ_PIN);

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
    // you can set transmitter powers from 5 to 23 dBm:
    //  driver.setTxPower(23, false);
    // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
    // transmitter RFO pins and not the PA_BOOST pins
    // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true. 
    // Failure to do that will result in extremely low transmit powers.
    // rf1276.setTxPower(14, true);


    // RF95 Modules don't have RFO pin connected, so just use PA_BOOST
    // check your country max power useable, in EU it's +14dB
    rf1276.setTxPower(14, false);

    // You can optionally require this module to wait until Channel Activity
    // Detection shows no activity on the channel before transmitting by setting
    // the CAD timeout to non-zero:
    //rf1276.setCADTimeout(10000);

    // Adjust Frequency
    rf1276.setFrequency(RF_FREQUENCY);
    
    // If we need to send something
    rf1276.setThisAddress(RF_GATEWAY_ID);
    rf1276.setHeaderFrom(RF_GATEWAY_ID);

    // Be sure to grab all node packet 
    // we're sniffing to display, it's a demo
    rf1276.setPromiscuous(true);

    // We're ready to listen for incoming message
    rf1276.setModeRx();

    printf( " OK NodeID=%d @ %3.2fMHz\n", RF_GATEWAY_ID, RF_FREQUENCY );
    printf( "Listening packet...\n" );

    //Begin the main body of code
    while (!force_exit) {

      // We have a IRQ pin ,pool it instead reading
      // Modules IRQ registers from SPI in each loop

      // Rising edge fired ?
      if (bcm2835_gpio_eds(RF868_IRQ_PIN)) {
        // Now clear the eds flag by setting it to 1
        bcm2835_gpio_set_eds(RF868_IRQ_PIN);
        //printf("Packet Received, Rising event detect for pin GPIO%d\n", RF_IRQ_PIN);

        if (rf1276.available()) { 

          // Should be a message for us now
          uint8_t buf[RH_SX1276_MAX_MESSAGE_LEN];
          uint8_t len  = sizeof(buf);
          uint8_t from = rf1276.headerFrom();
          uint8_t to   = rf1276.headerTo();
          uint8_t id   = rf1276.headerId();
          uint8_t flags= rf1276.headerFlags();;
          int8_t rssi  = rf1276.lastRssi();

          if (rf1276.recv(buf, &len)) {
            printf("Packet[%02d] #%d => #%d %ddB: ", len, from, to, rssi);
            printbuffer(buf, len);
            printf("\n" );

            // Send a message to rf95_server
            uint8_t data[] = "And hello back to you";
            uint8_t len = sizeof(data);
    	    // Where we're sending packet
            bcm2835_delay(500);
    	      rf1276.setHeaderTo(from);
    	      rf1276.setHeaderFrom(RF_GATEWAY_ID);
            rf1276.send(data, len);
            rf1276.waitPacketSent();
 
            printf("Sending %02d bytes to node #%d => ", len, rf1276.headerFrom() );
            printbuffer(data, len);
            printf("\n" );
          } else {
            Serial.print("receive failed\n");
          }
          printf("\n");
        }

      }
 
      // Let OS doing other tasks
      // For timed critical appliation you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(1);
    }
  }

  printf( "\n%s Ending\n", __BASEFILE__ );
  bcm2835_close();
  return 0;
}

