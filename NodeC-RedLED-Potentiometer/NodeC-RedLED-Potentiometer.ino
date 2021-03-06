/*CAN and J1939  LED and a pot
Function: Controll Green LEDs
*/

//Load Headers
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

#define CANinteruptPin 2
#define greenPin 6
#define redPin   3
#define potPin  A2

MCP_CAN CAN0(10); // Set CS to pin 10

String IDstring = "TULSA*MiniCaper*NodeC-RedLED with pot*1";
char compID[40]; //Declare a 29 byte place holder to convert the component ID string ro a character array.

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8]; 
int potState;
boolean redState;
boolean greenState;
boolean redLEDerror = false;
boolean greenLEDerror = false;

unsigned char messageCount = 0;

byte SA = 0x0D; //Source Address

byte CANdata[8] = {0, 0, 0, 0, 0, 0, 0, 0};

byte lightingStatusData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

byte lightingCommandData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

int greenBrightness = 255;
int redBrightness = 0;

unsigned long currentMillis = 0;
unsigned long previousMillis100 = 0;
unsigned long previousMillis1000 = 0;

/********************************************************************
Setup is always run 
********************************************************************/
void setup(){
  IDstring.toCharArray(compID,29); // Convert the component ID to a character array for CAN
  
  pinMode(potPin,INPUT);
  pinMode(greenPin,OUTPUT);
  pinMode(redPin,OUTPUT);
  
  analogWrite(greenPin,greenBrightness);
  analogWrite(redPin,redBrightness);
  potState=analogRead(potPin);
  
  
  Serial.begin(115200);
  
  // initialize the CAN bus
  Serial.println("Setting up CAN0 for J1939..."); //J1939
  if(CAN0.begin(CAN_250KBPS) == CAN_OK) Serial.println("CAN0 init ok!!");
  else Serial.print("CAN0 init fail!!\r\n");
  
  //Send a clear display message to Node B
  CANdata[0] = byte(0); //Clear Display
  CAN0.sendMsgBuf(0x18A80B0B, 1, 8, CANdata);
}

void processCANmessage(){
  if(!digitalRead(2)){
    CAN0.readMsgBuf(&len, rxBuf);              // Read data: len = data length, buf = data byte(s)
    rxId = CAN0.getCanId();    // Get message ID
    
    
    if ((rxId & 0x00FF0000) == 0x00EA0000) //Check to see if the message is a request message.
    {
        Serial.print("Request Received.");
        if (rxBuf[0] == 0xEB && rxBuf[1] == 0xFE) sendComponentInfo(compID); // Component ID PGN is 0xFEEB
        else if (rxBuf[0] == 0x40 && rxBuf[1] == 0xFE) sendLightingInfo(); //Lighting Status is 0xFE40
    }
    else if ((rxId & 0x00FFFF00) == 0x00FE4100) // Lighting Command 
    {
      redState = bitRead(rxBuf[6],1);
      greenState = bitRead(rxBuf[6],3);
    }
//    Serial.print(rxId, HEX);
//    Serial.print(" ");
//    for(int i = 0; i<len; i++)                // Print each byte of the data
//    {
//      if(rxBuf[i] < 0x10)                     // If data byte is less than 0x10, add a leading zero
//      {
//        Serial.print("0");
//      }
//      Serial.print(rxBuf[i], HEX);
//      Serial.print(" ");
//    }
//    Serial.println();
  }
    
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
 
 //Reference SPN2396 which is for the Green LED
 if (greenLEDerror)  
  {
    bitSet(lightingStatusData[6],2);
    bitClear(lightingStatusData[6],3);
  } 
  else
 {
   bitClear(lightingStatusData[6],2);
   if (redState) bitSet(lightingStatusData[6],3);
   else bitSet(lightingStatusData[6],2);
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
 //digitalWrite(redPin,HIGH);
 
  
 currentMillis = millis();
 if (currentMillis - previousMillis1000 >= 1000){
    previousMillis1000 = currentMillis;
    
    potState = analogRead(potPin);
    lightingCommandData[7]=potState/4;
    CAN0.sendMsgBuf(0x18FE4C0C, 1, 8, lightingCommandData); //Send data for Green LEDs
    
    Serial.print("Potentiometer: ");
    Serial.println(potState);
  }
}
