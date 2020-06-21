const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<div id="demo">
<h1>Laptimer Gateway LUA/OSD</h1>
  <button type="button" onclick="sendData('25000')">Simulate LAP1</button>
  <button type="button" onclick="sendData('29000')">Simulate Lap2</button><BR>
  <button type="button" onclick="sendesp32('R*R1')">R*R1</button>
  <button type="button" onclick="sendesp32('R*R0')">R*R0</button><BR>
     <input type="text" id="ip" value="10.0.0.81">
   <button type="button" onclick="reconnect('true')">MQTT Connect!!</button><br>
        <input type="text" id="ip2" value="10.0.0.81">
   <button type="button" onclick="reconnect2('true')">Chorus TCP Connect!!</button><br>
  <input type="text" id="valuein">
    <button type="button" onclick="sendesp32(valuein.value)">Send</button><BR>
</div>

<div>
  IP : <span id="test">0</span><br>
  IP : <span id="ADCValue">0</span><br>
    Forwarding to Taranis : <span id="LEDState">NA</span>
</div>
<script>

function dec2hex(n){
    n = parseInt(n); var c = 'ABCDEF';
    var b = n / 16; var r = n % 16; b = b-(r/16); 
    b = ((b>=0) && (b<=9)) ? b : c.charAt(b-10);    
    return ((r>=0) && (r<=9)) ? b+''+r : b+''+c.charAt(r-10);
}

function sendData(led) {
 

  led=Number(led).toString(16);
  console.log(led);
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("LEDState").innerHTML =
      this.responseText;
    }
  };

  xhttp.open("GET", "setLED?LEDstate=S1L01000"+led, true);
  xhttp.send();
}

function reconnect(led) {
 

  
  console.log(led);
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("LEDState").innerHTML =
      this.responseText;
    }
  };
let msg = document.querySelector("#ip").value;
  xhttp.open("GET", "reconnect?ip="+msg, true);
  xhttp.send();
}
function reconnect2(led) {
 

  
  console.log(led);
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("LEDState").innerHTML =
      this.responseText;
    }
  };
let msg = document.querySelector("#ip2").value;
  xhttp.open("GET", "reconnect2?ip="+msg, true);
  xhttp.send();
}
function sendesp32(led) {
 

  
  console.log(led);
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("LEDState").innerHTML =
      this.responseText;
    }
  };

  xhttp.open("GET", "setLED?LEDstate="+led, true);
  xhttp.send();
}

setInterval(function() {
  // Call a function repetatively with 2 Second interval
  getData();
}, 200); //2000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ADCValue").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readADC", true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";
