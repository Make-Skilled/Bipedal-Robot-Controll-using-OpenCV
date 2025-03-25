#include <Wire.h>

#define servo1  (16>>1)
#define servo2  (18>>1)
#define UART_BAUD_RATE  115200
#define LED 13

#define SERIAL_BUFFER_SIZE  256

void I2C_SERVOSET(unsigned char servo_num,unsigned int servo_pos);
void I2C_SERVOREVERSE(unsigned char servo_num,unsigned char servo_dir);
void I2C_SERVOOFFSET(unsigned char servo_num,int value);
void I2C_SERVOSPEED(unsigned char value);
void I2C_SERVONUTRALSET(unsigned char servo_num,unsigned int servo_pos);
void I2C_SERVOMIN(unsigned char servo_num,unsigned int servo_pos);
void I2C_SERVOMAX(unsigned char servo_num,unsigned int servo_pos);
char I2C_SERVOEND(void);
int I2C_SERVOGET(int servo_num);
int I2C_SERVOGETOFFSET(int servo_num);
void CheckEndMovement(void);
void PCControlledCode(void);
void UserCode(void);
void LEDToggle(void);

volatile int cnt,c,servoval;
volatile char state,servobuf[36],bytecnt;

int interval=100;
unsigned long previousMillis=0;
unsigned long currentMillis = millis();
char runCode =0;  //1 for PC controlled, 0 for user controlled
char LEDState=0;

#define State_Start  0
#define State_Command  1
#define State_Servoposition  2
#define State_Speed  3
#define State_Servomin  4
#define State_Servomax  5
#define State_Servooffset  6
#define State_Servoreverse  7
#define State_Servonutral  8
#define State_ReadOffsets  9


void setup()
{ 
  cnt=0;
  int i;
  unsigned int x;
  char buffer[10],tmp,tmp1;
  float range;
  Serial.begin(UART_BAUD_RATE);
  Serial.write('S');
  Serial.write('r');
  pinMode(13, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(8, INPUT);
  digitalWrite(8,1);
  delay(500);
  sei();
  Wire.begin();
  TWSR = 3; // no prescaler
  TWBR = 18;  //Set I2C speed lower to suite I2C Servo controller
  pinMode(2,OUTPUT);
  digitalWrite(2,HIGH);
  delay(500);
  state=State_Start;
  
//  if (digitalRead(8))
//  {
//    runCode=1;
//  }
//  else
//  {
//    runCode=0;
//  }
runCode=0;
}

void loop()
{
  if (runCode ==1)
  {
    PCControlledCode();
  }
  else
  {
    UserCode();
  LEDToggle();
  }
}


void PCControlledCode(void)
{
  currentMillis = millis();
  if(Serial.available()>0)
  {
    c=Serial.read();
    previousMillis = currentMillis;
    if(state==State_Start)
    {
      if(c==170)    
        state=State_Command;                     
    }          
    else if(state==State_Servoposition)
    {
      servobuf[bytecnt]=c;
      bytecnt++;
      if(bytecnt==36)
      {
        for(int i=1;i<19;i++)
        {
          servoval=servobuf[(i*2)-1]<<7;
          servoval=servoval+servobuf[(i*2)-2];
          I2C_SERVOSET(i,servoval);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write('r');
        CheckEndMovement();
        previousMillis = currentMillis; 
      }
    }                
    else if(state==State_Speed)
    {
      I2C_SERVOSPEED(c);
      state=State_Start;
      Serial.write("rs");
      CheckEndMovement();
    }
    else if(state==State_Servomin)
    {
      servobuf[bytecnt]=c;
      bytecnt++;                    
      if(bytecnt==36)
      {
        for(int i=1;i<19;i++)
        {
          servoval=servobuf[(i*2)-1]<<7;
          servoval=servoval+servobuf[(i*2)-2];
          I2C_SERVOMIN(i,servoval);
          delay(20);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write("rn");
        CheckEndMovement();
        previousMillis = currentMillis; 
      }
    }
    else if(state==State_Servomax)
    {
      servobuf[bytecnt]=c;
      bytecnt++;                    
      if(bytecnt==36)
      {
        for(int i=1;i<19;i++)
        {
          servoval=servobuf[(i*2)-1]<<7;
          servoval=servoval+servobuf[(i*2)-2];
          I2C_SERVOMAX(i,servoval);
          delay(20);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write("rx");
        CheckEndMovement();
        previousMillis = currentMillis;
      }  
    }             
    else if(state==State_Servooffset)
    {
      servobuf[bytecnt]=c;
      bytecnt++;                    
      if(bytecnt==36)
      {
        for(int i=1;i<19;i++)
        {
          servoval=servobuf[(i*2)-1]<<7;
          servoval=servoval+servobuf[(i*2)-2];
          I2C_SERVOOFFSET(i,servoval);
          delay(20);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write("ro");
        CheckEndMovement();
        previousMillis = currentMillis; 
      } 
    }
    else if(state==State_Servoreverse)
    {
      servobuf[bytecnt]=c;
      bytecnt++;                                      
      if(bytecnt==18)
      {
        for(int i=1;i<19;i++)
        {
          I2C_SERVOREVERSE(i,servobuf[(i-1)]);
          delay(20);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write("rv");
        CheckEndMovement();
        previousMillis = currentMillis;
      } 
    }           
    else if(state==State_Servonutral)
    {
      servobuf[bytecnt]=c;
      bytecnt++;                    
      if(bytecnt==36)
      {
        for(int i=1;i<19;i++)
        {
          servoval=servobuf[(i*2)-1]<<7;
          servoval=servoval+servobuf[(i*2)-2];
          I2C_SERVONUTRALSET(i,servoval);
        } 
        bytecnt=0;
        state=State_Start;
        Serial.write("rn");
        CheckEndMovement();
        previousMillis = currentMillis; 
      }  
    }
    else if(state==State_ReadOffsets)
    {
      Serial.write("O");
      for(int i=1;i<19;i++)
      {
        Serial.print(I2C_SERVOGETOFFSET(i));
        Serial.write(",");
        delay(1);
      }
      Serial.write("F");
      state=State_Start;
      Serial.write('r');
      CheckEndMovement();
      previousMillis = currentMillis;
    }
    else if(state==State_Command)
    {
        if(c==8)
        state=State_Speed;
        if(c==10)
        state=State_Servoposition;
        if(c==11)
        state=State_Servomin;
        if(c==12)
        state=State_Servomax;
        if(c==13)
        state=State_Servooffset;
        if(c==14)
        state=State_Servoreverse;                        
        if(c==15)
        state=State_Servonutral;
        if(c==21)
        state=State_ReadOffsets;
        bytecnt=0;                     
    } 
    else
    {
      Serial.write('e');
      CheckEndMovement();
    }
  }
  else
  {
    if ((unsigned long)(currentMillis - previousMillis) >= interval) 
    {
      Serial.write('i');
      Serial.write('r');
      CheckEndMovement();
      previousMillis = currentMillis;
    }
  }  
}

void CheckEndMovement(void)
{
  if(I2C_SERVOEND())
    Serial.write('X');
  else
    Serial.write('G');

  LEDToggle();
}

void I2C_SERVOSET(unsigned char servo_num,unsigned int servo_pos)
{
  if(servo_pos<500)
    servo_pos = 500;
  else if(servo_pos>2500)
    servo_pos=2500;

  if(servo_pos>501)
    servo_pos=(((servo_pos-2)*2)-1000);
  else
    servo_pos=0;

  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write(servo_num-1);
  Wire.write(servo_pos>>8);
  Wire.write(servo_pos & 0XFF);
  Wire.endTransmission();
}

void I2C_SERVOMIN(unsigned char servo_num,unsigned int servo_pos)
{
  if(servo_pos<500)
    servo_pos = 500;
  else if(servo_pos>2500)
    servo_pos=2500;
  servo_pos=((servo_pos*2)-1000);

  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write((servo_num-1)+(18*4));
  Wire.write(servo_pos>>8);
  Wire.write(servo_pos & 0XFF);
  Wire.endTransmission();
  delay(20);
}

void I2C_SERVOMAX(unsigned char servo_num,unsigned int servo_pos)
{
  if(servo_pos<500)
    servo_pos = 500;
  else if(servo_pos>2500)
    servo_pos=2500;
  servo_pos=((servo_pos*2)-1000);

  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write((servo_num-1)+(18*3));
  Wire.write(servo_pos>>8);
  Wire.write(servo_pos & 0XFF);
  Wire.endTransmission();
  delay(20);
}

void I2C_SERVONUTRALSET(unsigned char servo_num,unsigned int servo_pos)
{
  if(servo_pos<500)
    servo_pos = 500;
  else if(servo_pos>2500)
    servo_pos=2500;
  servo_pos=((servo_pos*2)-1000);

  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write((servo_num-1)+(18*5));
  Wire.write(servo_pos>>8);
  Wire.write(servo_pos & 0XFF);
  Wire.endTransmission();
}

void I2C_SERVOSPEED(unsigned char value)
{
  Wire.beginTransmission(servo1);
  Wire.write(18*2);
  Wire.write(value);
  Wire.write(0);
  Wire.endTransmission();
  Wire.beginTransmission(servo2);
  Wire.write(18*2);
  Wire.write(value);
  Wire.write(0);
  Wire.endTransmission();
  delay(20);
}

void I2C_SERVOOFFSET(unsigned char servo_num,int value)
{
  value=3000-value;
  value=value-1500;

  if (value<-500)
    value=-500;
  else if (value>500)
    value=500;

  if(value>0)
    value=2000+(value*2);
  else if(value<=0)
    value=-value*2;

  
  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write((servo_num-1)+(18*6));
  Wire.write(value>>8);
  Wire.write(value & 0XFF);
  Wire.endTransmission();
  delay(20);
}

void I2C_SERVOREVERSE(unsigned char servo_num,unsigned char servo_dir)
{
  if(servo_dir>0)
    servo_dir=1;
  if(servo_num<19)
    Wire.beginTransmission(servo1);
  else
    Wire.beginTransmission(servo2);
  Wire.write((servo_num-1)+(18*7));
  Wire.write(servo_dir);
  Wire.write(0);
  Wire.endTransmission();
  delay(20);
}

char I2C_SERVOEND(void)
{
  int i, n;
  char buffer;
  Wire.beginTransmission(servo1);
  n = Wire.write(181);
  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);
  if (n != 0)
    return (n);

  delayMicroseconds(350);
  Wire.requestFrom(servo1, 1, true);
  while(Wire.available())
    buffer=Wire.read();

  return(buffer);
}

int I2C_SERVOGET(int servo_num)
{
  int i, n, error;
  uint8_t buffer[2];
  Wire.beginTransmission(servo1);

  n = Wire.write((servo_num-1)+(18*8));
  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);
  if (n != 0)
    return (n);

  delayMicroseconds(240);
  Wire.requestFrom(servo1, 2, true);
  i = 0;
  while(Wire.available() && i<2)
  {
    buffer[i++]=Wire.read();
  }
  if ( i != 2)
    return (-11);
  return (((buffer[0]*256 + buffer[1])+4)/2 +500);
}

int I2C_SERVOGETOFFSET(int servo_num)
{
  int i, n, error;
  uint8_t buffer[2];
  Wire.beginTransmission(servo1);

  n = Wire.write((servo_num-1)+(182));
  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);
  if (n != 0)
    return (n);

  delayMicroseconds(240);
  Wire.requestFrom(servo1, 2, true);
  i = 0;
  while(Wire.available() && i<2)
  {
    buffer[i++]=Wire.read();
  }
  if ( i != 2)
    return (-11);
  i=((buffer[0]*256 + buffer[1]));
  if(i>2000)
    return(3000-(((i-2000)/2)+1500));
  else
    return(3000-((-i/2)+1500));
}


void ServoSetAll(unsigned int Servo1, unsigned int Servo2, unsigned int Servo3, unsigned int Servo4, unsigned int Servo5, unsigned int Servo6, unsigned int Servo7, unsigned int Servo8, unsigned int Servo9, unsigned int Servo10, unsigned int Servo11, unsigned int Servo12, unsigned int Servo13, unsigned int Servo14, unsigned int Servo15, unsigned int Servo16, unsigned int Servo17, unsigned int Servo18)
{
  if (Servo1 >= 500) {I2C_SERVOSET(1,Servo1);}
  if (Servo2 >= 500) {I2C_SERVOSET(2,Servo2);}
  if (Servo3 >= 500) {I2C_SERVOSET(3,Servo3);}
  if (Servo4 >= 500) {I2C_SERVOSET(4,Servo4);}
  if (Servo5 >= 500) {I2C_SERVOSET(5,Servo5);}
  if (Servo6 > 500) {I2C_SERVOSET(6,Servo6);}
  if (Servo7 >= 500) {I2C_SERVOSET(7,Servo7);}
  if (Servo8 >= 500) {I2C_SERVOSET(8,Servo8);}
  if (Servo9 >= 500) {I2C_SERVOSET(9,Servo9);}
  if (Servo10 >= 500) {I2C_SERVOSET(10,Servo10);}
  if (Servo11 >= 500) {I2C_SERVOSET(11,Servo11);}
  if (Servo12 >= 500) {I2C_SERVOSET(12,Servo12);}
  if (Servo13 >= 500) {I2C_SERVOSET(13,Servo13);}
  if (Servo14 >= 500) {I2C_SERVOSET(14,Servo14);}
  if (Servo15 >= 500) {I2C_SERVOSET(15,Servo15);}
  if (Servo16 >= 500) {I2C_SERVOSET(16,Servo16);}
  if (Servo17 >= 500) {I2C_SERVOSET(17,Servo17);}
  if (Servo18 >= 500) {I2C_SERVOSET(18,Servo18);}
  while (!I2C_SERVOEND())
  {
    delay(1);
  }
  LEDToggle();
}

void LEDToggle(void)
{
  if (LEDState == 0)
    LEDState = 1;
  else
    LEDState = 0;
  digitalWrite(LED, LEDState);
}

// Modified UserCode() function
void UserCode(void)
{
  // Configuration (initial servo limits)
  I2C_SERVOMAX(1,2500); I2C_SERVOMAX(2,2500); I2C_SERVOMAX(3,2500); I2C_SERVOMAX(4,2500); I2C_SERVOMAX(5,2500); I2C_SERVOMAX(6,2500); I2C_SERVOMAX(7,2500); I2C_SERVOMAX(8,2500); I2C_SERVOMAX(9,2500); I2C_SERVOMAX(10,2500); I2C_SERVOMAX(11,2500); I2C_SERVOMAX(12,2500); I2C_SERVOMAX(13,2500); I2C_SERVOMAX(14,2500); I2C_SERVOMAX(15,2500); I2C_SERVOMAX(16,2500); I2C_SERVOMAX(17,2500); I2C_SERVOMAX(18,2500);
  I2C_SERVOMIN(1,500); I2C_SERVOMIN(2,500); I2C_SERVOMIN(3,500); I2C_SERVOMIN(4,500); I2C_SERVOMIN(5,500); I2C_SERVOMIN(6,500); I2C_SERVOMIN(7,500); I2C_SERVOMIN(8,500); I2C_SERVOMIN(9,500); I2C_SERVOMIN(10,500); I2C_SERVOMIN(11,500); I2C_SERVOMIN(12,500); I2C_SERVOMIN(13,500); I2C_SERVOMIN(14,500); I2C_SERVOMIN(15,500); I2C_SERVOMIN(16,500); I2C_SERVOMIN(17,500); I2C_SERVOMIN(18,500);
  I2C_SERVOOFFSET(1,1500); I2C_SERVOOFFSET(2,1500); I2C_SERVOOFFSET(3,1500); I2C_SERVOOFFSET(4,1500); I2C_SERVOOFFSET(5,1500); I2C_SERVOOFFSET(6,1500); I2C_SERVOOFFSET(7,1500); I2C_SERVOOFFSET(8,1500); I2C_SERVOOFFSET(9,1500); I2C_SERVOOFFSET(10,1500); I2C_SERVOOFFSET(11,1500); I2C_SERVOOFFSET(12,1500); I2C_SERVOOFFSET(13,1500); I2C_SERVOOFFSET(14,1500); I2C_SERVOOFFSET(15,1500); I2C_SERVOOFFSET(16,1500); I2C_SERVOOFFSET(17,1500); I2C_SERVOOFFSET(18,1500);
  I2C_SERVOREVERSE(1,0); I2C_SERVOREVERSE(2,0); I2C_SERVOREVERSE(3,0); I2C_SERVOREVERSE(4,0); I2C_SERVOREVERSE(5,0); I2C_SERVOREVERSE(6,0); I2C_SERVOREVERSE(7,0); I2C_SERVOREVERSE(8,0); I2C_SERVOREVERSE(9,0); I2C_SERVOREVERSE(10,0); I2C_SERVOREVERSE(11,0); I2C_SERVOREVERSE(12,0); I2C_SERVOREVERSE(13,0); I2C_SERVOREVERSE(14,0); I2C_SERVOREVERSE(15,0); I2C_SERVOREVERSE(16,0); I2C_SERVOREVERSE(17,0); I2C_SERVOREVERSE(18,0);

  // Serial control loop
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read input until newline
    input.trim(); // Remove any whitespace

    // Single servo control: S<servo_num>:<position>
    if (input.startsWith("S") && input.indexOf(':') > 1) {
      int servoNum = input.substring(1, input.indexOf(':')).toInt(); // Extract servo number
      int position = input.substring(input.indexOf(':') + 1).toInt(); // Extract position

      // Validate input
      if (servoNum >= 1 && servoNum <= 18 && position >= 500 && position <= 2500) {
        I2C_SERVOSET(servoNum, position); // Move the servo
        Serial.print("Servo ");
        Serial.print(servoNum);
        Serial.print(" moved to position ");
        Serial.println(position);
      } else {
        Serial.println("Invalid input! Servo number must be 1-18, position 500-2500.");
      }
    }
    // Gesture-based control: G<gesture_id>
    else if (input.startsWith("G") && input.length() == 2) {
      char gestureId = input.charAt(1);
      switch (gestureId) {
        case '1': // Gesture 1: Come on
          comeon();
          Serial.println("Gesture 1: Executing Come On");
          break;
        case '2': // Gesture 2: Victory
          victory();
          Serial.println("Gesture 2: Executing Victory");
          break;
        case '3': // Gesture 3: Push-up
          pushup();
          Serial.println("Gesture 3: Executing Push-up");
          break;
        case '4': // Gesture 4: One leg
          oneleg();
          Serial.println("Gesture 4: Executing One Leg");
          break;
        case '5': // Gesture 5: Tarzan
          tarzan();
          Serial.println("Gesture 5: Executing Tarzan");
          break;
        case '6': // Gesture 6: Walking
          walking();
          Serial.println("Gesture 6: Executing Walking");
          break;
        case '7': // Gesture 7: Hip-hop
          hiphop();
          Serial.println("Gesture 7: Executing Hip-hop");
          break;
        default:
          Serial.println("Invalid gesture ID! Use G1 to G7.");
          break;
      }
    }
    // Predefined sequence commands (manual input)
    else if (input.length() == 1) {
      char command = input.charAt(0);
      switch (command) {
        case 'C': // Come on
          comeon();
          Serial.println("Executing: Come On");
          break;
        case 'V': // Victory
          victory();
          Serial.println("Executing: Victory");
          break;
        case 'P': // Push-up
          pushup();
          Serial.println("Executing: Push-up");
          break;
        case 'O': // One leg
          oneleg();
          Serial.println("Executing: One Leg");
          break;
        case 'T': // Tarzan
          tarzan();
          Serial.println("Executing: Tarzan");
          break;
        case 'W': // Walking
          walking();
          Serial.println("Executing: Walking");
          break;
        case 'H': // Hip-hop
          hiphop();
          Serial.println("Executing: Hip-hop");
          break;
        case 'R': // Reset all servos to 1500
          ServoSetAll(1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500);
          Serial.println("All servos reset to 1500");
          break;
        default:
          Serial.println("Invalid command! Use S<servo_num>:<position> (e.g., S1:2000) or C, V, P, O, T, W, H, R");
          break;
      }
    }
    else {
      Serial.println("Invalid format! Use S<servo_num>:<position> (e.g., S1:2000), G<1-7> (e.g., G1), or single letter: C, V, P, O, T, W, H, R");
    }
  }
}

void comeon(){
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1500,1231,1500,1662,1500,1326,1500,1500,1500);    // Line # 0
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 1
  delay(100);   //Default Delay
  ServoSetAll(1000,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 2
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 3
  delay(100);   //Default Delay
  ServoSetAll(1000,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 4
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 5
  delay(100);   //Default Delay
  ServoSetAll(1000,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 6
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 7
  delay(100);   //Default Delay
  ServoSetAll(1000,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 8
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 9
  delay(100);   //Default Delay
  ServoSetAll(1000,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 10
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,751,2237,1428,1500,1000,1231,1500,1662,1500,1326,1500,500,2500);   // Line # 11
  delay(100);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1600,1500,1500,1500,1500,1500,1500,1500,1500,1400,1500,1500,1500,1500);   // Line # 12
  delay(100);
}

void victory(){
  ServoSetAll(1500,1500,1500,1500,1600,1500,1500,1500,1500,1500,1500,1500,1500,1400,1500,1500,1500,1500);   // Line # 0
  delay(100);   //Default Delay
  ServoSetAll(1500,600,1500,1500,1600,1500,1500,1500,1500,1500,1500,1500,1500,1400,1500,1500,2400,1500);    // Line # 1
  delay(100);   //Default Delay
  ServoSetAll(1500,600,1500,1500,1600,1500,1500,1500,1500,1326,1500,1500,1500,1500,1400,500,2400,1500);   // Line # 2
  delay(100);   //Default Delay
  ServoSetAll(1500,600,1500,1578,1500,1500,1500,1578,1500,1326,1500,1500,1650,1302,1500,500,2400,1500);   // Line # 3
  delay(100);   //Default Delay
  ServoSetAll(1500,600,1500,1662,1500,1500,1500,1638,1500,1326,1500,1500,1686,1183,1500,500,2400,1500);   // Line # 4
  delay(100);   //Default Delay
  ServoSetAll(1500,1590,1500,1738,1602,1500,1590,1829,1500,1003,1500,1299,2213,728,1500,500,2400,500);    // Line # 5
  delay(100);   //Default Delay
  delay(2000);    // Line # 6
  delay(2000);    // Line # 7
  ServoSetAll(1500,1590,1500,1738,1602,1500,1590,1829,1500,1003,1500,1299,1951,1171,1500,500,2400,500);   // Line # 8
  delay(100);   //Default Delay
}

void pushup(){
  ServoSetAll(1500,1500,1500,2105,1500,1500,1500,2165,1500,1500,871,1500,1500,1500,967,1500,1500,1500);    // Line # 0
  delay(1000);   //Default Delay
  ServoSetAll(2165,560,2478,2105,1500,1500,1500,2165,1500,1500,871,1500,1500,1500,967,540,2460,800);    // Line # 1
  delay(1000);   //Default Delay
  ServoSetAll(2165,560,2478,2105,1111,1500,1500,2165,1500,1500,871,1500,1500,1829,967,540,2460,800);    // Line # 2
  delay(1000);   //Default Delay
  ServoSetAll(2165,560,2478,1500,1111,1500,1500,1500,1500,1500,1500,1500,1500,1829,1500,540,2460,800);    // Line # 3
  delay(1000);   //Default Delay
  ServoSetAll(2165,560,2478,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,540,2460,800);    // Line # 4
  delay(1000);   //Default Delay
  ServoSetAll(2009,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,919);    // Line # 5
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,1123);   // Line # 6
  delay(1000);   //Default Delay
  ServoSetAll(2009,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,919);    // Line # 7
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,1123);   // Line # 8
  delay(1000);   //Default Delay
  ServoSetAll(2009,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,919);    // Line # 9
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,1123);   // Line # 10
  delay(1000);   //Default Delay
  ServoSetAll(2009,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,919);    // Line # 11
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,1123);   // Line # 12
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,2428,1500,1500,1500,1500,1500,1500,548,1500,1500,608,2460,1123);    // Line # 13
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,2200,2428,800,1500,1500,1500,1500,2200,548,800,1500,608,2460,1123);    // Line # 14
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1700,2428,800,1500,1500,1500,1500,2200,548,1300,1500,608,2460,1123);   // Line # 15
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1600,2428,800,1500,1500,1500,1500,2200,548,1400,1500,608,2460,1123);   // Line # 16
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,2150,1025,1500,1500,1500,1500,1850,1015,1500,1500,608,2460,1123);   // Line # 17
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,2150,1070,1500,1500,1500,1500,1850,1070,1500,1500,608,2460,1123);   // Line # 18
  delay(1000);   //Default Delay
  ServoSetAll(1769,560,2431,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,608,2460,1123);   // Line # 19
  delay(1000);   //Default Delay
}

void oneleg(){
  ServoSetAll(1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500);    // Line # 0
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,1500,1500,1386,1500,1500,1500,2308,668,1500,1434,1500,1500,1500);    // Line # 1
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1900,1500,1500,1366,1500,1500,1500,2308,668,2500,1434,1500,1500,1500);    // Line # 2
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1900,1500,1500,1386,1500,1500,1500,2308,668,2500,1434,1500,1500,1500);    // Line # 3
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1500,1500);    // Line # 4
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1900,1500,1500,1500,2500,1434,500,1800,1900);    // Line # 5
  delay(1000);   //Default Delay
  ServoSetAll(1100,1200,2500,1500,2105,1500,1500,1386,1500,1100,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 6
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1900,1500,1500,1500,2500,1434,500,1800,1900);    // Line # 7
  delay(1000);   //Default Delay
  ServoSetAll(1100,1200,2500,1500,2105,1500,1500,1386,1500,1100,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 8
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1900,1500,1500,1500,2500,1434,500,1800,1900);    // Line # 9
  delay(1000);   //Default Delay
  ServoSetAll(1100,1200,2500,1500,2105,1500,1500,1386,1500,1100,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 10
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1900,1500,1500,1500,2500,1434,500,1800,1900);    // Line # 11
  delay(1000);   //Default Delay
  ServoSetAll(1100,1200,2500,1500,2105,1500,1500,1386,1500,1100,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 12
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 13
  delay(1000);   //Default Delay
  ServoSetAll(1900,1300,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1700,1100);    // Line # 14
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 15
  delay(1000);   //Default Delay
  ServoSetAll(1900,1300,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1700,1100);    // Line # 16
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 17
  delay(1000);   //Default Delay
  ServoSetAll(1900,1300,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1700,1100);    // Line # 18
  delay(1000);   //Default Delay
  ServoSetAll(1900,1800,2500,1500,2105,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1200,1100);    // Line # 19
  delay(1000);   //Default Delay
  ServoSetAll(1900,1300,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1700,1100);    // Line # 20
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1970,1500,1500,1386,1500,1500,1500,1500,1500,2500,1434,500,1500,1500);    // Line # 21
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1970,1500,1500,1386,1500,1500,1500,2308,668,2500,1434,500,1500,1500);   // Line # 22
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1850,1500,1500,1386,1500,1500,1500,2308,668,2500,1434,500,1500,1500);   // Line # 23
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1686,1500,1500,1386,1500,1500,1500,2308,668,1300,1434,500,1500,1500);   // Line # 24
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1686,1500,1500,1386,1500,1500,1500,1500,1500,1500,1500,500,1500,1500);    // Line # 25
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,1500);    // Line # 26
  delay(1000);   //Default Delay
}

void tarzan(){
  ServoSetAll(1500,1500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,1500,1500);    // Line # 0
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 1
  delay(1000);   //Default Delayus
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,500);   // Line # 2
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 3
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,500);   // Line # 4
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 5
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,500);   // Line # 6
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 7
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,500);   // Line # 8
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 9
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 10
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 11
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 12
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 13
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 14
  delay(1000);   //Default Delay
  ServoSetAll(1500,800,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2200,1500);    // Line # 15
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2500,500);   // Line # 16
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 17
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2500,500);   // Line # 18
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 19
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2500,500);   // Line # 20
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 21
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2500,500);   // Line # 22
  delay(1000);   //Default Delay
  ServoSetAll(2500,500,2500,1700,1500,900,1900,1710,1500,1800,1248,1100,2100,1500,1300,500,2500,1500);    // Line # 23
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1700,1500,900,1900,1710,1500,1200,1248,1100,2100,1500,1300,500,2500,500);   // Line # 24
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1700,1500,900,1900,1710,1500,1500,1248,1100,2100,1500,1300,500,1500,1500);   // Line # 25
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1700,1500,1100,1800,1710,1500,1500,1248,1150,1900,1500,1300,500,1500,1500);    // Line # 26
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1500,1100,1800,1500,1500,1500,1500,1150,1900,1500,1500,500,1500,1500);    // Line # 27
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1500,1300,1700,1500,1500,1500,1500,1250,1800,1500,1500,500,1500,1500);    // Line # 28
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1500,1500,1700,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,1500);    // Line # 29
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,1500);    // Line # 30
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500);   // Line # 31
  delay(1000);   //Default Delay
}

void walking(){
  ServoSetAll(1500,550,2057,1500,1886,1025,1750,1500,1500,1500,1500,1250,1975,1135,1500,1913,2450,1500);    // Line # 0
  delay(300);   //Default Delay
  ServoSetAll(1500,550,2057,1500,1865,1100,1750,1404,1500,1500,1500,1500,1900,907,1500,1913,2450,1500);   // Line # 1
  delay(300);   //Default Delay
  ServoSetAll(1500,550,2057,1500,1865,1100,1750,1445,1500,1500,1500,1500,1900,907,1500,1913,2450,1500);   // Line # 2
  delay(300);   //Default Delay
  ServoSetAll(1500,550,2057,1500,2105,835,1750,1500,1500,1500,1653,1500,1900,907,1500,1913,2450,1500);    // Line # 3
  delay(300);   //Default Delay
  ServoSetAll(1500,550,2057,1500,2105,1243,1374,1500,1500,1500,1653,1500,1900,907,1500,1913,2450,1500);   // Line # 4
  delay(300);   //Default Delay
  ServoSetAll(1500,550,1099,1500,1865,1326,1500,1500,1500,1500,1518,1500,1761,1135,1500,931,2450,1500);   // Line # 5
  delay(300);   //Default Delay
  ServoSetAll(1500,550,1099,1500,1886,1025,1750,1500,1500,1500,1500,1250,1975,1135,1500,931,2450,1500);   // Line # 6
  delay(300);   //Default Delay
  ServoSetAll(1500,550,1099,1500,2165,943,1650,1500,1500,1500,1650,1250,1975,1135,1500,931,2450,1500);    // Line # 7
  delay(300);   //Default Delay
  ServoSetAll(1500,550,1099,1500,2165,943,1650,1500,1500,1500,1506,1207,2045,1135,1500,931,2450,1500);    // Line # 8
  delay(300);   //Default Delay
  ServoSetAll(1500,550,1099,1500,1886,943,1817,1450,1500,1500,1506,1243,2105,1135,1500,931,2450,1500);    // Line # 9
  delay(300);   //Default Delay
}

void hiphop(){
  ServoSetAll(1500,1100,1650,1500,1700,1700,1300,1500,1500,1500,1500,1700,1300,1300,1500,1350,1900,1500);    // Line # 0
  delay(1000);   //Default Delay
  ServoSetAll(1500,1100,1650,1500,1900,1900,1100,1500,1500,1500,1500,1900,1100,1100,1500,1350,1900,1500);   // Line # 1
  delay(1000);   //Default Delay
  ServoSetAll(1500,1100,1650,1500,2000,2300,900,1500,1500,1500,1500,2100,700,1000,1500,1350,1900,1500);   // Line # 2
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,1650,1500,2500,2300,500,1500,1500,1500,1500,2484,700,500,1500,1350,2300,1500);   // Line # 3
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,1650,1500,2500,2500,500,1500,1500,1500,1500,2344,500,500,1500,1350,2300,1500);   // Line # 4
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,2500,1500,2500,2500,1500,1500,1500,1500,1500,1500,500,500,1500,500,2300,1500);   // Line # 5
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,2500,1500,1000,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,500,2300,1500);    // Line # 6
  delay(1000);   //Default Delay
  delay(100);   // Line # 7
  ServoSetAll(1500,700,2500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,2000,1500,500,2300,1500);   // Line # 8
  delay(1000);   //Default Delay
  delay(100);   // Line # 9
  ServoSetAll(1500,700,2500,1500,1000,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,500,2300,1500);    // Line # 10
  delay(1000);   //Default Delay
  delay(100);   // Line # 11
  ServoSetAll(1500,700,2500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,2000,1500,500,2300,1500);   // Line # 12
  delay(1000);   //Default Delay
  delay(100);   // Line # 13
  ServoSetAll(1500,700,2500,1500,1000,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,500,2300,1500);    // Line # 14
  delay(1000);   //Default Delay
  delay(100);   // Line # 15
  ServoSetAll(1500,700,2500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,2000,1500,500,2300,1500);   // Line # 16
  delay(1000);   //Default Delay
  delay(100);   // Line # 17
  ServoSetAll(1500,700,2500,1500,1000,1500,1500,1500,1500,1500,1500,1500,1500,500,1500,500,2300,1500);    // Line # 18
  delay(1000);   //Default Delay
  delay(100);   // Line # 19
  ServoSetAll(1500,700,2500,1500,2500,1500,1500,1500,1500,1500,1500,1500,1500,2000,1500,500,2300,1500);   // Line # 20
  delay(1000);   //Default Delay
  delay(100);   // Line # 21
  ServoSetAll(1500,700,2200,1500,2500,2080,1500,1500,1500,1500,1500,1500,1000,500,1500,800,2300,1500);    // Line # 22
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,2200,1500,2500,2300,1000,1500,1500,1500,1500,2000,900,500,1500,800,2300,1500);   // Line # 23
  delay(1000);   //Default Delay
  ServoSetAll(1500,700,2200,1500,2500,2300,700,1500,1500,1500,1500,2300,900,500,1500,800,2300,1500);    // Line # 24
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,2100,2500,850,1500,1500,1500,1500,2150,500,900,1500,500,2500,1500);    // Line # 25
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,2100,2500,500,1500,1500,1500,1500,2500,500,900,1500,500,2500,1500);    // Line # 26
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1900,2500,656,1500,1500,1500,1500,2500,500,1100,1500,500,2500,1500);   // Line # 27
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1500,2500,700,1500,1500,1500,1500,2200,500,1500,1500,500,2500,1500);   // Line # 28
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1500,2250,900,1500,1500,1500,1500,2100,750,1500,1500,500,2500,1500);   // Line # 29
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1750,1900,1100,1500,1500,1500,1500,1900,1100,1250,1500,500,2500,1500);   // Line # 30
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1500,1700,1300,1500,1500,1500,1500,1700,1299,1500,1500,500,2500,1500);   // Line # 31
  delay(1000);   //Default Delay
  ServoSetAll(1500,500,2500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,500,2500,1500);   // Line # 32
  delay(1000);   //Default Delay
  ServoSetAll(1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500,1500);   // Line # 33
  delay(1000);   //Default Delay
}