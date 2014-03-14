#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_TSL2561_U.h>
#include <OneWire.h>
#include <avr\sleep.h>
#include <avr\delay.h>
#include <LowPower.h>
#include <RFM69.h>  //install this library in your Arduino library directory from https://github.com/LowPowerLab/RFM69
#include <SPI.h>

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/SITUATION!
//*****************************************************************************************************************************
#define GATEWAYID            1
#define NODEID               9
#define NETWORKID            100

#define DHTPIN               5 
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define SENDINTERVAL         5000
#define ACK_TIME             50
#define FREQUENCY            RF69_915MHZ
#define ENCRYPTKEY          "01234567890abcdef" 
#define IS_RFM69HW
#define LED                  9
#define TEMPSENSOR1          3 
#define SERIAL_BAUD          115200
#define SENSORREADPERIOD     SLEEP_250MS
//*****************************************************************************************************************************

RFM69 radio;
char input;
boolean REQUEST_STATUS;

typedef struct {                
    int           nodeId; //store this nodeId
    unsigned long uptime;
    float  temperature;
    float  humidity;
    float  pressure;
    float  light;
} Weather;
Weather data;

DHT dht(DHTPIN, DHTTYPE);
OneWire ds(TEMPSENSOR1);  // on digital pin 3
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(1308);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
unsigned long lastSend = 0;
unsigned long now = 0;
void setup(void) {
  Serial.begin(SERIAL_BAUD);
  dht.begin();
  /* Initialise the sensor */
  if(!tsl.begin()) {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  lastSend = 0;
  /* Setup the sensor gain and integration time */
  configureSensor();
  /* Initialise the sensor */
  if(!bmp.begin()) {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower(); //must include only for RFM69HW!
  #endif
    
  radio.encrypt(ENCRYPTKEY);
  radio.sleep();
  char buff[50];
  sprintf(buff, "WeatherMote : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}


void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoGain(true);          /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
}

float getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
  
}

void loop() {
  Serial.println(now - lastSend);
  if (now - lastSend > SENDINTERVAL) {
    sensors_event_t event;
    bmp.getEvent(&event);
    if (event.pressure) {
      data.pressure = event.pressure;
    }
    tsl.getEvent(&event);
    if (event.light) {
      data.light = event.light;
    }
    
    float h = dht.readHumidity();
    data.humidity = h;
    float temperature = getTemp();
    data.uptime = millis(); // number of milliseconds since last power off
    data.nodeId = NODEID; // node id
    data.temperature = (temperature * 1.8)+ 32.00;

    radio.send(GATEWAYID, (const void*)(&data), sizeof(data));
    lastSend = now;
  }
  now = now + 250;
  radio.sleep();
  LowPower.powerDown(SENSORREADPERIOD, ADC_OFF, BOD_ON);
}
