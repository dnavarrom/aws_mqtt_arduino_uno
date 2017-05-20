 /*********************************************************************************
   MQTT Bridge to AWS Example
 
   MQTT Client for MQTT AWS Bridge https://github.com/dnavarrom/aws_mosquitto_broker
 
   Tested on: Arduino Uno / Mega 2560 + Ethernet Shield
 
   Written by Diego Navarro M
   Using PubSubClient library https://github.com/knolleary/pubsubclient
   Using ArduinoJson Library https://bblanchon.github.io/ArduinoJson/
   Using SoftReset Library https://github.com/WickedDevice/SoftReset
   MIT license, all text above must be included in any redistribution
 ***********************************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <SoftReset.h>
#include <PubSubClient.h> 
#include <Wire.h> 
#include <ArduinoJson.h>

//Comment to disable debug output
#define GENERAL_DEBUG

#ifdef GENERAL_DEBUG
#define DEBUG_PRINT(string) (Serial.println(string))
#endif

#ifndef GENERAL_DEBUG
#define DEBUG_PRINT(String)
#endif



/************************* Ethernet Client Setup *****************************/
byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0x1F, 0x48};

//Uncomment the following, and set to a valid ip if you don't have dhcp available.
//IPAddress ip(192, 168, 100, 99);
//If you uncommented either of the above lines, make sure to change "Ethernet.begin(mac)" to "Ethernet.begin(mac, iotIP)" or "Ethernet.begin(mac, iotIP, dnsIP)"

char myIPAddress[20];


/************************* MQTT Client Config ********************************/

const char* mqttserver = "192.168.0.14";               // Local Broker
const int mqttport = 1883;                             // MQTT port
String subscriptionTopic = "awsiot_to_localgateway";   // Get messages from AWS
String publishTopic = "localgateway_to_awsiot";        // Send messages to AWS 


/************************* MQTT Connection Monitoring ************************/

long connecionMonitoringFrequency = 10000UL;  // (1000 = 1 sec)
unsigned long lastConnectionCheck = 0;
int connectionAttempt = 0;
const int maxconnectionAttempt = 5;           // Reset after x connection attempt
const int maxConnectionCycles = 100;//100;    // Security Reset (Soft Reet)
const int maxCiclosDiarios = 5000;//500;      // Full Reset
int ConnectionCyclesCount = 0;
int totalCyclesCount = 0;


/********************* MQTT Broker Callback Function **************************

 * 
 *  Process MQTT Messages (subscription)
 *
 *  Parameters
 *  topic - the topic the message arrived on (const char[])
 *  payload - the message payload (byte array)
 *  length - the length of the message payload (unsigned int)
 */
void callback(char* topic, byte* payload, unsigned int length) {

  /*
  Internally, the client uses the same buffer for both inbound and outbound messages. After the callback function returns, 
  or if a call to either publish or subscribe is made from within the callback function, the topic and payload values passed 
  to the function will be overwritten. The application should create its own copy of the values if they are required beyond this.
  */
  
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  
  char* chararray;
  chararray = (char*)p;

  ProcessPayload(chararray);

  free(p);
}


/************************** PubSubClient Declaration *******************************/


EthernetClient client;
PubSubClient mqttclient(mqttserver,mqttport,callback, client);   //Callback function must be declared before this line


/************************** Sketch Code ********************************************/

void setup() {


  #if defined(GENERAL_DEBUG)
  Serial.begin(9600);
  #endif

  //Configure mqttClient
  mqttclient.setServer(mqttserver, mqttport);
  mqttclient.setCallback(callback);
  
  // start the Ethernet connection:
  Ethernet.begin(mac);

  // Connect to Broker
  if (reconnect()) {
    DEBUG_PRINT("<SETUP> : Connected to broker");
  }
  else {
    DEBUG_PRINT("<SETUP> : Initial Connection Failed");    
  }
  
  DEBUG_PRINT("<SETUP> : End Config..");

}

void loop() {

  //Check connection Status
  if (millis() - lastConnectionCheck > connecionMonitoringFrequency) {
    lastConnectionCheck = millis();
    CheckConnection();
  }
  
  mqttclient.loop();  //end cycle
   
}


/***************************** Payload Processing function ************************/


// Function to process the Broker Payload
// Parameters: 
//  chararray : payload from Callback Function
void ProcessPayload(char* chararray) {

  size_t charSize;
  

  // Extract Json
  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(chararray);
  
  if (!root.success())
  {
    DEBUG_PRINT("<DecodeJson> parseObject() failed");
    return;
  }
  
  for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it)
  {
    DEBUG_PRINT(it->key);
    DEBUG_PRINT(it->value.asString());
  }
  
  
  delay(1000);

  //Publish ACK
  Publicar(publishTopic,"ok", true);

}


//Function to Publish to MQTT Broker
//Parameters
//      topic : ..
//      value  : El valor que se va a publicar en el broker
//      retain:  (UNSUPPORTED) If we want to retain the value

bool Publicar(String topic, String value , bool retain) 
{
  
  bool success = false;
  char cpytopic [50];
  char message [50];
  value.toCharArray(message, value.length() + 1);
  topic.toCharArray(cpytopic,topic.length() + 1);
  success = mqttclient.publish(cpytopic, message);
  
  return success;  
}




/***************************** Broker Connection Functions ************************/


void CheckConnection()
{
  ConnectionCyclesCount++;
  totalCyclesCount++;

  //Restart Ethernet Shield after x connection cycles
  if (ConnectionCyclesCount > maxConnectionCycles)
  {
    DEBUG_PRINT("<CheckConnection> : Restart Ethernet Shield..");
    client.stop();
    Ethernet.begin(mac);
    ConnectionCyclesCount = 0;
  }
  else
  {
    // Daily Softreset
    if (totalCyclesCount > maxCiclosDiarios) {
      DEBUG_PRINT("<CheckConnection> : Reset Device..");
      totalCyclesCount = 0;    
      delay(1000);
      soft_restart();
    }
    else
    {
      //Check MQTT Connection
      if (!mqttclient.connected()) {
        if (!reconnect()) {
          DEBUG_PRINT("<CheckConnection> : Disconnected.. connection attempt #: " + (String)connectionAttempt);
          connectionAttempt++;
          if (connectionAttempt > maxconnectionAttempt)
          {

            connectionAttempt = 0;
            DEBUG_PRINT("<CheckConnection> : Restart Ethernet!!");
            client.stop();
            Ethernet.begin(mac);
            delay(1000);
          }
        }
        else
        {
          connectionAttempt = 0;
          DEBUG_PRINT("<CheckConnection> : Reconnected!!");
        }
      }
      else
      {
          DEBUG_PRINT("<CheckConnection> : Connected!!");
      }
    }
  }
}

// Functon to reconnect to MQTT Broker
boolean reconnect() {
  if (mqttclient.connect("arduinoClient")) {

    char topicConnection [50];
    
    subscriptionTopic.toCharArray(topicConnection,25);
    mqttclient.subscribe(topicConnection);
  }
  
  return mqttclient.connected();
}



/******************************* Generic Helpers *********************************/

//Funcion para convertir la IP en formato string
char* getIpReadable(IPAddress ipAddress)
{
  //Convert the ip address to a readable string
  unsigned char octet[4]  = {
    0, 0, 0, 0    };
  for (int i = 0; i < 4; i++)
  {
    octet[i] = ( ipAddress >> (i * 8) ) & 0xFF;
  }

  sprintf(myIPAddress, "%d.%d.%d.%d\0", octet[0], octet[1], octet[2], octet[3]);

  return myIPAddress;
}

void DecodeJson(char json[]) {
  StaticJsonBuffer<200> jsonBuffer;
  
  JsonObject& root = jsonBuffer.parseObject(json);
  
  if (!root.success())
  {
    DEBUG_PRINT("<DecodeJson> parseObject() failed");
    return;
  }
  
  const char* mensaje    = root["message"];
  long data    = root["valor"];

}






