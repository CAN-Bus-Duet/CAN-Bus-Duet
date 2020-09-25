//TIMING PARAMS
#define frameLength 123 //108 + 15 bit stuffing
#define usPerBit 2 // us
#define freq 16 //MHz
#define frameTime (frameLength*usPerBit) //us
#define frameWithErrorTime ((frameLength-7)*usPerBit) //us
#define receiveDelay 330 //us, was 330 on testbed, 380 for 70% with 0C1
#define startTime ((2*frameTime) + receiveDelay) //us
#define overflowTime (65536/freq) //us, 2^16/freq
#define period 10000 //us //periodicity
#define periodMs (((float)period)/1000) //ms

bool trainingPhase = true;

////IDs and Payloads

int accompliceIDBase = 0x001;
int attackerIDBase = 0x00B;
int victimIDBase = 0x015;

int victimID = victimIDBase;

int precedeID1 = accompliceIDBase;
int precedeID2 = accompliceIDBase+1;
int accompliceID = accompliceIDBase+2;
int precedeID3 = accompliceIDBase+3;
int precedeID4 = accompliceIDBase+4;

int phaseChangeToTestID = 513;
int phaseChangeToTrainID = 512;

const int victim_numIDs = 4;
int victim_IDs[victim_numIDs] = {victimIDBase, victimIDBase+1, victimIDBase+2, victimIDBase+3}; 
bool victim_sendIDs[victim_numIDs] = {true, false, false, false};
float victim_periods[victim_numIDs] = {periodMs, periodMs, 100, 100};
unsigned long victim_nextWakeupAtOverflows[victim_numIDs] = {2, 3, 4, 5};
unsigned int victim_nextWakeupAtCounter[victim_numIDs] = {100, 200, 300, 400};

const int accomplice_numIDs = 6;
int accomplice_IDs[accomplice_numIDs] = {precedeID1, precedeID2, accompliceID, precedeID3, precedeID4, victimID};
bool accomplice_sendIDs[accomplice_numIDs] = {true, true, true, true, true, false}; 
float accomplice_periods[accomplice_numIDs] = {periodMs, periodMs, periodMs, periodMs, periodMs, periodMs};
unsigned long accomplice_nextWakeupAtOverflows[accomplice_numIDs] = {2, 3, 4, 5, 6, 7};
unsigned int accomplice_nextWakeupAtCounter[accomplice_numIDs] = {100, 200, 300, 400, 500, 600};
int particularBufs[accomplice_numIDs] = {0, 1, 2, 0, 1, 2};

const int attacker_numIDs = 4;
int attacker_IDs[attacker_numIDs] = {attackerIDBase, attackerIDBase+1, attackerIDBase+2, attackerIDBase+3};
bool attacker_sendIDs[attacker_numIDs] = {true, false, false, false};
float attacker_periods[attacker_numIDs] = {periodMs, periodMs, 100, 100};
unsigned long attacker_nextWakeupAtOverflows[attacker_numIDs] = {2, 3, 4, 5};
unsigned int attacker_nextWakeupAtCounter[attacker_numIDs] = {100, 200, 300, 400};

byte msgLen = 8;

unsigned char victimMsg[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char accompliceBenignMsg[8] = {0, 0, 0, 0, 0, 0, 0, 127};
unsigned char accompliceSignalToattackerMsg[8] = {0, 0, 0, 0, 0, 0, 0, 1};
unsigned char attackerMsg[8] = {0, 0, 0, 0, 0, 0, 0, 63};


////Estimated TEC
int victimTEC = 0;
int accompliceTEC = 0;
int attackerTEC = 0;


//General vars
volatile unsigned long overflows = 0;  
unsigned char len = 0;
unsigned char buf[8];
char strBuf[80];

/*-------------------------------------------
---------------Functions---------------------
-------------------------------------------*/

inline void setupPhase(){
	if (!trainingPhase){
		victim_sendIDs[0] = false;
		accomplice_sendIDs[5] = true;
	
	} else {
		victim_sendIDs[0] = true;
		accomplice_sendIDs[5] = false;
	}
}


inline uint64_t getElapsedTime(){ //in us
  uint64_t currOverflows = overflows; 
  unsigned int currTimer = TCNT1; 
  return (currOverflows*overflowTime + currTimer/freq);
}


inline long getCounter(){
  return TCNT1;
}

inline void commonSetup(){
  setupPhase();
  Serial.begin(1000000);
  while (CAN_OK != CAN.begin(CAN_500KBPS))             
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");
  CAN.mcp2515_modifyRegister(MCP_TXB0CTRL,3,3);
  CAN.mcp2515_modifyRegister(MCP_TXB1CTRL,3,2);
  CAN.mcp2515_modifyRegister(MCP_TXB2CTRL,3,1);
  TCCR1A = 0;  
  TCCR1B = 0;  
  TCCR1C = 0;  
  TIMSK1 = _BV(TOIE1);
  delay(5000);
}

inline void startCleanTimer(){
  overflows = 0; TCNT1 = 0; TCCR1B = 1;
}

inline byte readTEC(bool sleep = false){
  if (sleep) delay(1);
  return CAN.mcp2515_readRegister(MCP_TEC);
}

inline void printTEC(bool sleep = false){
  if (sleep) delay(1);
  byte tec = readTEC();
  sprintf(strBuf, "TEC:%d, Predicted(Att:%d, Acc:%d, Vic:%d)", tec, accompliceTEC, attackerTEC, victimTEC);
  Serial.println(strBuf);
}

inline void emptyBuffers(){
  while (CAN_MSGAVAIL == CAN.checkReceive()){
    CAN.readMsgBuf(&len, buf);
  }
}

inline long readMsgID() {
  CAN.readMsgBuf(&len, buf);
  return CAN.getCanId();
}

inline void waitNewMsg(){
  bool received = false;
  unsigned long receivedID=0;
  while (!received){
    while (CAN_MSGAVAIL == CAN.checkReceive()){
		receivedID=readMsgID();
		if(CAN.isExtendedFrame()) receivedID=(receivedID>>18);
		if (receivedID==victimID) {
			received=true;
			break;
		}
    }
  }
}

inline void enableOneShot(){
  CAN.mcp2515_modifyRegister(MCP_CANCTRL, MODE_ONESHOT, MODE_ONESHOT);
}

inline void disableOneShot(){
  CAN.mcp2515_modifyRegister(MCP_CANCTRL, MODE_ONESHOT, 0);
}


inline byte sendInAnyBuf(unsigned long id, unsigned char * buf, byte ext, int len=msgLen){
  return CAN.sendMsgBuf(id, ext, len, buf, false);
}

inline byte sendInParticularBuf(unsigned long id, unsigned char * buf, byte bufferIdx, int len=msgLen){
  return CAN.trySendMsgBuf(id, 0, 0, len, buf, bufferIdx);
}

ISR(TIMER1_OVF_vect){  
  overflows++;  
}

inline void setWakeupTimer(uint64_t duration, unsigned long *nextWakeupAtOverflows, unsigned int *nextWakeupAtCounter, bool current = true){
  uint64_t currTime;
  if (current) currTime = getElapsedTime();
  else {
    uint64_t currOverflows = (*nextWakeupAtOverflows);
    currTime = currOverflows*overflowTime + (*nextWakeupAtCounter)/freq;
  }
  uint64_t nextTime = currTime + duration; //us
  *nextWakeupAtOverflows = ((uint64_t)nextTime)/overflowTime;
  *nextWakeupAtCounter = (((uint64_t)(nextTime % overflowTime)) * freq);
}

inline bool shouldWakeup(unsigned long targetOverflows, unsigned int targetCounter){
  unsigned long currOverflows = overflows; 
  unsigned int currTimer = getCounter(); 
  return ((targetOverflows == currOverflows && targetCounter<currTimer) || (targetOverflows < currOverflows));
}

inline void resetCANController(int listenIDs[], int len){
  CAN.mcp2515_fast_reset();
  CAN.mcp2515_configRate(CAN_500KBPS, MCP_16MHz);
  CAN.mcp2515_modifyRegister(MCP_RXB0CTRL,
                             MCP_RXB_RX_MASK | MCP_RXB_BUKT_MASK,
                             MCP_RXB_RX_STDEXT | MCP_RXB_BUKT_MASK);
  CAN.mcp2515_modifyRegister(MCP_RXB1CTRL, MCP_RXB_RX_MASK,
                             MCP_RXB_RX_STDEXT);
  CAN.mcp2515_modifyRegister(MCP_TXB0CTRL,3,3);
  CAN.mcp2515_modifyRegister(MCP_TXB1CTRL,3,2);
  CAN.mcp2515_modifyRegister(MCP_TXB2CTRL,3,1);
  CAN.mcp2515_write_id(MCP_RXM0SIDH, 0, 0x3ff);
  CAN.mcp2515_write_id(MCP_RXM1SIDH, 0, 0x3ff);
  for (int i=0; i<6; i++){
    int idx = i%len;
    int registerAddr = 0;
    switch(i){
      case 0: registerAddr=MCP_RXF0SIDH; break;
      case 1: registerAddr=MCP_RXF1SIDH; break;
      case 2: registerAddr=MCP_RXF2SIDH; break;
      case 3: registerAddr=MCP_RXF3SIDH; break;
      case 4: registerAddr=MCP_RXF4SIDH; break;
      case 5: registerAddr=MCP_RXF5SIDH; break;
    }
    CAN.mcp2515_write_id(registerAddr, 0, listenIDs[idx]);
  }
  CAN.mcp2515_setCANCTRL_Mode(MODE_NORMAL);
}

inline bool noTXPending(int bufsToCheck[], int len){
  bool noTx = true;
  for (int bufIdx=0; bufIdx<len; bufIdx++){
    byte txreq = CAN.mcp2515_readRegister(txCtrlReg(bufsToCheck[bufIdx])) & 0x08;
    if (txreq){
      noTx = false; break;
    }
  }
  return noTx;
}

inline void Serialprint(uint64_t value)
{
  if ( value >= 10 ) Serialprint(value / 10);
  Serial.print((int)(value % 10));
}
