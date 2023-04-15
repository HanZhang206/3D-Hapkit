/*********************
 * Arduino SPI communication of 1 Integer
 * Implemented by Jan Bartels 26OCT2022
 ********************/

 #include <Wire.h>

int pwmPin = 5; // PWM output pin for motor 1
int dirPin = 8; // direction output pin for motor 1
// Fastest transmission speed is 1/2 clock speed. On the Hapkit that's 16MHz clock speed. SPI can't be faster than 8MHz, would recommend 4MHz to be safe.

// On the atmega328p the SPI pins are on register B. 
// Change these assignments when using a different AVR processor.
#define DDR_SPI        DDRB
#define PORT_SPI       PORTB
#define DD_SCK         PB5
#define DD_LIFO        PB4 // This document uses Leader-Follower terminology
#define DD_LOFI        PB3
#define DD_SS          PB2 // This is the follower's chip select. 

int sensorPosPin = A2; // input pin for MR sensor 

// Position tracking variables
int updatedPos = 0;     // keeps track of the latest updated value of the MR sensor reading
int rawPos = 0;         // current raw reading from MR sensor
int lastRawPos = 0;     // last raw reading from MR sensor
int lastLastRawPos = 0; // last last raw reading from MR sensor
int flipNumber = 0;     // keeps track of the number of flips over the 180deg mark
int tempOffset = 0;
int rawDiff = 0;
int lastRawDiff = 0;
int rawOffset = 0;
int lastRawOffset = 0;
const int flipThresh = 700;  // threshold to determine whether or not a flip over the 180 degree mark occurred
boolean flipped = false;
double OFFSET = 980;
double OFFSET_NEG = 15;

bool new_data = false;
uint8_t read_state;

volatile int input;
volatile int output = 100;

int flag = 0;
int tempOutput = 0;
double ts = 0;

bool bRun = false;

//C-code examples are in the datasheet on page 137 and 138
char position_data = 0;
 
 
void setup() {
  pinMode(sensorPosPin, INPUT); // set MR sensor pin to be an input
  Serial.begin(115200);
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
   Wire.onReceive(receiveEvent); // register event
    
  Serial.println("Initialized Follower");
  
  
  // Output pins
  pinMode(pwmPin, OUTPUT);  // PWM pin for motor A
  pinMode(dirPin, OUTPUT);  // dir pin for motor A
 
  
  setPwmFrequency(pwmPin,1);  
   
  analogWrite(pwmPin, 0);     // set to not be spinning (0/255)
  digitalWrite(dirPin, LOW);  // set direction
  

  // Initialize position valiables
  lastLastRawPos = analogRead(sensorPosPin);
  lastRawPos = analogRead(sensorPosPin);
  flipNumber = 0;

  cli();
   
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 40;// = (16*10^6)/(50*1024) - 1 (must be <65536) 
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12)| (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);   
 
  sei();
}
 
ISR(TIMER1_COMPA_vect)
{   
   getAngle();  
}


void requestEvent()
 {
    byte a[4];    
    int n = ts*10;  
    a[0] = n&0xFF; a[1] = (n>>8)&0xFF; a[2] = (n>>16)&0xFF; a[3] = (n>>24)&0xFF; 
    Wire.write(a,4);  
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int input = 0;
  int num = 0;
  
  while (Wire.available() && num < 4) { // follower may send less than requested
    byte c = Wire.read(); // receive a byte as character
    if(num == 0)
      input |= c;
    else if (num == 1)
      input |= (c<<8);
    else if (num == 2)
      input |= (c<<16);
    else if (num == 3)
      input |= (c<<24);
    num++;
  }
  if(input >= 0)
    digitalWrite(dirPin,HIGH);
  else
   digitalWrite(dirPin,LOW);
   
  analogWrite(pwmPin,fabs(input));  // output the signal  
  //Serial.println(input);         // print the integer
}


void loop() 
{     
    //if(!bRun)  
   //   ts = getAngle(); 
   // Serial.println(ts);     
}

double getAngle() 
{
  //*************************************************************
  //*** Section 1. Compute position in counts (do not change) ***  
  //*************************************************************
  
  // Get voltage output by MR sensor
  rawPos = analogRead(sensorPosPin);  //current raw position from MR sensor
  
  // Calculate differences between subsequent MR sensor readings
  rawDiff = rawPos - lastRawPos;          //difference btwn current raw position and last raw position
  lastRawDiff = rawPos - lastLastRawPos;  //difference btwn current raw position and last last raw position
  rawOffset = abs(rawDiff);
  lastRawOffset = abs(lastRawDiff);
  
  // Update position record-keeping vairables
  lastLastRawPos = lastRawPos;
  lastRawPos = rawPos;
  
  // Keep track of flips over 180 degrees
  if((lastRawOffset > flipThresh) && (!flipped)) { // enter this anytime the last offset is greater than the flip threshold AND it has not just flipped
    if(lastRawDiff > 0) {        // check to see which direction the drive wheel was turning
      flipNumber--;              // cw rotation 
    } else {                     // if(rawDiff < 0)
      flipNumber++;              // ccw rotation
    }
    flipped = true;            // set boolean so that the next time through the loop won't trigger a flip
  } else {                        // anytime no flip has occurred
    flipped = false;
  }
  updatedPos = rawPos + flipNumber*OFFSET; // need to update pos based on what most recent offset is
  //Serial.println(flipNumber);
  //Serial.println(updatedPos);

  //************************************************************
  //** Section 2. Compute position in meters *******************
  //************************************************************

  // ADD YOUR CODE HERE
  // Define kinematic parameters you may need
  double rh = 0.090;   //[m]
  // Step B.1: print updatedPos via serial monitor
  
  // Step B.2:
  ts =  0.0111*updatedPos - 1.905;// Compute the angle of the sector pulley (ts) in degrees based on updatedPos  
  return ts; 
}

// --------------------------------------------------------------
// Function to set PWM Freq -- DO NOT EDIT
// --------------------------------------------------------------
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
