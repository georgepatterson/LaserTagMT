void SetPWM(int freq){
  //Freq is in kiloHertz either, 38 or 56 

  switch (freq) {
  case 38:
    // Add code to set the parameters for a 36kHz carrier frequency.

    break; 
  case 56:
    // Add code to set the parameters for a 56kHz carrier frequency.

    break;
  default:
    Serial.print("Invalid Frequency");
    Serial.print(freq);
    Serial.println("Please recheck");

  }
}

// IRsend function
boolean SonyIR(unsigned long data, int nbits) {
  if(bitstx == 0) { //if IDLE then transmit
    timertx=0; //reset timer
    resettx=96; //initial header pulse is 2400us long. 2400/25us ticks = 96
    spacetx = false;
    // unsigned long is 32 bits. data gets shifted so
    //    that the leftmost (MSB) will be the first of the 32.
    datatx=data << (32 - (nbits + 1)); 
    TCCR2A |= _BV(COM2B1); // Enable pin 3 PWM output for transmission of header
    bitstx=nbits + 1; //bits left to transmit, including header

    return true;
  }
  else {
    return false;
  }
}





