
#include <SPI.h>
#include "rotating_buffer.h"

static const int TX_REQUEST_PIN = 14;
static const unsigned RX_BUFFER_SIZE = 512;

String strBuffer;

// Needs to be interrupt safe.
typedef buffer_type(char, RX_BUFFER_SIZE) RxBufferType;
static RxBufferType rxBuffer;
static volatile unsigned rxBufferOverrunCount = 0;

void setup (void) {
  Serial.begin(115200);   // debugging
  Serial.println("Starting Arduino SPI Slave.");

  buffer_init(rxBuffer, RX_BUFFER_SIZE);

  pinMode(TX_REQUEST_PIN, OUTPUT);
  digitalWrite(TX_REQUEST_PIN, LOW);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);

  /*  Enable SPI.
      SPCR0:
      Bit 7 – SPIE0: SPI0 Interrupt Enable
      Bit 6 – SPE0: SPI0 Enable
      Bit 5 – DORD0: Data0 Order
          0 = the MSB of the data word is transmitted first.
          1 = the LSB of the data word is transmitted first.
      Bit 4 – MSTR0: Master/Slave0 Select
          0 = Slave
          1 = Master
      Bit 3 – CPOL0: Clock0 Polarity
          0 = SCK is low when idle
          1 = SCK is high when idle
      Bit 2 – CPHA0: Clock0 Phase
          0 = data is sampled on the leading edge of SCK
          1 = data is sampled on the trailing edge of SCK
      Bits 1:0 – SPR0n: SPI0 Clock Rate Select n [n = 1:0]
  */
  // MSB First,
  // SCK is low when idle,
  // data is sampled on the leading edge of SCK
  SPCR = (1<<SPE);
  //SPCR |= bit(SPE);
  //SPCR |= _BV(SPE);

  // now turn on interrupts
  SPI.attachInterrupt();
  //SPCR |= bit(SPIE);
}


// SPI interrupt routine
ISR (SPI_STC_vect) {
  char rxChar = SPDR;  // read from SPI Data Register

  if (is_buffer_full(rxBuffer)) {
    //TODO: FAIL!
    ++rxBufferOverrunCount;
  } else {
    buffer_write(rxBuffer, rxChar);
  }

  // Transmit back to the Master.
  SPDR = 0;
}


// main loop - wait for flag set in interrupt routine
void loop (void) {
  if (is_buffer_empty(rxBuffer)) {
    // Nothing to do yet.
  } else {
    char tmpCh;
    buffer_read(rxBuffer, tmpCh);

    if (tmpCh) {
      // Append the char just received.
      strBuffer += tmpCh;
    } else {
      // End-of-string.
      Serial.println(strBuffer);
      strBuffer = "";
    }
  }
}
