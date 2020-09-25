#include <SPI.h>
#include <math.h>
#include "mcp_can.h"

const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin


///// Constant values ////
#define freq 16 //MHz
#define overflowTime (65536/freq) //us, 2^16/freq
#define period 10000 //us //periodicity

const int phaseChangeToTestID = 513;
const int phaseChangeToTrainID = 512;
const unsigned char phaseChangeMsg[8] = {0, 0, 0, 0, 0, 0, 0, 0};

const bool trainPhase=true; // Set this for Duet Exp

void setup()
{
  Serial.begin(1000000);
  while (CAN_OK != CAN.begin(CAN_500KBPS))  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");
  delay(100);
  
  if(trainPhase) {
    CAN.sendMsgBuf(phaseChangeToTrainID, 0, 8, phaseChangeMsg, false);
    Serial.println("Training Phase");
  } else {
    CAN.sendMsgBuf(phaseChangeToTestID, 0, 8, phaseChangeMsg, false);
    Serial.println("Test Phase");
  } 
}

void loop() { 
}
