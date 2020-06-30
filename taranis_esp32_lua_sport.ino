#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <EEPROM.h>
//OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <stdio.h> //you must include this library to get the atoi function
#include <stdlib.h>
//WEB
#include <WebServer.h>
#include <HTTPClient.h>
#include "index.h"  //Web page header file
#include <SPort.h>                                    //Include the SPort library 
//MQTT
#include <PubSubClient.h>

//--------------CONFIGURATION
// Dont´t forget to discover Telemetry Sensors on the RADIO!!
// 
//WIFIMULTI Handle the WIFI Stuff....!! 
//MQTT and Chorus Client use same server IP


//Sport Config
SPortHub hub(0x12, 4);                                //SPORT PIN!!!!!!!!!! 4!!!!!
SimpleSPortSensor sensor(0x1AEB);                     //Sensor with ID 
SimpleSPortSensor sensor2(0x1AEF);                    //Sensor with ID
int counter=0;                                        //Just Counter
int laptime=2000;                                     //INIT Laptime
int ip4sport=0;
WebServer server(80);


#define WIFI_AP_NAME1 "Chorus32 LapTimer"      //Dont´t change... reserved for Chorus32 connection
#define WIFI_AP_NAME2 "Laptimer"               //my laptimer ssid on the field
#define WIFI_AP_NAME3 "A1-7FB051"              //my dev ssid

#define WIFI_AP_PASSWORD1 ""                   //Chorus32 Laptimer password
#define WIFI_AP_PASSWORD2 "laptimer"           //you know ... your password
#define WIFI_AP_PASSWORD3 "hainz2015"          //you know ... your password

#define MQTTSERVER1 "192.168.4.1"           //MQTT or TCP Client for Chorus32 Port 9000
#define MQTTSERVER2 "192.168.0.141"
#define MQTTSERVER3 "10.0.0.50"

#define UART_TX 15
#define UART_RX 14    //4 normal esp32         14 esp32-cam

#define PROXY_PREFIX 'P' // used to configure any proxy (i.e. this device ;))
#define PROXY_CONNECTION_STATUS 'c'
#define PROXY_WIFI_STATUS 'w'
#define PROXY_WIFI_RSSI 't'
#define PROXY_WIFI_IP 'i'

#define EEPROM_SIZE 32
#define delaytime 70
#define MAX_BUF 1500
char buf[MAX_BUF];
uint32_t buf_pos = 0;
String global;
int mqtt=0;
long lastReconnectAttempt = 0;
String mqttserver;

String ipa="10.0.0.50";                //old stuff delete?
int mqttid=2; //Change THIS .... MQTTID!! //old stuff delete?

//TEST
  
//#include <SPort.h>                  //Include the SPort library
//SPortHub hub(0x12, 0);              //Hardware ID 0x12, Software serial pin 3
//SimpleSPortSensor sensor(0x5900);   //Sensor with ID 0x5900

WiFiClient client;
WiFiMulti wifiMulti;


WiFiClient espClient;
PubSubClient client2(espClient);

int real_rssi=0;

//Rapidfire Station
String serverName = "http://radio0/setLED?LEDstate";


//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}
 
void handleADC() {
 
 //String adcValue = String(WiFi.localIP());
 
 server.send(200, "text/plane", global); //Send ADC value only to client ajax request
}
 
void callback(char* topic, byte* message, unsigned int length) {             //MQTT Callback

  //Serial.print("Message arrived on topic: ");
  //Serial.print(topic);
  //Serial.print(". Message: ");
 String line;
 String rxvid=WiFi.macAddress();
 String topicmsg;
 int bandn;
 rxvid.replace(":", ""); //remove : from mac

  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    line += (char)message[i];   
  }
  global=global+line+"<br>";
  //Serial.println("Topic:");

   //Serial.println(String(topic));
   //Serial.println(line);
  
     //Rotorhazard send \n first 
     //   ->                    29UML1:Callsign 2 L1: 0:06.451%
     line.trim();
     //SWITCH CHANNEL
     if(line.charAt(0) == '0' && line.charAt(1) == '9' && line.charAt(2) == 'B' && line.charAt(3) == 'C') {   
     //channel(line.charAt(4)+1);
     global=global+"CHANNEL: "+line.charAt(4)+"<br>";
     }
     //SWITCH BAND
     if(line.charAt(0) == '0' && line.charAt(1) == '9' && line.charAt(2) == 'B' && line.charAt(3) == 'G') { 
     bandn=line.charAt(4);   
     if(bandn==2){bandn=2;} //raceband
     if(bandn==4){bandn=1;} //fatshark
     if(bandn==3){bandn=3;} //e
     if(bandn==5){bandn=6;} //low
     if(bandn==1){bandn=4;} //b
     if(bandn==0){bandn=5;} //a
    
     //band(bandn);
     global=global+"BAND: "+line.charAt(4)+"<br>";
     }
     //09BC7%    09B=command change freq..  c=raceband channel 7   
     //Change RotorHazard vrx id rx/cv1/cmd_esp_target/CV_246F28166140
     
     if(line.indexOf('node_number')){                      //<<command 
     mqttid=line.substring(12,13).toInt();
     if(mqttid>0){
      EEPROM.write(0, mqttid);
      EEPROM.commit();
     //String stringnr="5";
      client2.unsubscribe("rx/cv1/cmd_node/0");
      client2.unsubscribe("rx/cv1/cmd_node/1");
      client2.unsubscribe("rx/cv1/cmd_node/2");
      client2.unsubscribe("rx/cv1/cmd_node/3");
      client2.unsubscribe("rx/cv1/cmd_node/4");
      client2.unsubscribe("rx/cv1/cmd_node/5");
      client2.unsubscribe("rx/cv1/cmd_node/6");
      client2.unsubscribe("rx/cv1/cmd_node/7");
      client2.unsubscribe("rx/cv1/cmd_node/8");
      topicmsg = "rx/cv1/cmd_node/"+line.substring(12,13);                                            
                             
      client2.subscribe(topicmsg.c_str());
     global=global+topicmsg+"<br>"; 
     char JSONmessageBuffer[100];
     StaticJsonBuffer<300> JSONbuffer;
     JsonObject& JSONencoder = JSONbuffer.createObject();
     //JSONencoder["node_number"] = String(6);
     JSONencoder["node_number"] = String(line.substring(12,13));

     JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
     
     
     topicmsg="status_variable/CV_"+rxvid;
     client2.publish(topicmsg.c_str(), JSONmessageBuffer);
     
     global=global+"CHANGE TO"+String(mqttid)+"<br>";
     global=global+"CHANGE NODE_NUMBER"+"<br>";
     //mqttid=current_nr;
     //reconnect();
     }
     }
      
   
                           //rx/cv1/cmd_esp_target/CV_00000001  

     if(line.charAt(0) == '0' && line.charAt(1) == '9' && line.charAt(2) == 'U') {             //09UMFinish%
     //Serial.println("Send text mqtt");
     String stringOne;
     stringOne="T="+line.substring(4,line.lastIndexOf('%')+3); //8
     //stringOne="T="+line; //8
     int len = strlen(stringOne.c_str());
     String fill;
     int i;
     for (int i = len; i < 24; i++){
     fill=fill+" ";
     }
     stringOne=stringOne+fill;
     stringOne.replace("%", " "); //replace % with space
     //text(stringOne);            //not working
     global=global+stringOne+"<br>";
     }
     
     if(line.charAt(0) == '2' || line.charAt(0) == '0' && line.charAt(1) == '9' && line.charAt(2) == 'U') {           //Push in laptimes
     //Serial.println("Send text mqtt");
     String stringOne;

     stringOne="T="+line.substring(5,line.lastIndexOf('%')+3); //8
     int len = strlen(stringOne.c_str());
     //fill with spaces
     String fill;
     int i;
     for (int i = len; i < 24; i++){
     fill=fill+" ";
     }
     stringOne=stringOne+fill;
     stringOne.replace("%", " "); //replace % with space

     stringOne=stringOne.substring(0,line.lastIndexOf('/')); //extract only my time not others
     /*
     int firsttime = stringOne.indexOf(':');
     stringOne=stringOne.substring(firsttime+1,stringOne.length());
     firsttime = stringOne.indexOf(':');
     stringOne=stringOne.substring(firsttime+1,stringOne.length());
     firsttime = stringOne.indexOf(':');
     int lasttime = stringOne.lastIndexOf(':');
     */
     int lappos=stringOne.lastIndexOf('L'); 
     int lap=String(stringOne.charAt(lappos+1),DEC).toInt(); 
     int lasttime = stringOne.lastIndexOf(':');              //found time position
     stringOne=stringOne.substring(lasttime-1,lasttime+7);
     global=global+stringOne; //-1=0:233                      //full time string in 0:00.000
     global=global+"<br>";
     
     
     int minutechar=String(stringOne.charAt(0),DEC).toInt();                     //esctract only minutes char
     int minute=minutechar-48;                              //esctract only minutes int
     stringOne=stringOne.substring(1,stringOne.length());     //extract only seconds
     stringOne.replace(":","");                               //remove :
     stringOne.replace(".","");                               //remove .
     int seconds=stringOne.toInt();                           //convert to int
     int sum=seconds+(60000*minute);                              //calculate minute and second
    
     global=global+"lap"+lap+"<br>";
     global=global+"minute"+minute+"<br>";
     global=global+"seconds"+seconds+"<br>";
     global=global+"summery"+sum+"<br>";
     global=global+"<br>";
     laptime=sum;
     if(sum>0 && lap!=97 && lap!=84){                                                //Only send positiv values
     String stringtwo =  String(sum, HEX); 
     lap=lap-48;
    
     
     String stringlap = String(lap, DEC); 
     Serial.println("S1L0"+stringlap+"""000"+stringtwo);
     global=global+"S1L0"+stringlap+"""000"+stringtwo+"<br>";
     }
     }
     
     if(line.charAt(0) == 'T') {  //Normal mqtt osd text working!!
     String t_state=line;
     t_state.trim();
     int len = strlen(t_state.c_str());
     String fill;
     int i;
     for (int i = len; i < 26; i++){
     fill=fill+" ";
     }
     t_state=t_state+fill;
     t_state.replace("%", " "); //replace % with space
     //text(t_state);            //not working 


      
     //text(line);
     }
     
     //LED Light
     if(line.charAt(0) == 'L') {
     if(line.charAt(2)=='0'){
     pinMode(4,OUTPUT);
     digitalWrite(4,LOW);
     }
     if(line.charAt(2)=='1'){
     pinMode(4,OUTPUT);
     digitalWrite(4,HIGH);
     }
     }

     //Rapidfire CMD
  if(line.charAt(0) == 'P') {
  String stringOne;
  stringOne=line.substring(2,line.length()); 
  Serial.println(stringOne);
  global=global+"Forwarding Taranis"+stringOne+"<br>";
  }

   //Rapidfire CMD SPORT
  if(line.charAt(0) == 'X') {
  String stringOne;
  stringOne=line.substring(2,line.length()); 
  laptime=stringOne.toInt();
  global=global+"Forwarding Taranis"+stringOne+"<br>";
  }

  if(line.charAt(0) == 'S') {
  //buzzer();
  //Serial.println("BUZZER");
  global=global+"MQTTBUZZER"+"<br>";
  }
  if(line.charAt(0) == 'O') {
  //osdmode(line.charAt(2));
  //Serial.println("OSD");

  }
  if(line.charAt(0) == 'C') {
  //channel(line.charAt(2));
  //Serial.println("Channel");

  }     
  if(line.charAt(0) == 'B') {
  //band(line.charAt(2));
  //Serial.println("Band");
  } 
     


}

boolean reconnect() {                                       //MQTT Connect and Reconnect
  // Loop until we're reconnected
  mqttid=EEPROM.read(0);
 mqtt=1;
 ipa=server.arg("ip");
 String rxvid=WiFi.macAddress();
 String topic;
 rxvid.replace(":", ""); //remove : from mac
 global=global+rxvid+"<br>";
   if(ipa.length()>=5){
    //Serial.print("New MQTT connection...");
    mqttserver=ipa;
   }else{

    if(WiFi.SSID()==WIFI_AP_NAME1){
    mqttserver=MQTTSERVER1;
    global=global+"try mqttconnect<br>";
    }


    if(WiFi.SSID()==WIFI_AP_NAME2){
    mqttserver=MQTTSERVER2;
    global=global+"try mqttconnect<br>";
    }

    if(WiFi.SSID()==WIFI_AP_NAME3){
    mqttserver=MQTTSERVER3;
    global=global+"try mqttconnect<br>";

    }

    //Serial.print("MQTT reconnect...");
   }
   
   
  client2.setServer(mqttserver.c_str(), 1883);
  client2.setCallback(callback);
  
    //Serial.print("Attempting MQTT connection...");
    //Serial.println(mqttserver.c_str());
    // Attempt to connect
    topic = "rapidfire_"+rxvid;
    if (client2.connect(topic.c_str())) {
      
      //Serial.println("connected");
      // Subscribe
      //client2.subscribe("esp32/output");
      //client2.subscribe("rx/cv1/cmd_esp_all");
      

      client2.subscribe("rx/cv1/cmd_all");
      topic = "rx/cv1/cmd_esp_target/CV_" + rxvid;
      client2.subscribe(topic.c_str()); //change id new via mac
      //client.subscribe("rx/cv1/cmd_esp_target/CV_00000001"); //change id old
      
      global=global+"MQTT Connected"+"<br>";
      //String topic = "rxcn/CV_" + rxvid;     why two times? bottom....
      //client.publish(topic.c_str(), "1");


StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
 
  JSONencoder["dev"] = "rx";
  JSONencoder["ver"] = "todover";
  JSONencoder["fw"] = "todover";
  JSONencoder["nn"] = "taranis";
  
  //JsonArray& values = JSONencoder.createNestedArray("values");
 
 
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  //Serial.println("Sending message to MQTT topic..");
  //Serial.println(JSONmessageBuffer);
 
 

     JSONencoder["node_number"] = "1";
     topic = "rxcn/CV_" + rxvid;
     client2.publish(topic.c_str(), "1");
     delay(500);
     topic = "status_static/CV_" + rxvid;
     client2.publish(topic.c_str(), JSONmessageBuffer);
  char JSONmessageBuffer2[100];
  JSONencoder.printTo(JSONmessageBuffer2, sizeof(JSONmessageBuffer2));
  //Serial.println("Sending message to MQTT topic..");
  //Serial.println(JSONmessageBuffer2);
    delay(500);
     topic = "status_variable/CV_" + rxvid;
     
     client2.publish(topic.c_str(), JSONmessageBuffer2);
//rx/cv1/cmd_node/1
     
     String topicmsg;
      client2.unsubscribe("rx/cv1/cmd_node/0");
      client2.unsubscribe("rx/cv1/cmd_node/1");
      client2.unsubscribe("rx/cv1/cmd_node/2");
      client2.unsubscribe("rx/cv1/cmd_node/3");
      client2.unsubscribe("rx/cv1/cmd_node/4");
      client2.unsubscribe("rx/cv1/cmd_node/5");
      client2.unsubscribe("rx/cv1/cmd_node/6");
      client2.unsubscribe("rx/cv1/cmd_node/7");
      client2.unsubscribe("rx/cv1/cmd_node/8");
      topicmsg = "rx/cv1/cmd_node/"+String(mqttid);                                            
        global=global+"TOPIC: "+String(topicmsg)+"<br>";
  
                       
      client2.subscribe(topicmsg.c_str());
     JSONencoder["node_number"] = mqttid;  //         mqttid );<<<<<<<<<<<<<<<

     JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
     
     
     topicmsg="status_variable/CV_"+rxvid;
        global=global+"TOPIC: "+String(topicmsg)+"<br>";
  
     client2.publish(topicmsg.c_str(), JSONmessageBuffer);
    } else {
      mqtt=0;
      //Serial.print("failed, rc=");
      //Serial.print(client2.state());
      global=global+"MQTT not Connected"+"<br>";
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(500);
    }
  return client2.connected();  
}

 
void handleLED() {

 String t_state = server.arg("LEDstate"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
 Serial.println(t_state);
 //Serial.println(t_state);

 int timems=server.arg("LEDstate").toInt();
laptime=timems;
 global=global+t_state+"<br>";
 server.send(200, "text/plane", t_state); //Send web page
}

void handleTime() {

 //int t_state = server.arg("time").toInt(); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
 int timems=server.arg("time").toInt();

 


 String stringOne =  String(timems, HEX); 
 
 Serial.println("S1L01000"+stringOne);
laptime=timems;

 //server.send(200, "text/plane", t_state); //Send web page
}

void chorus_connect() {
  if(WiFi.isConnected()) {
        

    if(WiFi.SSID()==WIFI_AP_NAME1){
    if(client.connect(MQTTSERVER1, 9000)) {
      //Serial.println("Connected to chorus via tcp 192.168.4.1");
    }
    }


    if(WiFi.SSID()==WIFI_AP_NAME2){
    if(client.connect(MQTTSERVER2, 9000)) {
      //Serial.println("Connected to chorus via tcp 192.168.0.141");
    }
    }

    if(WiFi.SSID()==WIFI_AP_NAME3){
    if(client.connect(MQTTSERVER3, 9000)) {
    //Serial.println("Connected to chorus via tcp 10.0.0.50");
    }
    }

  }
}
void handleCommand(int prim, int applicationId, int value) {
  hub.sendCommand(0x32, applicationId, value + 1);    //Send a command back to lua, with 0x32 as reply and the value + 1
}



void setup() {
    hub.commandId = 0x1B;                               //Listen to data send to thist physical ID
  hub.commandReceived = handleCommand;                //Function to handle received commands
  hub.registerSensor(sensor);                         //Add sensor to the hub
  hub.registerSensor(sensor2);                         //Add sensor to the hub
  hub.begin();                                        //Start listening
  //hub.registerSensor(sensor);       //Add sensor to the hub
  //hub.begin();                      //Start listening

  EEPROM.begin(EEPROM_SIZE);
  mqttid=EEPROM.read(0);

  Serial.begin(115200);                   //ESP32
    //  Serial.begin(115200, SERIAL_8N1, 3, 1, true);                   //ESP32
  //Serial.begin(115200, SERIAL_8N1, UART_RX, UART_TX, true);   //ESP32-CAM
  //WiFi.begin(WIFI_AP_NAME);
   
  

    wifiMulti.addAP(WIFI_AP_NAME1, WIFI_AP_PASSWORD1);
    wifiMulti.addAP(WIFI_AP_NAME2, WIFI_AP_PASSWORD2);
    wifiMulti.addAP(WIFI_AP_NAME3, WIFI_AP_PASSWORD3);

    //Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        //Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
    MDNS.begin("radio0");
        Serial.println(WiFi.localIP());
        String ip4sportstr;
        ip4sportstr=WiFi.localIP().toString();
        ip4sportstr.trim();
        ip4sportstr.replace(".", ""); //replace % with space
        ip4sport=ip4sportstr.toInt();
    }
  






  delay(500);

 
  server.on("/", handleRoot);      //This is display page
  server.on("/readADC", handleADC);//To get update of ADC Value only
  server.on("/setLED", handleLED);
  server.on("/setTime", handleTime);
  server.on("/reconnect", reconnect);
  server.on("/reconnect2", chorus_connect);
 
  server.begin();                  //Start server
  //Serial.println("HTTP server started");



  ArduinoOTA.begin();
   global=global+"node_number: "+mqttid+"<br>";
 global=global+"Try boot up mqtt connect...<br>";
 
 reconnect();
}



void loop() {
  ArduinoOTA.handle();
  real_rssi=WiFi.RSSI();
  server.handleClient();
  sensor.value = laptime;                                //Set the sensor value
  sensor2.value = ip4sport;                                //Set the sensor value
  
  hub.handle();                                       //Handle new data
  counter++;

  
 //sensor.value = 1234;              //Set the sensor value
 //hub.handle();                     //Handle new data

  delay(1);
    if(wifiMulti.run() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        delay(1000);
    }


  while(client.connected() && client.available()) {
    // Doesn't include the \n itself
    String line = client.readStringUntil('\n');

    // Only forward specific messages for now
    //if(line[2] == 'L') {
    Serial.println(line);
     
      //Serial.printf("Forwarding to taranis: %s\n", line.c_str());
    //}
  }
    IPAddress myip = WiFi.localIP();
            Serial.println(myip);

  while(Serial.available()) {
    if(buf_pos >= MAX_BUF -1 ) {
      buf_pos = 0; // clear buffer when full
      break;
    }
    buf[buf_pos++] = Serial.read();
    
    if(buf[buf_pos - 1] == '\n') {
      global=global+buf[buf_pos - 1];
      // catch proxy command
      if(buf[0] == 'P') {
        switch(buf[3]) {
          case PROXY_WIFI_STATUS:
            Serial.printf("%cS*%c%1x\n", PROXY_PREFIX, PROXY_WIFI_STATUS, WiFi.status());
            break;
                    case PROXY_WIFI_RSSI:
            Serial.printf("%cS*%c%2x\n", PROXY_PREFIX, PROXY_WIFI_RSSI, abs(real_rssi*-1));
            //Serial.println(real_rssi);
            break;
          case PROXY_CONNECTION_STATUS:
            Serial.printf("%cS*%c%1x\n", PROXY_PREFIX, PROXY_CONNECTION_STATUS, client.connected());
            break;
        }
      }
      else if((buf[0] == 'E' && (buf[1] == 'S' || buf[1] == 'R')) || buf[0] == 'S' || buf[0] == 'R') {
        //Serial.print("Forwarding to chorus: ");
        Serial.write((uint8_t*)buf, buf_pos);
        client.write(buf, buf_pos);
        //int a = analogRead(A0);
             
               
      }
      String adcValue = String(buf);
      if(buf[0] == 'C') {
        HTTPClient http;
          
          String serverPath = serverName + "=C=" + (buf[1]-47);
          // Your Domain name with URL path or IP address with path
          http.begin(serverPath.c_str());
           // Send HTTP GET request
           int httpResponseCode = http.GET();

      }
        server.send(200, "text/plane", adcValue); //Send ADC value only to client ajax request
      buf_pos = 0;
    }
  }
    mqtt=1;
if (!client2.connected() && mqtt==1) {
    long now = millis();
    if (now - lastReconnectAttempt > 10000) { // Try to reconnect.
      lastReconnectAttempt = now;
      if (reconnect()) { // Attempt to reconnect.
        lastReconnectAttempt = 0;
      }
    }
  } else{
    mqtt==1; // Connected.
    client2.loop();
    //client.publish(channelName,"Hello world!"); // Publish message.
    //Serial.println(mqtt);
 
  }

    //Try Chorus Connection
    if(WiFi.SSID()=="Chorus32 LapTimer"){
    mqttserver=MQTTSERVER1;
    global=global+"try chorus connect to 192.168.4.1<br>";


    if(!client.connected()) {
    delay(1000);
    Serial.println("Lost tcp connection! Trying to reconnect");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.SSID());
    chorus_connect();
  }

    }








}
