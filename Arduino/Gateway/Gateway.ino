// **********************************************************************************************************
// GarageMote garage door controller base receiver sketch that works with Moteinos equipped with HopeRF RFM69W/RFM69HW
// Can be adapted to use Moteinos using RFM12B
// This is the sketch for the base, not the controller itself, and meant as another example on how to use a
// Moteino as a gateway/base/receiver
// 2013-09-13 (C) felix@lowpowerlab.com, http://www.LowPowerLab.com
// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// **********************************************************************************************************

#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/SITUATION!
//*****************************************************************************************************************************
#define NODEID        1
#define GARAGENODEID  99
#define NETWORKID     100
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "0123456789abcdf" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW  //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define LED           9
#define SERIAL_BAUD   115200
#define ACK_TIME      30  // # of ms to wait for an ack
//*****************************************************************************************************************************
typedef struct {                
    int           nodeId; //store this nodeId
    //char *          name;
    unsigned long uptime;
    boolean       washerStatus;
    boolean       dryerStatus;
    boolean       washerAlert;
    boolean       dryerAlert;
} Payload;

typedef struct {                
    int           nodeId; //store this nodeId
    unsigned long uptime;
    float  temperature;
    float  humidity;
    float  pressure;
    float  light;
} Weather;


RFM69 radio;
SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
byte readSerialLine(char* input, char endOfLineChar=10, byte maxLength=64, uint16_t timeout=50);

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower(); //must include only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
}
unsigned long weatherUpdate = 0;
byte ackCount=0;
byte inputLen=0;
char input[64];
void loop() {
  //delay(5000);
  //process any serial input
  //Serial.print("Something");
  inputLen = readSerialLine(input);
  //Serial.print(inputLen);
  if (inputLen >= 6)
  {
    if (input[0]=='G' && input[1]=='R' && input[2]=='G' && input[3]=='O' && input[4]=='P' && input[5]=='N')
    {
      Serial.print("OPN ... ");
      if (radio.sendWithRetry(GARAGENODEID, "OPN", 3))
        Serial.println("ok ... ");
      else Serial.println("nothing ... ");
    }
    if (input[0]=='G' && input[1]=='R' && input[2]=='G' && input[3]=='C' && input[4]=='L' && input[5]=='S')
    {
      Serial.print("CLS ... ");
      if (radio.sendWithRetry(GARAGENODEID, "CLS", 3))
        Serial.println("ok ... ");
      else Serial.println("nothing ... ");
    }
    if (input[0]=='G' && input[1]=='R' && input[2]=='G' && input[3]=='S' && input[4]=='T' && input[5]=='S')
    {
      Serial.print("STS ... ");
      
      if (radio.sendWithRetry(7, "STS", 3)) {
        Serial.println("ok...");
      } else {
        //Serial.println("nothing...");
      }
      delay(50);
      if (radio.sendWithRetry(GARAGENODEID, "STS", 3)) {
        Serial.println("ok ... ");
      } else {
        //Serial.println("nothing ... ");
      }
    }

    //if (input == 'i')
    //{
    //  Serial.print("DeviceID: ");
    //  word jedecid = flash.readDeviceId();
    //  Serial.println(jedecid, HEX);
    //}
  }

  if (radio.receiveDone())
  {
    if (radio.SENDERID == 7) {
      if (radio.DATALEN != sizeof(Payload)) {
        Serial.print("Invalid payload received, not matching Payload struct!");
      } else {
        Payload theData = *(Payload*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
          Serial.print("{ \"nodeId\" : ");
          Serial.print(theData.nodeId);
          Serial.print(", \"uptime\" : ");
          Serial.print(theData.uptime);
          Serial.print(", \"washerStatus\" : ");
          Serial.print(theData.washerStatus);
          Serial.print(", \"dryerStatus\" : ");
          Serial.print(theData.dryerStatus);
          Serial.print(", \"washerAlert\" : ");
          Serial.print(theData.washerAlert);
          Serial.print(", \"dryerAlert\" : ");
          Serial.print(theData.dryerAlert);
          Serial.print(" }\n");
    }
      
    } else if (radio.SENDERID == 9) {
      if (radio.DATALEN != sizeof(Weather)) {
        Serial.print("Invalid payload received, not matching Payload struct!");
      } else {
        Weather temp = *(Weather*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
          Serial.print("{ \"nodeId\" : ");
          Serial.print(temp.nodeId);
          Serial.print(", \"uptime\" : ");
          Serial.print(temp.uptime);
          Serial.print(", \"tempreture\" : ");
          Serial.print(temp.temperature);
          Serial.print(", \"humidity\" : ");
          Serial.print(temp.humidity);
          Serial.print(", \"pressure\" : ");
          Serial.print(temp.pressure);
          Serial.print(", \"light\" : ");
          Serial.print(temp.light);
          Serial.print(" }\n");
    }
      
    } else {
      Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
      for (byte i = 0; i < radio.DATALEN; i++)
        Serial.print((char)radio.DATA[i]);
      Serial.print("   [RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    }
    
    if (radio.ACK_REQUESTED)
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
      Serial.print("[ACK-sent]");
    }
    Serial.println();
    Blink(LED,3);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

// reads a line feed (\n) terminated line from the serial stream
// returns # of bytes read, up to 255
// timeout in ms, will timeout and return after so long
byte readSerialLine(char* input, char endOfLineChar, byte maxLength, uint16_t timeout)
{
  byte inputLen = 0;
  Serial.setTimeout(timeout);
  inputLen = Serial.readBytesUntil(endOfLineChar, input, maxLength);
  input[inputLen]=0;//null-terminate it
  Serial.setTimeout(0);
  //Serial.println();
  return inputLen;
}
