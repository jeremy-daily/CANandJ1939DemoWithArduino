/*CAN and J1939  LED and a potentiometer
Decodes CAN messages as if they were J1939
Sample Source code for Arduino running on a MiniCaper (Arduino Pro Mini with MCP2515 CAN Controller)

Initially Written by
Dr. Jeremy Daily
The University of Tulsa
*/

//Load Headers
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

#define CANinteruptPin 2
#define redPin   3
#define potPin  A2

MCP_CAN CAN0(10); // Set SPI chip select to pin 10

//Hard code the data for Component ID. Fields are separated by an asterisk (*). 
String IDstring = "TULSA*MiniCaper*NodeF-RedLED with pot*1";
char compID[40]; //Declare a 29 byte place holder to convert the component ID string ro a character array.

//Set up CAN message data structures
long unsigned int rxID; //Identifier
unsigned char len = 0; //Length
unsigned char rxBuf[8]; //Receive buffer

//Initialize the potentiometer and LED variables
int potState;
boolean redState = true;
boolean redLEDerror = false;
int redBrightness = 0;

//Initialize the data processing variables
unsigned char messageCount = 0;

//Set up J1939 ID sections
byte Priority = 7;
unsigned long int PGN; //Parameter Group Number
byte SA = 0x0D; //Source Address
byte DA = 0xFF; //Destination Address
byte PF = 0x00; //Protocol Format
boolean DP = 0; //Data Page
boolean EDP = 0; //Extended Data Page


byte CANdata[8] = {0, 0, 0, 0, 0, 0, 0, 0}; //an eight byte array for a message

byte lightingStatusData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

byte lightingCommandData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

//Initialze timing variables for broadcasting periodic messages
unsigned long currentMillis = 0;
unsigned long previousMillis100 = 0;
unsigned long previousMillis1000 = 0;

/********************************************************************
Setup is always run once at the beginning of the execution.
Set up serial feedback, CAN connection at 250k
********************************************************************/
void setup(){
  IDstring.toCharArray(compID,29); // Convert the component ID to a character array for CAN
  
  pinMode(potPin,INPUT);
  pinMode(redPin,OUTPUT); 
  
  Serial.begin(115200);
  
  // initialize the CAN bus
  Serial.println("Setting up CAN0 for J1939..."); //J1939
  if(CAN0.begin(CAN_250KBPS) == CAN_OK) Serial.println("CAN0 init ok!!");
  else Serial.print("CAN0 init fail!!\r\n");
}

/********************************************************************
Run this function when a CAN message is received.
********************************************************************/
void processCANmessage(){
  if(!digitalRead(CANinteruptPin)) //Check the CAN interrupt Pin for a current message
  {
    CAN0.readMsgBuf(&len, rxBuf);              // Read data: len = data length, buf = data byte(s)
    rxID = CAN0.getCanId();    // Get message ID
   
    Priority = (rxID & 0x1C000000) >> 26;
    EDP = (rxID & 0x01000000) >> 24;
    DP  = (rxID & 0x02000000) >> 25;
    PF = (rxID & 0x00FF0000) >> 16;
    if (PF >= 240)
    {
      PGN = (rxID & 0x00FFFF00) >> 8;
      DA = 0xFF;
    }
    else
    {
      PGN = (rxID & 0x00FF0000) >> 8;
      DA = (rxID & 0x0000FF00) >> 8;
    }
    SA = (rxID & 0xFF);
          
    if ((rxID & 0x00FF0000) == 0x00EA0000) //Check to see if the message is a request message.
      {
          Serial.print("Request Received.");
          if (rxBuf[0] == 0xEB && rxBuf[1] == 0xFE) sendComponentInfo(compID); // Component ID PGN is 0x00FEEB
          else if (rxBuf[0] == 0x40 && rxBuf[1] == 0xFE) sendLightingInfo(); //Lighting Status is 0xFE40
          //Add more PGNS that will respond to requests here.
      }
      else if ((rxID & 0x00FFFF00) == 0x00FEF100) //Check for Cruise Control Vehicle Speed (CCVS) PGN (0x00FEF1)
      {
          outputCANmessage();
      }
   }
}


void outputCANmessage(){
    Serial.print(micros());
    Serial.print(" ");      
    Serial.print(rxID, HEX);
    Serial.print(" ");      
    Serial.print(Priority);      
    Serial.print(" ");      
    Serial.print(PGN);      
    Serial.print(" ");      
    Serial.print(DA);      
    Serial.print(" ");      
    Serial.print(SA);      
    Serial.print(" ");      
    
    for(int i = 0; i<len; i++)                // Print each byte of the data
    {
      if(rxBuf[i] < 0x10)                     // If data byte is less than 0x10, add a leading zero
      {
        Serial.print("0");
      }
      Serial.print(rxBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
}

void sendLightingInfo()
{
  //Reference SPN2398 for the RED LED
  if (redLEDerror)  
  {
    bitSet(lightingStatusData[6],0);
    bitClear(lightingStatusData[6],1);
  } 
  else
 {
   bitClear(lightingStatusData[6],0);
   if (redState) bitSet(lightingStatusData[6],1);
   else bitSet(lightingStatusData[6],0);
 }
 
 
 CAN0.sendMsgBuf(0x18FE400D, 1, 8, lightingStatusData);
}


void sendComponentInfo(char id[29])
{
       Serial.print("Received Request for Component ID. Sending  ");
       for (int i = 0; i<34;i++) Serial.print(id[i]);
       Serial.println();
       
       byte transport0[8] = {32,28,0,4,0xFF,0xEB,0xFE,0};
       byte transport1[8] = {1,id[0],id[1],id[2],id[3],id[4],id[5],id[6]};
       byte transport2[8] = {2,id[7],id[8],id[9],id[10],id[11],id[12],id[13]};
       byte transport3[8] = {3,id[14],id[15],id[16],id[17],id[18],id[19],id[20]};
       byte transport4[8] = {4,id[21],id[22],id[23],id[24],id[25],id[26],id[27]};
       byte transport5[8] = {4,id[28],id[29],id[30],id[31],id[32],id[33],id[34]};
       CAN0.sendMsgBuf(0x1CECFF0D, 1, 8, transport0);
       delay(50);
       CAN0.sendMsgBuf(0x1CEBFF0D, 1, 8, transport1);
       delay(50);
       CAN0.sendMsgBuf(0x1CEBFF0D, 1, 8, transport2);
       delay(50);
       CAN0.sendMsgBuf(0x1CEBFF0D, 1, 8, transport3);
       delay(50);
       CAN0.sendMsgBuf(0x1CEBFF0D, 1, 8, transport4);  
       delay(50);
       CAN0.sendMsgBuf(0x1CEBFF0D, 1, 8, transport5);      
}
      

void loop(){
 
 processCANmessage();
  
 //Control the LEDs
 if (redState) analogWrite(redPin,redBrightness);
 else digitalWrite(redPin,LOW);
 
 potState = analogRead(potPin);
 redBrightness=potState/4;
 
 currentMillis = millis();
 if (currentMillis - previousMillis100 >= 100){
    previousMillis100 = currentMillis;
    
    potState = analogRead(potPin);
    lightingCommandData[7]=potState/4;
    CAN0.sendMsgBuf(0x18FE4C0F, 1, 8, lightingCommandData); //Send data for LED
  }
}
