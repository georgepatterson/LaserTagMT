// IRsend function
boolean SonyIR(unsigned long data, int nbits) {
  if(bitstx == 0) { //if IDLE then transmit
    timertx=0; //reset timer
    resettx=96; //initial header pulse is 2400us long. 2400/25us ticks = 96
    spacetx = false;
    datatx=data << (32 - (nbits + 1)); //unsigned long is 32 bits. data gets   shifted so that the leftmost (MSB) will be the first of the 32.
    TCCR2A |= _BV(COM2B1); // Enable pin 3 PWM output for transmission of header
    bitstx=nbits + 1; //bits left to transmit, including header

    return true;
  }
  else {
    return false;
  }
}

