#include <SPI.h>
#include "mcp_can.h"

const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);

#include "victim.h"
#include "raid.h"

int lastTEC = 0;
bool enableRAID = false;
unsigned long txID;

void setup() {
  commonSetup();
  for (int i=0;i<2;i++)  CAN.init_Mask(i, 0, 0x3ff);

  CAN.init_Filt(0, 0, phaseChangeToTestID);
  CAN.init_Filt(1, 0, phaseChangeToTestID);
  CAN.init_Filt(2, 0, phaseChangeToTrainID);
  CAN.init_Filt(3, 0, phaseChangeToTrainID);
  CAN.init_Filt(4, 0, phaseChangeToTrainID);
  CAN.init_Filt(5, 0, phaseChangeToTrainID);

  Serial.println("Train Phase: Victim transmitting");
  startCleanTimer();
}


void loop() {

  if (victimTEC>50 && lastTEC>victimTEC){
    int bufToCheck[] = {0,1,2};
    if (noTXPending(bufToCheck,3)){
      int filterIDs[2] = {phaseChangeToTestID,phaseChangeToTrainID};
      resetCANController(filterIDs,2);
      victimTEC = readTEC();

    }
  }

  while (CAN_MSGAVAIL == CAN.checkReceive()){
    int receivedID = readMsgID();
    //sprintf(strBuf, "Received ID:%ld", receivedID);
    //Serial.println(strBuf);
    if (receivedID==phaseChangeToTestID){
        trainingPhase = false;
        setupPhase();
        Serial.println("Test Phase: Victim stopped transmission");
    } else if (receivedID==phaseChangeToTrainID) {
        trainingPhase = true;
        setupPhase();
        Serial.println("Train Phase: Victim transmitting");
    }
  }

  for (int i=0; i<victim_numIDs; i++){
    if (victim_sendIDs[i]){
      if (shouldWakeup(victim_nextWakeupAtOverflows[i],
                victim_nextWakeupAtCounter[i])){
        setWakeupTimer((uint64_t)(victim_periods[i]*1000), &(victim_nextWakeupAtOverflows[i]), &(victim_nextWakeupAtCounter[i]), false);
        if (trainingPhase && enableRAID) {
          txID=padID(victim_IDs[i]);
          sendInAnyBuf(txID, victimMsg, 1);
        }
        else {
          txID=victim_IDs[i];
          sendInAnyBuf(txID, victimMsg, 0);
        }
        lastTEC = victimTEC;
        victimTEC = readTEC();
      }
    }
  }

//  int tec = readTEC(true);
//  if (lastTEC!=tec){
//    printTEC();
//    lastTEC = tec;
//  }

}
