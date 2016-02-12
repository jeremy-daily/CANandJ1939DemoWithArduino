/*CAN and J1939 Demo
*/

//Load Headers
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

#define CANinteruptPin 2
#define greenPin 6
#define redPin   3
#define switchPin 15

MCP_CAN CAN0(10); // Set CS to pin 10

String IDstring = "TULSA*MiniCaper*NodeA-RedLED with Switch*1";
char compID[29]; //Declare a 29 byte place holder to convert the component ID string ro a character array.

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8]; 
boolean switchState;
boolean redState;
boolean greenState;
boolean redLEDerror = false;
boolean greenLEDerror = false;

unsigned char messageCount = 0;

byte SA = 0x0A; //Source Address
byte startSA = 0x0A
byte endSA = 0xFA

ucJ1939Name[8] = { 0x00, 0x00, 0x60, 0x01, 0x00, 0x81, 0x00, 0x00 };


byte CANdata[8] = {0, 0, 0, 0, 0, 0, 0, 0};

byte lightingStatusData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

byte lightingCommandData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

int greenBrightness = 255;
int redBrightness = 0;

unsigned long currentMillis = 0;
unsigned long previousMillis100 = 0;
unsigned long previousMillis1000 = 0;
unsigned long PGN = 0;

boolean addressClaimed = false;

/********************************************************************
Setup is always run 
********************************************************************/
void setup(){
  IDstring.toCharArray(compID,29); // Convert the component ID to a character array for CAN
  
  pinMode(switchPin,INPUT_PULLUP);
  pinMode(greenPin,OUTPUT);
  pinMode(redPin,OUTPUT);
  
  analogWrite(greenPin,greenBrightness);
  analogWrite(redPin,redBrightness);
  switchState=digitalRead(switchPin);
  
  
  Serial.begin(115200);
  
  // initialize the CAN bus
  Serial.println("Setting up CAN0 for J1939..."); //J1939
  if(CAN0.begin(CAN_250KBPS) == CAN_OK) Serial.println("CAN0 init ok!!");
  else Serial.print("CAN0 init fail!!\r\n");
  
  
  while (!addressClaimed)
  {
    //Transmit Address Request
    startAddressClaim = millis();
    CAN0.sendMsgBuf((0x18EEFF00 + SA), 1, 8, ucJ1939Name);
    while (millis() - startAddressClaim < 250)
    {
      processCANmessage();
      if (PGN == 60928){ // Listen for another address claim
        if ((rxId & 0x000000FF) == SA) //Check to see if new address is equal to current SA
        {
          //Insert code to arbitrate using NAME fields. 
          // is ucJ1939Name > rxBuf ?
        }
      }
    }
    
  }  
  
  
  //Send a clear display message to Node B
  CANdata[0] = byte(0); //Clear Display
  CAN0.sendMsgBuf(0x18A80B0B, 1, 8, CANdata);
}

void processCANmessage(){
    CAN0.readMsgBuf(&len, rxBuf);              // Read data: len = data length, buf = data byte(s)
    rxId = CAN0.getCanId();    // Get message ID
    unsigned long tempPGN = rxId & 0x03FFFF00) >> 8;
    byte PDUFormat = (tempPGN & 0x0000FF00) >> 8;
    if {PDUFormat >= 240)
    {
      PGN = tempPGN; //PDU 2 Format
      DA = 255;
    }
    else
    {
      DA =  tempPGN & 0x000000FF;
      PGN = tempPGN & 0x0003FF00;
    }
    if (DA == SA) {
    Serial.println("Hey! Someone called on me!");
    )
    if ((rxId & 0x00FF0000) == 0x00EA0000) //Check to see if the message is a request message.
    {
        Serial.print("Request Received.");
        if (rxBuf[0] == 0xEB && rxBuf[1] == 0xFE) sendComponentInfo(compID); // Component ID PGN is 0xFEEB
        else if (rxBuf[0] == 0x40 && rxBuf[1] == 0xFE) sendLightingInfo(); //Lighting Status is 0xFE40
    }
    else if ((rxId & 0x03FFFF00) == 0x00FE4C00) //Military Lighting Command 
    {
      redBrightness = rxBuf[7];
      greenBrightness = rxBuf[7];
      Serial.print("Intensity: ");
      Serial.println(0.4*rxBuf[7]);
    }
    else if ( ((rxId & 0x03FFFF00)) >> 8 == 0x00FE4100) // Lighting Command 
    {
      redState = bitRead(rxBuf[6],1);
      greenState = bitRead(rxBuf[6],3);
    }
    else if (  == 61444) //Check to see if the message is Electronic Engine Controller 1
    {
      int rpm = (rxBuf[3] + (rxBuf[4] << 8)); //Calculate engine RPM
      if (rpm >= 8031) Serial.print("Engine RPM: Out of Range");
      else
      {
        Serial.print("Engine RPM: ");
        Serial.println(rpm);
      }
      byte spn899 // 
      
    }
    else if ( ((rxId & 0x00FFFF00) >> 8) == 65263) //Check to see if the message is Engine Fluid Level Presure
    {
      
      int spn94 = rxBuf[0] * 4 ;// kpa
      if (spn94 <= 1000 && spn94 >= 0){
        Serial.print("Engine Fuel Delivery Pressure = ");
        Serial.print(spn94);
        Serial.println(" kPa");
      }
      else{
        Serial.println("Engine Fuel Delivery Pressure = Out of Range");
      }
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
       for (int i = 0; i<28;i++) Serial.print(id[i]);
       Serial.println();
       
       byte transport0[8] = {32,35,0,5,0xFF,0xEB,0xFE,0};
       byte transport1[8] = {1,id[0],id[1],id[2],id[3],id[4],id[5],id[6]};
       byte transport2[8] = {2,id[7],id[8],id[9],id[10],id[11],id[12],id[13]};
       byte transport3[8] = {3,id[14],id[15],id[16],id[17],id[18],id[19],id[20]};
       byte transport4[8] = {4,id[21],id[22],id[23],id[24],id[25],id[26],id[27]};
       byte transport5[8] = {5,id[28],id[29],id[30],id[31],id[32],id[33],id[34]};
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

 
  
 currentMillis = millis();
 if (currentMillis - previousMillis1000 >= 1000){
    previousMillis1000 = currentMillis;
    
    switchState = digitalRead(switchPin);
    greenState = switchState;
    bitWrite(lightingCommandData[6],3,greenState);
    CAN0.sendMsgBuf(0x18FE410D, 1, 8, lightingCommandData); //Send data for Green LEDs
    
    Serial.print("Green State: ");
    Serial.println(switchState);
  }
}
