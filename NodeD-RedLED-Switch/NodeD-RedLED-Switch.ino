#include <ARD1939.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

#include <SoftwareSerial.h>
//#include <SPI.h>

#define CANinteruptPin 2
#define greenPin 6
#define redPin   3

SoftwareSerial displaySerial(8, 9); // RX, TX

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
boolean newMessage = false;
byte messageCount = 0;


//MCP_CAN CAN0(10); // Set CS to pin 10

byte SA = 0x04; //Source Address
byte CANdata[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int greenBrightness = 255;
int redBrightness = 0;

unsigned long currentMillis = 0;
unsigned long previousMillis100 = 0;
unsigned long previousMillis1000 = 0;

void setup(){
  pinMode(greenPin,OUTPUT);
  pinMode(redPin,OUTPUT);
  analogWrite(greenPin,greenBrightness);
  analogWrite(redPin,redBrightness);
  //attachInterrupt(0,readCAN,LOW);
  
  Serial.begin(115200);
  displaySerial.begin(9600);

  //Set backlight brightness
  displaySerial.write(0x7C);
  displaySerial.write(150);
  delay(100);
  displaySerial.write(0xFE);
  displaySerial.write(0x01); //clear Display
  delay(100);
  displaySerial.write(0xFE);
  displaySerial.write(0x0C);
  delay(100);
  displaySerial.write(0xFE);
  displaySerial.write(0x80+0); //Move to cursor Position 1
  delay(100);
  displaySerial.print("CAN Demo");
  
  
  // init can bus
  Serial.println("Setting up CAN0 for J1939..."); //J1939
  if(CAN0.begin(CAN_250KBPS) == CAN_OK) Serial.println("CAN0 init ok!!");
  else Serial.print("CAN0 init fail!!\r\n");
  
  CANdata[0] = byte(0); //Clear Display
  CAN0.sendMsgBuf(0x18A80B0C, 1, 8, CANdata);
}

void printMessage()
{
  for (int i = 2; i<8; i++)
  {
    if (isprint(rxBuf[i])) displaySerial.write(rxBuf[i]);
    else break;
  }  
}

void readCAN(){
      
      CAN0.readMsgBuf(&len, rxBuf);              // Read data: len = data length, buf = data byte(s)
      rxId = CAN0.getCanId();
      if ( ((rxId & 0x00FF0000) >> 8) == 0x43008) 
      {
        if (rxBuf[0] & 0x0F == 0)
        {
          //Clear Display
        }
        else if (rxBuf[0] & 0x0F == 3)
        {
          // overwrite display from position 1
          displaySerial.write(0xFE);
          displaySerial.write(0x80+0); //Move to cursor Position 1
          printMessage();
          
        }
        else if (rxBuf[0] & 0x0F == 4)
        {
          //overwrite substring
          displaySerial.write(0xFE);
          displaySerial.write(0x80+rxBuf[1]); //Move to cursor on byte 2 
          printMessage();
        }
      }  
      
} 
void loop(){
 // analogWrite(redPin,redBrightness);
 analogWrite(greenPin,greenBrightness);  
  
  currentMillis = millis();
  if (currentMillis - previousMillis1000 >= 1000){
    previousMillis1000 = currentMillis;
    CANdata[0] = byte(3); //overwrite String
    CANdata[1] = byte(0); //position
    CANdata[2] = byte('H');
    CANdata[3] = byte('e');
    CANdata[4] = byte('l');
    CANdata[5] = byte('l');
    CANdata[6] = byte('o');
    CANdata[7] = byte(' ');
    
    CAN0.sendMsgBuf(0x18A80B0C, 1, 8, CANdata);
    //displaySerial.write(0xFE);
    //displaySerial.write(0x80+0); //Move to cursor Position 1
    delay(100);
    messageCount+=1;
    CANdata[0] = byte(4); //overwrite String
    CANdata[1] = byte(6); //position
    CANdata[2] = byte('W');
    CANdata[3] = byte('o');
    CANdata[4] = byte('r');
    CANdata[5] = byte('l');
    CANdata[6] = byte('d');
    CANdata[7] = byte(messageCount);
    Serial.println(CANdata[7]);
    CAN0.sendMsgBuf(0x18A80B0C, 1, 8, CANdata);
    
    //displaySerial.print(messageCount);
  }
  
  if(!digitalRead(2)) readCAN();
  
//  if (newMessage){   // If pin 2 is low, read receive buffer
//      newMessage = false;
//      //CAN0.readMsgBuf(&len, rxBuf);
//     //# messageCount +=1;
//      //rxId = CAN0.getCanId();                    // Get message ID
//      Serial.print(messageCount);
//      Serial.print("  ");
//      Serial.print(rxId, HEX);
//      Serial.print(" ");
//      for(int i = 0; i<len; i++)                // Print each byte of the data
//      {
//        if(rxBuf[i] < 0x10)                     // If data byte is less than 0x10, add a leading zero
//        {
//          Serial.print("0");
//        }
//        Serial.print(rxBuf[i], HEX);
//        Serial.print(" ");
//      }
//      Serial.println();
//  }
}
