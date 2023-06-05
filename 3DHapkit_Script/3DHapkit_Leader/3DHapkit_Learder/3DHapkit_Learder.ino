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
#define DD_SS_INT      PCINT2 // When changing the follower CS make sure to change its interrupt vector and mask register too
#define DD_SS_INT_MSK  PCMSK0

#define DDR_FOLLOW_CS  DDRD // Data direction register for Port D, the register that both chip select pins sit on
#define FOLLOW_CS_PORT PORTD // Data register for Port D
#define F1_CS          PD7 // This is the chip select pin for the leader to select follower 1
#define F2_CS          PD6 // This is the chip select pin for the leader to select follower 2

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


uint8_t dir = 0;
bool new_data = false;

double R = 95;
double r = 54.45;
double l1 = 80;
double l2 = 170;
 
 
int handle_pos[3];
double handle_angle[3];
double ts;

int zWallPosintion = 130;
int zWallThick = 10;
int nBallRadius = 10;
int nHoleRadius = 20;
int nCirleCenter[2][2];

// Kinematics variables
double xh = 0;           // position of the handle [m]
double theta_s = 0;      // Angle of the sector pulley in deg
double xh_prev;          // Distance of the handle at previous time step
double xh_prev2;
double dxh;              // Velocity of the handle
double dxh_prev;
double dxh_prev2;
double dxh_filt;         // Filtered velocity of the handle
double dxh_filt_prev;
double dxh_filt_prev2;

double force = 0;           // force at the handle
double duty[3];
int output = 0;
double Tp = 0;              // torque of the motor pulley

double rp = 0.005;   //[m]
double rs = 0.074;   //[m]
double rh = 0.090;
double dTmp =  rh * rp / (rs*0.03);  

bool bSerial =false;

#define PROCESSING 1 


void setup() {
  Wire.begin();        
  Serial.begin(115200);  
  
   #ifndef PROCESSING 
    Serial.println("Initialized Leader");
   #endif 
  delay(5000); //Give follower some time to come up.
  duty[0] = 0;  duty[1] = 0;  duty[2] = 0; 
    
// Input pins
  pinMode(sensorPosPin, INPUT); // set MR sensor pin to be an input
   
  // Output pins
  pinMode(pwmPin, OUTPUT);  // PWM pin for motor A
  pinMode(dirPin, OUTPUT);  // dir pin for motor A
  
 setPwmFrequency(pwmPin,1);  
 // Initialize motor 
  analogWrite(pwmPin, 0);     // set to not be spinning (0/255)
  digitalWrite(dirPin, LOW);  // set direction
  
  // Initialize position valiables
  lastLastRawPos = analogRead(sensorPosPin);
  lastRawPos = analogRead(sensorPosPin);
  flipNumber = 0;
  
  handle_pos[0] = 0; handle_pos[1] = 0; handle_pos[2] = 0;
   
  cli();
   
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 40;// = (16*10^6)/(50*1024) - 1 (must be <65536) 
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12)| (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);   

  nCirleCenter[0][0] = -20; 
  nCirleCenter[0][1] = -20; 
  nCirleCenter[1][0] = 20; 
  nCirleCenter[1][1] = 20;   
    
  sei();
  #ifdef PROCESSING
  establishContact();  // send a byte to establish contact until Processing responds 
  #endif
}
 
ISR(TIMER1_COMPA_vect)
{   
   ts = getAngle();
}

int dCCCC = (nHoleRadius+nBallRadius)*(nHoleRadius+nBallRadius); 

bool bIntersectCirle(double* b)
{
    bool bFlag = false;
    for(int i = 0; i < 3; i++)
    {
       // if(fabs(b[1]-nCirleCenter[i][0]) > nHoleRadius+nBallRadius);  
       double distace =  (b[1]-nCirleCenter[i][0])*(b[1]-nCirleCenter[i][0])+ (b[0]-nCirleCenter[i][1])*(b[01]-nCirleCenter[i][1]);
       if(distace < dCCCC) 
       {
           return true;
       }
    }
    return false; 
}

int getDisCircle2Circle(double* b)
{
    for(int i = 0; i < 2; i++)
    {
       // if(fabs(b[1]-nCirleCenter[i][0]) > nHoleRadius+nBallRadius);  
       double distace =  (b[1]-nCirleCenter[i][0])*(b[1]-nCirleCenter[i][0])+ (b[0]-nCirleCenter[i][1])*(b[0]-nCirleCenter[i][1]);
       if(distace < dCCCC) 
       {
           return sqrt(distace);
       }
    }
    return 1000;
}

int GetfollowerAngle(int nfollowerIndex)
{
    Wire.requestFrom(nfollowerIndex, 4);    // request 6 bytes from follower device #8
    int num = 0;
    int input = 0;
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
    return input; 
}

void SendForce2follower(int nfollowerIndex,int out)
{   
    Wire.beginTransmission(nfollowerIndex); // transmit to device #8
    byte a[4];
    //int n = (output>=0? output:-output);     
    a[0] = out&0xFF; a[1] = (out>>8)&0xFF; a[2] = (out>>16)&0xFF; a[3] = (out>>24)&0xFF; 
    Wire.write(a,4);
    Wire.endTransmission();    // stop transmitting
}
  
void loop()
{   
  handle_angle[1] = GetfollowerAngle(8)/10.0; 
  delay(20);
  handle_angle[2] = GetfollowerAngle(12)/10.0; 

  #ifdef PROCESSING 
  if (Serial.available() > 0) 
  #endif
  {
     #ifdef PROCESSING
     int inByte = Serial.read();
     #endif       
 
     double RealAngle[3]; 
     RealAngle[0] = 110-ts; 
     RealAngle[1] = 112-handle_angle[1];
     RealAngle[2] = 112-handle_angle[2];
    
     double b[3];    
     forward(RealAngle, b);  
     handle_pos[0] = b[1]*10;handle_pos[1] = b[0]*10;handle_pos[2] = -b[2]*10;
  
     ////// //////  force
       
    xh = -((-b[2]-nBallRadius)-zWallPosintion-zWallThick/2)/1000.0;  
   
    bool bInverst = false;
    if(xh > (nBallRadius+zWallThick/2)/1000.0) bInverst = true;  

    if(xh < 0.0 || xh > 0.002*(nBallRadius+zWallThick/2)) force = 0;
    else   // Enabling a Virtual Wall
    { 
        int distanceCC = getDisCircle2Circle(b);                  
        if(distanceCC <= nBallRadius) force = 0;          
        else
        {
           if (bInverst) // virtual wall force feedback rendering
            {              
              force = 100 * fabs((2*nBallRadius+zWallThick)/1000.0-xh);
            }
            else   // force without the damping when exiting
            {
              force = 150 * (xh);
            }                        
        }       
    }     
    if(force > 4.5) force = 4.5;  // max fore = 4.9
    
    #ifndef PROCESSING 
     Serial.print("xh: ");
     Serial.print(xh*1000);    
     Serial.print(" dxh_filt: ");
     Serial.print(dxh_filt);
     Serial.print(" force: ");
     Serial.print(force);
   #endif
   
    double Tp = force * dTmp;//rh * rp / (rs*0.03);  
  
    // Determine correct direction for motor torque
    if(force > 0) { 
      digitalWrite(dirPin, bInverst?LOW:HIGH);
    } else {
      digitalWrite(dirPin, bInverst?HIGH:LOW);
    }
  
    // Compute the duty cycle required to generate Tp (torque at the motor pulley)
   
    //duty[0] = sqrt(abs(Tp)/0.03);
    duty[0] = sqrt(abs(Tp));
    
    #ifndef PROCESSING 
      Serial.print(" Tp: ");
      Serial.print(Tp);
   #endif 
   
    // Make sure the duty cycle is between 0 and 100%
    if (duty[0] > 1) {            
      duty[0] = 1;
    } else if (duty[0] < 0) { 
      duty[0] = 0;
    }  
    output = (int)(duty[0]* 255);   // convert duty cycle to output signal
    analogWrite(pwmPin,output);  // output the signal
  
    #ifndef PROCESSING 
      Serial.print(" output: ");
      Serial.print(output);
    #endif 
    
     if(force > 0) { 
       SendForce2follower(8,bInverst?-output:output);
       delay(20);
       SendForce2follower(12,bInverst?-output:output);
        
    } else {
       SendForce2follower(8,bInverst?output:-output);
       delay(20);
       SendForce2follower(12,bInverst?output:-output);  
    }
   
  
    ////////////////////   
      
      #ifdef PROCESSING 
          byte sign[2]; sign[0] = handle_pos[0]>=0?1:0; sign[1] = handle_pos[1]>=0?1:0;
          handle_pos[0] = fabs( handle_pos[0]); handle_pos[1] = fabs( handle_pos[1]);
          //handle_pos[0] = 10; handle_pos[1] = 100; handle_pos[2] = 300;
          Serial.write(handle_pos[0]&0xFF);  Serial.write((handle_pos[0]>>8)&0xFF); Serial.write((handle_pos[0]>>16)&0xFF); Serial.write((handle_pos[0]>>24)&0xFF); 
          Serial.write(handle_pos[1]&0xFF);  Serial.write((handle_pos[1]>>8)&0xFF); Serial.write((handle_pos[1]>>16)&0xFF); Serial.write((handle_pos[1]>>24)&0xFF); 
          Serial.write(handle_pos[2]&0xFF);  Serial.write((handle_pos[2]>>8)&0xFF); Serial.write((handle_pos[2]>>16)&0xFF); Serial.write((handle_pos[2]>>24)&0xFF);      
           Serial.write(sign[0]); Serial.write(sign[1]);         
       #else   
             
        Serial.print(" pos1: ");
        Serial.print(RealAngle[0]);
        Serial.print("  pos2: ");
        Serial.print(RealAngle[1]); 
        Serial.print("  pos3: ");
        Serial.print(RealAngle[2]);
        
        Serial.print(" x: ");
        Serial.print(handle_pos[0]/10.0);
        Serial.print(" y: ");
        Serial.print(handle_pos[1]/10.0);
        Serial.print(" z: ");
        Serial.println(handle_pos[2]/10.0);
      #endif      
   }            
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
  // Serial.println(updatedPos);

  //************************************************************
  //** Section 2. Compute position in meters *******************
  //************************************************************

  // ADD YOUR CODE HERE
  // Define kinematic parameters you may need
  double rh = 0.090;   //[m]
  // Step B.1: print updatedPos via serial monitor
  
  // Step B.2:
  double ts =  0.0111*updatedPos - 1.905;// Compute the angle of the sector pulley (ts) in degrees based on updatedPos
  return ts; 
}

void forward(double* angIn, double* pos)
{
  double ang[3];
  ang[0] = angIn[0] * PI / 180;
  ang[1] = angIn[1] * PI / 180;
  ang[2] = angIn[2] * PI / 180;
  double a1 = R + l1 * sin(ang[0]) - r;
  double b1 = 0;
  double c1 = l1 * cos(ang[0]);
  double a2 = -(R + l1 * sin(ang[1]) - r) / 2;
  double b2 = sqrt(3) * (R + l1 * sin(ang[1]) - r) / 2;
  double c2 = l1 * cos(ang[1]);
  double a3 = -(R + l1 * sin(ang[2]) - r) / 2;
  double b3 = -sqrt(3) * (R + l1 * sin(ang[2]) - r) / 2;
  double c3 = l1 * cos(ang[2]);
  double a12 = a2 - a1;
  double c12 = c2 - c1;
  double d1 = (pow(a2, 2) - pow(a1, 2) + pow(c2, 2) - pow(c1, 2) + pow(b2, 2)) / 2;
  double a13 = a3 - a1;
  double c13 = c3 - c1;
  double d2 = (pow(a3, 2) - pow(a1, 2) + pow(c3, 2) - pow(c1, 2) + pow(b3, 2)) / 2;
  double e1 = (b3 * c12 - b2 * c13) / (b3 * a12 - b2 * a13);
  double f1 = (b3 * d1 - b2 * d2) / (b3 * a12 - b2 * a13);
  double e2 = (a13 * c12 - a12 * c13) / (a13 * b2 - a12 * b3);
  double f2 = (a13 * d1 - a12 * d2) / (a12 * b2 - a12 * b3);
  double a = pow(e1, 2) + pow(e2, 2) + 1;
  double b = 2 * e1 * f1 - 2 * e1 * a1 + 2 * e2 * f2 + 2 * c1;
  double c = pow(f2, 2) + pow((a1 - f1), 2) + pow(c1, 2) - pow(l2, 2);
  double bb = b * b - 4 * a * c;
  double aa = sqrt(b * b - 4 * a * c);
  pos[2] = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
  pos[0] = e1 * pos[2] + f1;
  pos[1] = e2 * pos[2] + f2;
  return 0;
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


void establishContact() {
 while (Serial.available() <= 0) {
      Serial.write('A');   // send a capital A
      delay(1000);
      bSerial = true;

  }
}
