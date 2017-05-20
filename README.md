# aws_mqtt_arduino_uno
AWS IOT MQTT Bridge Client example for Arduino Uno / Mega with Ethernet Shield

Arduino Example connection to [AWS IOT Mosquitto Broker](https://github.com/dnavarrom/aws_mosquitto_broker)

Tested on:

- Arduino UNO + Etheret Shield
- Arduino MEGA 2560 Clone + Ethernet Shield

## Instructions

1.- Install libraries using [this guide](https://www.arduino.cc/en/guide/libraries)

Library 1: [PubSubClient](https://github.com/knolleary/pubsubclient)
Library 2: [ArduinoJson](https://bblanchon.github.io/ArduinoJson/)
Library 3: [SoftReset](https://github.com/WickedDevice/SoftReset)


2.- Check if Docker AWS Bridge is up and running

`mosquitto_pub -h localhost -p 1883 -q 1 -d -t localgateway_to_awsiot  -i clientid1 -m "{\"key\": \"helloFromLocalGateway\"}"`
Setup intructions [here](https://github.com/dnavarrom/aws_mosquitto_broker)

3.- Setup arduino sketch

`/************************* MQTT Client Config ********************************/ `

`const char* mqttserver = "192.168.0.14";               // Local Broker`
`const int mqttport = 1883;                             // MQTT port`
`String subscriptionTopic = "awsiot_to_localgateway";   // Get messages from AWS`
`String publishTopic = "localgateway_to_awsiot";        // Send messages to AWS `

4.- RUN

You should see mesages in your AWS IOT Console, Docker Logs and Arduino Serial Console

