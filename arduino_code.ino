// Thingspeak  
String statusChWriteKey = "LU0DPGBUGKYV5RDL";  // Status Channel id: 385184

String canalID1 = "1005475"; // Enter your Actuator1 Channel ID here
String canalID2 = "1005476"; // Enter your Actuator1 Channel ID here

#include <SoftwareSerial.h>
SoftwareSerial EspSerial(6, 7); // Rx,  Tx

// HW Pins
#define FREEZE_LED 13
#define HARDWARE_RESET 8

// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 5 // DS18B20 on pin D5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int soilTemp = 0;

//DHT
#include "DHT.h"
#include <stdlib.h>
int pinoDHT = 11;
int tipoDHT =  DHT22;
DHT dht(pinoDHT, tipoDHT); 
int airTemp = 0;
int airHum = 0;

// LDR (Light)
#define ldrPIN 1
int light = 0;

// Soil humidity
#define soilHumPIN 0
int soilHum = 0;

// LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(9, 4, 3, 2, A4, A5);

// Variables to be used with timers
long writeTimingSeconds = 15-6; // ==> Define Sample time in seconds to send data
long readTimingSeconds = 10; // ==> Define Sample time in seconds to receive data
long startReadTiming = 0;
long elapsedReadTime = 0;
long startWriteTiming = 0;
long elapsedWriteTime = 0;

//Relays
#define ACTUATOR1 10 // RED LED   ==> Pump
#define ACTUATOR2 12 // GREEN LED ==> Lamp
boolean pump = 0; 
boolean lamp = 0; 

int command = 0;
int spare = 0;
boolean error;

void setup()
{
  Serial.begin(9600);
  
  lcd.begin(16, 2);
  
  pinMode(ACTUATOR1,OUTPUT);
  pinMode(ACTUATOR2,OUTPUT);
  pinMode(FREEZE_LED,OUTPUT);
  pinMode(HARDWARE_RESET,OUTPUT);

  digitalWrite(ACTUATOR1, HIGH); 
  digitalWrite(ACTUATOR2, HIGH); 
  digitalWrite(FREEZE_LED, LOW);
  digitalWrite(HARDWARE_RESET, HIGH);
  
  DS18B20.begin();
  dht.begin();

  EspSerial.begin(9600); 
  EspHardwareReset(); //Reset
  startReadTiming = millis(); // starting the "program clock"
  startWriteTiming = millis(); // starting the "program clock"
}

void loop()
{
  start: //label 
  error=0;
  
  
  elapsedWriteTime = millis()-startWriteTiming; 
  //elapsedReadTime = millis()-startReadTiming; 

  //ESPcheck();
  command = readThingSpeak(canalID1); 
  if (command != 9) pump = command; 
  takeActions();
  command = readThingSpeak(canalID2); 
  if (command != 9) lamp = command; 
  takeActions();
  
  //startReadTiming = millis();   
  
  if (elapsedWriteTime > (writeTimingSeconds*1024)) 
  {
    //ESPcheck();
    readSensors();
    writeLCD();
    Serial.println("New data!");
    writeThingSpeak();
    startWriteTiming = millis();   
  }
  
  if (error==1) //Resend if transmission is not completed 
  {       
    Serial.println(" <<<< ERROR >>>>");
    digitalWrite(FREEZE_LED, HIGH);
    goto start; //go to label "start"
  }
}

/********* Read Sensors value *************/
void readSensors(void)
{
  airTemp = dht.readTemperature();
  airHum = dht.readHumidity();

  DS18B20.requestTemperatures(); 
  soilTemp = DS18B20.getTempCByIndex(0); // Sensor 0 will capture Soil Temp in Celcius
             
  light = map(analogRead(ldrPIN), 1023, 0, 0, 100); //LDRDark:0  ==> light 100%  
  soilHum = map(analogRead(soilHumPIN), 1023, 0, 0, 100); 

}

void writeLCD(void)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AT:" + String(airTemp) + "C");
  lcd.setCursor(9, 0);   
  lcd.print("WT:" + String(soilTemp) + "C");
  lcd.setCursor(0, 1);
  lcd.print("Hum:" + String(airHum) + "%");
  lcd.setCursor(9, 1);
  lcd.print("Br:" + String(light) + "%");
}

/********* Take actions based on ThingSpeak Commands *************/
void takeActions(void)
{
  Serial.print("Pump: ");
  Serial.println(pump);
  Serial.print("Lamp: ");
  Serial.println(lamp);
  if (pump == 1) digitalWrite(ACTUATOR1, LOW);
  else digitalWrite(ACTUATOR1, HIGH);
  if (lamp == 1) digitalWrite(ACTUATOR2, LOW);
  else digitalWrite(ACTUATOR2, HIGH);
}

/********* Read Actuators command from ThingSpeak *************/
int readThingSpeak(String channelID)
{
  startThingSpeakCmd();
  int command;
  String getStr = "GET /channels/";
  getStr += channelID;
  getStr +="/fields/1/last";
  getStr += "\r\n";

  String messageDown = sendThingSpeakGetCmd(getStr);
  if (messageDown[5] == 49)
  {
    command = messageDown[7]-48; 
    Serial.print("Command received: ");
    Serial.println(command);
  }
  else command = 9;
  return command;
}

void writeThingSpeak(void)
{

  startThingSpeakCmd();

  String getStr = "GET /update?api_key=";
  getStr += statusChWriteKey;
  getStr +="&field1=";
  getStr += String(pump);
  getStr +="&field2=";
  getStr += String(lamp);
  getStr +="&field3=";
  getStr += String(airTemp);
  getStr +="&field4=";
  getStr += String(airHum);
  getStr +="&field5=";
  getStr += String(soilTemp);
  getStr +="&field6=";
  getStr += String(soilHum);
  getStr +="&field7=";
  getStr += String(light);
  getStr +="&field8=";
  getStr += String(spare);
  getStr += "\r\n\r\n";

  sendThingSpeakGetCmd(getStr); 
}

/********* Echo Command *************/
boolean echoFind(String keyword)
{
 byte current_char = 0;
 byte keyword_length = keyword.length();
 long deadline = millis() + 5000; 
 while(millis() < deadline){
  if (EspSerial.available()){
    char ch = EspSerial.read();
    Serial.write(ch);
    if (ch == keyword[current_char])
      if (++current_char == keyword_length){
       Serial.println();
       return true;
    }
   }
  }
 return false;
}

/********* Reset ESP *************/
void EspHardwareReset(void)
{
  Serial.println("Reseting......."); 
  digitalWrite(HARDWARE_RESET, LOW); 
  delay(500);
  digitalWrite(HARDWARE_RESET, HIGH);
  delay(500);
  Serial.println("RESET"); 
}

/********* Check ESP *************/
boolean ESPcheck(void)
{
  EspSerial.println("AT"); // Send "AT+" command to module
   
  if (echoFind("OK")) 
  {
    //Serial.println("ESP ok");
    digitalWrite(FREEZE_LED, LOW);
    return true; 
  }
  else //Freeze ou Busy
  {
    Serial.println("ESP Freeze *********************************************************************");
    digitalWrite(FREEZE_LED, HIGH);
    EspHardwareReset();
    return false;  
  }
}

/********* Start communication with ThingSpeak*************/
void startThingSpeakCmd(void)
{
  EspSerial.flush();
  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149";
  cmd += "\",80";
  EspSerial.println(cmd);
  Serial.print("enviado ==> Start cmd: ");
  Serial.println(cmd);

  if(EspSerial.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}

/********* send a GET cmd to ThingSpeak *************/
String sendThingSpeakGetCmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  EspSerial.println(cmd);
  Serial.print("enviado ==> lenght cmd: ");
  Serial.println(cmd);

  if(EspSerial.find((char *)">"))
  {
    EspSerial.print(getStr);
    Serial.print("enviado ==> getStr: ");
    Serial.println(getStr);
    delay(500);

    String messageBody = "";
    while (EspSerial.available()) 
    {
      String line = EspSerial.readStringUntil('\n');
      if (line.length() == 1) 
      { //actual content starts after empty line (that has length 1)
        messageBody = EspSerial.readStringUntil('\n');
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    EspSerial.println("AT+CIPCLOSE");     // alert user
    Serial.println("ESP8266 CIPSEND ERROR: RESENDING"); //Resend...
    spare = spare + 1;
    error=1;
    return "error";
  } 
}
