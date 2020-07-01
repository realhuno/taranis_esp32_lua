<h1>FRSKY Wifi Serial/Sport Bridge</h1>

S.Port bridge should work with all Radios with S.Port in the Modul bay or outside. <br>

Serial:
Only working with UARTS and OPENTX 2.3.9 and the serialport LUA is activated<br><br>
working trackers: <b>chorus32 and RotorHazard</b><br>

<h1>Config</h1>
change ssid, pw and ip´s (mqttserver/tcp server)<br>
#define WIFI_AP_NAME1 "Chorus32 LapTimer"      //Dont´t change... reserved for Chorus32 connection<br>
#define WIFI_AP_NAME2 "Laptimer"               //my laptimer ssid on the field<br>
#define WIFI_AP_NAME3 "A1-7FB051"              //my dev ssid<br><br>

#define WIFI_AP_PASSWORD1 ""                   //Chorus32 Laptimer password<br>
#define WIFI_AP_PASSWORD2 "laptimer"           //you know ... your password<br>
#define WIFI_AP_PASSWORD3 "blablabla"          //you know ... your password<br><br>

#define MQTTSERVER1 "192.168.4.1"           //MQTT or TCP Client for Chorus32 Port 9000<br>
#define MQTTSERVER2 "192.168.0.141"         //field mqtt server<br>
#define MQTTSERVER3 "10.0.0.50"             //home mqtt server<br><br>

copy sounds in the zip file to /SCRIPTS/SOUNDS/LapTmr/<br><br>
copy lua´s to /SCRIPTS/TELEMETRY/<br>
TCP Connection with Chrorus32 on Port 9000
MQTT Connection with Rotorhazard
TODO Change IP´s and SSID ... Cleanup 
<h1>Serial method</h1>
use chorus.lua<br>
taranis_esp32_lua.ino<br><br>
change ip´s ssid´s in the CODE!
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/schematic.png width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/laplist.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/version.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/hardware.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/lua.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/webui.PNG width=50% height=50%>
<br>
<h1>Sport method</h1>
Use laptmr.lua<br>
taranis_esp32_lua_sport.ino<br>
setup config and wire up the esp32<br>
add logical switch ls1 see photos..<br>(if you using the lua script) 
discovery telemetry sensors!! 1AEB appers on the lcd screen! this is always the laptime in seconds<br>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/sport.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/laps.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/sporthardware.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/logical.jpg width=50% height=50%>
<img src=https://github.com/realhuno/taranis_esp32_lua/blob/master/logical2.jpg width=50% height=50%>


