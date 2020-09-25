  
#include <SPI.h>
#include "mcp_can.h"

const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

unsigned char flagRecv = 0;
unsigned char len;
unsigned char buf[8];
const int Nlen=100;
unsigned char str[Nlen];
unsigned long str_id[Nlen];
unsigned int tec[Nlen];
unsigned int rec[Nlen];
unsigned int count_str=0;
unsigned int count_loop=0;

void setup()
{
    Serial.begin(1000000);

    while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
    delay(100);                  
}

void loop()
{   
  while(CAN_MSGAVAIL == CAN.checkReceive()){
    CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf 
    str_id[count_str]=CAN.getCanId();
    str[count_str]=buf[len-1]; 
    
    tec[count_str]= readTEC();
    rec[count_str]=readREC();
    count_str=count_str+1;
    if(count_str>Nlen-1) {
      count_str=0;        
      for(int i = 0; i<Nlen; i++) {   // print the data
        Serial.print("Count:"); 
        String stringCount =  String(i);
        while (stringCount.length()<2) stringCount = "0" + stringCount; 
        Serial.print(stringCount);  
        Serial.print("\t ID:0x");     
        String stringID =  String(str_id[i], HEX);
//        while (stringID.length()<3) stringID = "0" + stringID; 
        Serial.print(stringID);    
        Serial.print("\t DATA:0x");
        Serial.print(str[i], HEX);
        Serial.print("\t TEC:");
        Serial.print(tec[i]);
        Serial.print("\t REC:");
        Serial.println(rec[i]);
      }
      Serial.println();              
    }
  }
}

inline byte readTEC(){
  return CAN.mcp2515_readRegister(MCP_TEC);
}

inline byte readREC(){
  return CAN.mcp2515_readRegister(MCP_REC);
}
