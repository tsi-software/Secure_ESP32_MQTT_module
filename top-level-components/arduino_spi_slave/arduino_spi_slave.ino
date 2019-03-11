
//#include <SPI.h>
#include "rotating_buffer.h"

static const int TX_REQUEST_PIN = 14;
static const unsigned TX_BUFFER_SIZE = 128; // MUST be less than 255!!!
static const unsigned RX_BUFFER_SIZE = 128; // MUST be less than 255!!!
static const int DEBUG_LED = 17;

unsigned long lastPing = 0;
int spiRxStatus = 0;
bool waitingForFirstSpiRx = true;
bool ledState = 0;
String strBuffer;

// Needs to be interrupt safe.
typedef buffer_type(char, TX_BUFFER_SIZE) TxBufferType;
static TxBufferType txBuffer;

// Needs to be interrupt safe.
typedef buffer_type(char, RX_BUFFER_SIZE) RxBufferType;
static RxBufferType rxBuffer;

static volatile unsigned rxBufferOverrunCount = 0;


void setup (void) {
  Serial.begin(115200/2);   // divide by 2 because we are running a 5v 16mhz Arduino Pro Mini at 3.3v 8mhz
  //Serial.begin(115200);   // debugging
  Serial.println("Starting Arduino SPI Slave.");

  lastPing = millis();
  pinMode(DEBUG_LED, OUTPUT);

  buffer_init(txBuffer, TX_BUFFER_SIZE);
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
  //SPCR = (1<<SPE);
  //SPCR |= bit(SPE);
  //SPCR |= _BV(SPE);

  // now turn on interrupts
  //SPI.attachInterrupt();
  //SPCR |= bit(SPIE);
  //SPCR |= _BV(SPIE);

  SPCR = (SPCR & 0b11) | _BV(SPE) | _BV(SPIE);
  Serial.print("SPCR:0x");
  Serial.println(String(SPCR, BIN));
}


// SPI interrupt routine
ISR (SPI_STC_vect) {
  char rxChar = SPDR;  // read from SPI Data Register

  if (is_buffer_full(rxBuffer)) {
    //TODO: FAIL!
    ++rxBufferOverrunCount;
    spiRxStatus = -1;
  } else {
    // Echo the received character to the Serial Tx.
    buffer_write(rxBuffer, rxChar);
    //spiRxStatus = rxChar;
  }

  // Transmit back to the SPI Master.
  if (is_buffer_empty(txBuffer)) {
    SPDR = 0;
  } else {
    buffer_read(txBuffer, SPDR);
    if (is_buffer_empty(txBuffer)) {
      digitalWrite(TX_REQUEST_PIN, LOW);
    }
  }
}


static void ping(void) {
  unsigned long thisPing = millis();
  
  if (thisPing >= lastPing + 4*1000) {
    lastPing = thisPing;
    
    ledState = !ledState;
    digitalWrite(DEBUG_LED, ledState);

    // JUST TESTING...
    //buffer_safe_write(rxBuffer, '.');
    //buffer_safe_write(rxBuffer, '\0');
    //
    //unsigned dataSize = 0;
    //buffer_data_size(rxBuffer, dataSize);
    //Serial.print("ping - buffer=");
    //Serial.println(dataSize);
  }
}


static void subscribe(void) {
  char *msg = "Hello SPI.";
  for (int ndx = 0; ndx < strlen(msg)+1; ++ndx) {
    buffer_safe_write(txBuffer, msg[ndx]);
  }
  digitalWrite(TX_REQUEST_PIN, HIGH);
  Serial.print("SPI subscribe: ");
  Serial.println(msg);
}


// main loop - wait for flag set in interrupt routine
void loop (void) {
  ping();

  // Debugging.
  if (spiRxStatus) {
    Serial.print("spiRxStatus:");
    Serial.println(spiRxStatus);
    spiRxStatus = 0;
  }

  if (is_buffer_empty(rxBuffer)) {
    // Nothing to do yet.
  } else {
    char tmpCh;
    buffer_read(rxBuffer, tmpCh);

    if (tmpCh != 0) {
      //Serial.print(strBuffer);
      //Serial.print('-');
      //Serial.println(tmpCh);
      strBuffer += tmpCh; // Append the char just received.
    }

    if (tmpCh == 0) {
      // End-of-string.
      if (strBuffer.length() > 0) {
        Serial.println(strBuffer);
        strBuffer = "";
      }
    }

    if (waitingForFirstSpiRx) {
      waitingForFirstSpiRx = false;
      subscribe();
    }
  }
}
