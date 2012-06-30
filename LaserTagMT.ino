/////////////////////
// GAME VARIABLES  //
/////////////////////

int life = 100;
unsigned long elapsed = 0; // stores millis() function
unsigned long lastshot = 0 ; // time at which last shot occurred
boolean reloadreleased = true; // whether or not button for reloading is released
unsigned long lastreload = 0; //debouncer for reload button

// bullets for each magazine
unsigned int magazines[] = {
  30,30,30,30
};


unsigned long ircode = 0;     // gun-specific code
int temp = 0;                // temp variable, used for multiple temp stuff
unsigned int timertone = 0;  // timer counter used for frequency
unsigned int counttone = 0;  // counter for the lenght
unsigned int resettone = 0;  // reset value for above timer   sound_freq = 40000 / (reset*2)
unsigned int lenghttone = 0; // wanted lenght of tone
boolean triggerreleased = true; // tells whether or not the sound is playing already
boolean barrelbullet = false; // tells whether or not a bullet is in the barrel
int selector = 0; // 0 = safe, 1 = single, 2 = full auto
int magazine_used = 0;

#define rofdelay 150 // min milliseconds between consecutive shots
#define damage 20    // damage per bullet
#define team 0       // 2 TEAM bits so: team 00, team 01, team 10, team 11
#define player 0     // 5 PLAYER bits so: 0 to 31
#define magazine_pin 5 // ADC pin for reading mag resistor

// PROTOCOL: header + "0" + 5 PLAYER bits + 2 TEAM bits + 6 DAMAGE bits + 2 PARITY bits

#define sbi(port,bit) (port)|=(1<<(bit))
#define CLR(x,y) (x&=(~(1<<y)))
#define SET(x,y) (x|=(1<<y))



// TX_RX const

#define STATE_IDLE     2
#define STATE_MARK     3
#define STATE_SPACE    4
#define STATE_STOP     5
#define STATE_HDR      6
#define MARK  0
#define SPACE 1
#define TOPBIT 0x80000000

// RX variables
#define tsoppin 2          // pin for IR data from detector
int pinrx;                 // reading from that pin
int staterx = STATE_IDLE;  // state machine
int bitsrx=0;              // number of bits received
unsigned int timerrx;      // state timer, counts 25uS ticks.
unsigned long datarx = 0;  // result will be stored here

// TX variables
unsigned int timertx=0;    // timer, counts 25us ticks.
int resettx=0;             // reset value for TX
int bitstx=0;              // number of bits to send
unsigned long datatx=0;
boolean spacetx = false;

// Found from https://gist.github.com/1542249 (This is an assumption...)
#define putstring_nl(s) Serial.println(s)
#define putstring(s) Serial.print(s)

// Free RAM
int freeRam(void)
{
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }

  return free_memory;
}


// SETUP function
void setup() {
  Serial.begin(9600);
  putstring_nl("Guerrilla Tag");
  putstring("Free RAM: ");
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!

  //------------
  // PIN SETUP
  //------------
  pinMode(6, OUTPUT); //DAC
  pinMode(7, OUTPUT); //DAC
  pinMode(9, OUTPUT); //DAC
  pinMode(13, OUTPUT); // pin13 LED
  pinMode(4, OUTPUT); // second speaker pin
  // enable pull-up resistors on switch pins (analog inputs)
  pinMode(14, INPUT);
  pinMode(15, INPUT);
  pinMode(16, INPUT);
  pinMode(17, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  digitalWrite(14, HIGH);
  digitalWrite(15, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  pinMode(8, OUTPUT); //audio disabler
  pinMode(3, OUTPUT); //secondary audio
  pinMode(tsoppin, INPUT);

  putstring_nl("Ready!");

  //-------
  //  PWM
  //-------

  //set PWM: FastPWM, OCR2A as top, OC2B sets at bottom, clears on compare
  //COM2B1=1 sets PWM active on pin3, COM2B1=0 disables it (25us ticks)

  digitalWrite(tsoppin, HIGH);
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21);
  OCR2A = 49;
  OCR2B = 24;
  TIMSK2 |= _BV(TOIE2); //activate interrupt on overflow (TOV flag, triggers at OCR2A)
  ircode = (player << 10) | (team << 8) | ((damage/2) << 2);
  delay(500); // HUH: Why was this done??
}



/////////////////////
// LOOP FUNCTION   //
/////////////////////

void loop() {
  elapsed = millis();

  // COMPUTE INCOMING DATA

  if(staterx == STATE_STOP && life>0){          // DATA! a STOP has been issued meaning there's something to compute
    if(datarx==0 || bitsrx!=16 || bitRead(datarx, 15)==1 || bitRead(datarx, 0)==1 || bitRead(datarx, 1)==1){      // if check failed
      //nearmisstone();    //prepare for near miss
    }
    else{
      temp = (datarx & 768) >> 8;                 // temp is the team bits shifted right
      if(temp != team){                           // no friendly fire
        temp = (datarx & 252) >> 2;               // temp is the damage bits
        life = life - temp*2;                     // apply damage, transmitted as half
        resettone = 4;                            // play hit sound. lenghttone specifies duration in number of ticks (25us)
        lenghttone = 45000;
      }
      Serial.println(datarx);
      staterx = STATE_IDLE;                       // carry on, receive other values
      bitsrx = 0;
      datarx = 0;
    }
  }

  // RELOADING
  // GRP: We will probably replace this with something more flexible and move it into Ammo.pde
  if(!(PINC & 0b00001000) && reloadreleased && (elapsed-lastreload)>=1000){  // if reload button has been pressed, empty chamber first, then if magazine is in put a bullet in the chamber.

    reloadreleased = false;
    barrelbullet = false;
    
    temp = analogRead(magazine_pin);

    if((temp>=35) && (temp<=55) && magazines[0]>0){  // MAG number 0
      magazines[0] -= 1;
      barrelbullet = true;
    }
    else if((temp>=200) && (temp<=307) && magazines[1]>0){  // MAG number 1
      magazines[1] -= 1;
      barrelbullet = true;
    }

    Serial.println(temp);
  }

  if((PINC & 0b00001000) && !reloadreleased){
    reloadreleased = true;
  }

  // FIRE IR PULSE

  if(((elapsed-lastshot)>=rofdelay) && !(PINC & 0b00000100)){  //if enough time has elapsed since last shot and magazine has bullets
    temp=analogRead(4);      // check where the selector is and set corresponding rof

    if(temp>=25 && temp<=65){
      selector = 1;
      CLR(PORTB,0);
    }
    else if(temp>=285 && temp<=325){
      selector = 2;
    }
    else{
      selector = 0;
    }
    // GRP: Was REFACTOR BELOW: if(!barrelbullet && (wave.isplaying) && magazines[magazine_used] == 0) {// prevents full auto sound from
    //    on-stopping when magazine is == 0 but button is still pressed
    if(!barrelbullet  && magazines[magazine_used] == 0) {// prevents full auto sound from non-stopping when magazine is == 0 but button is still pressed
      //wave.stop();
      CLR(PORTB,0);
    }

    if(((triggerreleased && selector == 1) || (selector == 2)) && barrelbullet && life>0){  // if selector=1 (single shot) and sound is false (meaning trigger has been released previously) fire
      barrelbullet = false;            // no more bullets in barrel
      magazineupdate();
      if(selector == 1) {
        // GRP: Not Required: playfile("Sound1.WAV");        // play shot sound
        triggerreleased = false;
      }
      // GRP: Not required: was:  else if(!wave.isplaying && selector == 2){
      else if(selector == 2){
        //playfile("Sound2.WAV");
        triggerreleased = false;
      }

      lastshot = elapsed;              // this shot becomes last shot
      SonyIR(ircode, 16);              // fire digital bullet
    }
  }

  // DEATH

  if(life<=0){  // you're dead :(
    resettone = 4;                            // play "dead" sound.
    lenghttone = 0;
  }

  // UPDATE TRIGGER

  if((PINC & 0b00000100) && !triggerreleased && (elapsed-lastshot)>=rofdelay){
    // GRP: Was: wave.stop();
    //wave.stop();
    CLR(PORTB,0);
    triggerreleased = true;
  }
}






//ISR ROUTINE

ISR(TIMER2_OVF_vect, ISR_NOBLOCK){

  //TONE GENERATION

  if(resettone != 0){
    timertone++;
    counttone++;
    if(timertone == resettone){
      sbi(PIND,4);
      timertone = 0;

      if(counttone >= lenghttone && lenghttone != 0){
        resettone = 0;
        counttone = 0;
        CLR(PORTD,5) ;
      }
    }
  }

  //TRANSMISSION

  if(bitstx != 0) {  //if we have got something to transmit
    timertx++;
    
    if(timertx >= resettx) {  //if reset value is reached
      timertx=0;
      if(spacetx){  //it was a space that has just been sent thus a total "set" (bit + space) so..
        spacetx = false; //we are not going to send a space now
        bitstx = bitstx - 1;  //we got 1 less bit to send
        datatx <<= 1;  //shift it so MSB becomes 1st digit
        TCCR2A |= _BV(COM2B1);  //activate pin 3 PWM (ONE)

        if((datatx & TOPBIT)&&(bitstx != 0)) {
          resettx = 48;
        }
        else if(!(datatx & TOPBIT)&&(bitstx != 0)){
          resettx = 24;
        }
        else{
          TCCR2A &= ~(_BV(COM2B1));  //deactivate pin 3 PWM (end of transmission, no more bits to send)
        }
      }
      else {  //we sent the bit, now we have to "send" the space
        spacetx = true;  //we are sending a space
        resettx = 24; //600us/25us = 24
        TCCR2A &= ~(_BV(COM2B1));
      }
    }
  }

  //RECEPTION
  pinrx = digitalRead(tsoppin);
  timerrx++;

  switch(staterx) {

  case STATE_IDLE: // In the middle of a gap
    if (pinrx == MARK) {
      timerrx = 0;
      staterx = STATE_HDR;  // transmission begins, check HDR (header)
    }
    break;

  case STATE_HDR:
    if (pinrx == SPACE) {  // HEADER ended, check time
      if((timerrx>=74)&&(timerrx<=126)){  // HDR is correct, between 2300 and 2500usecs
        timerrx = 0;
        staterx = STATE_SPACE;
      }
      else{
        /* DEBUG
         Serial.println("HDR error, too long:");
         Serial.println(timerrx);
         */

        staterx = STATE_STOP;  // HDR not correct, stop communication for near miss
        datarx = 0;
      }
    }
    break;


  case STATE_MARK: // timing MARK
    if (pinrx == SPACE){   // MARK ended, record time
      if((timerrx>=20)&&(timerrx<=36)){ // ZERO bit
        datarx <<= 1;
        staterx = STATE_SPACE;
        bitsrx++;
      }
      else if((timerrx>=38)&&(timerrx<=66)){ // ONE bit
        datarx = (datarx << 1) | 1;
        staterx = STATE_SPACE;
        bitsrx++;
      }
      else{  //too long to be a ONE bit or too short
        /* DEBUG
         Serial.println("MARK error, too long:");
         Serial.println(timerrx);
         */

        staterx = STATE_STOP;  // MARK not correct, stop communication for near miss
        datarx = 0;
      }
      timerrx = 0;
    }
    else if(timerrx>66){  // MARK, and too long to be a ONE bit
      staterx = STATE_STOP;  // MARK not correct, stop communication for near miss
      datarx = 0;
    }
    break;

  case STATE_SPACE: // timing SPACE
    if (pinrx == MARK) { // SPACE just ended, record it
      if((timerrx>=14)&&(timerrx<=26)){  // SPACE is correct, between 500 and 700usecs
        timerrx = 0;
        staterx = STATE_MARK;
      }
      else{
        staterx = STATE_STOP;  // SPACE too long, communication over
      }
    }
    else{  // SPACE
      if (timerrx > 26) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        staterx = STATE_STOP;  // SPACE too long, communication over

        /* DEBUG
         Serial.println("OK");
         Serial.println(datarx, HEX);
         staterx = STATE_IDLE;
         */
      }
    }
    break;
  }
}
