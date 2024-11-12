
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>


#define mqttServer "mqtt_serv" /* определение переменных для mqtt сервера*/
#define mqttPort 9999
#define mqttUser "user"
#define mqttPassword "password" 


#define SEALEVELPRESSURE_HPA (1013.25) //давление на уровне моря


WiFiClient espClient;
PubSubClient client(espClient); //клиент для mqtt


Preferences prefs; // для хранения данных во внутренней памяти
AsyncWebServer server(80); //сервер
AsyncWebSocket ws("/ws"); //протокол для отправки данных с esp
AsyncWebSocketClient * globalClient = NULL; // для ws протокола
Adafruit_BME280 bme; // I2C


         //Edit here pin Reset
bool  Reset = 0;
const char* ssid = "ESP32_Team1";     // Edit here AP name
const char* password = "12345678";   //Edit here AP password
  //Edit here AP website IP


String wifi_ssid = ""; //первоначальные переменные вай-фая и задержки
String wifi_pass = "";
unsigned long delayTimeHtml = 1000;


int trying = 0; //количетсво попыток подключения к фай-фаю


//страницы esp для показаний датчиков и настройки
const char* html_results = R"( 
<!DOCTYPE html>
<html>
    <head>
        <title>ESP IOT DASHBOARD</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="icon" type="image/png" href="favicon.png">
        <link rel="stylesheet" type="text/css" href="style.css">
    </head>
    <body>
        <div class="topnav">
        
            <h1>SENSOR READINGS (WEBSOCKET)</h1>
            <div> <a href= "/wifi">Wifi settings</a> </div>
        </div>
        <div class="content">
            <div class="card-grid">
                <div class="card">
                    <p class="card-title">Team</p>
                    <p class="reading"><span id="team"></span> </p>
                </div>
                <div class="card">
                    <p class="card-title"><i class="fas fa-thermometer-threequarters" style="color:#059e8a;"></i> Temperature</p>
                    <p class="reading"><span id="temperature"></span> &deg;C</p>
                </div>
                <div class="card">
                    <p class="card-title"> Humidity</p>
                    <p class="reading"><span id="humidity"></span> &percnt;</p>
                </div>
                <div class="card">
                    <p class="card-title"> Pressure</p>
                    <p class="reading"><span id="pressure"></span> hpa</p>
                </div>
            </div>
        </div>
        <script src="script.js"></script>
    </body>
</html>
<style>
html {
    font-family: Arial, Helvetica, sans-serif;
    display: inline-block;
    text-align: center;
}
h1 {
    font-size: 1.8rem;
    color: white;
}
.topnav {
    overflow: hidden;
    background-color: #0A1128;
}
body {
    margin: 0;
}
.content {
    padding: 50px;
}
.card-grid {
    max-width: 800px;
    margin: 0 auto;
    display: grid;
    grid-gap: 2rem;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
}
.card {
    background-color: white;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
}
.card-title {
    font-size: 1.2rem;
    font-weight: bold;
    color: #034078
}
.reading {
    font-size: 1.2rem;
    color: #1282A2;
}
</style>
<script>
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
}
</script>
)";


const char* html = R"(
<!DOCTYPE HTML>
<html>
<head>
<meta charset="UTF-8">
</head>
  <body>
  <div class = "nav_bar"> <a href= "/">Results</a> </div>
    <h2>ESP32 Configuration Wi-Fi</h2>
    <form id="myForm">
      SSID WiFi: <input type="text" id="input1" name="SSID WiFi:"><br><br>
      Password: <input type="text" id="input2" name="Password:"><br><br>
      Delay: <input type="range" id="delay" name="delay" min="1" max="60" value="1" oninput="document.getElementById('delayValue').innerHTML = this.value + 's'"><span id="delayValue">1s</span><br><br>
      <input type="submit" value="Send">
    </form>
    
  </body>
<script>
document.getElementById("myForm").onsubmit = function(e) {
  e.preventDefault();
  var input1Value = encodeURIComponent(document.getElementById("input1").value);
  var input2Value = encodeURIComponent(document.getElementById("input2").value);
  var delayValue = encodeURIComponent(document.getElementById("delay").value)
  fetch("/wifi", {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
    },
    body: "input1=" + input1Value + "&input2=" + input2Value + "&delay=" + delayValue,
  })
      .then((response) => {
        alert("Успешно отправлено, устройство будет пытаться подключиться к вай фаю");
      })
      .catch((error) => {
        console.error("Error sending data.");
      });
  };
</script>
</html>

<style>
  body {
    font-family: Arial, sans-serif;
    text-align: center;
  }
  h2 {
    margin-bottom: 20px;
  }
  form {
    display: inline-block;
    width: 300px;
    text-align: left;
  }
  input[type="text"] {
    width: 100%;
  }
  input[type="submit"] {
    width: 100%;
    background-color: #4CAF50;
    color: white;
    padding: 14px 20px;
    margin: 8px 0;
    border: none;
    border-radius: 4px;
    cursor: pointer;
  }
  input[type="submit"]:hover{
    background-color: #103612;
  }
  .nav_bar {
    background-color: #333;
    overflow: hidden;
    display: flex;
    justify-content: center;
    align-items: center;
  }
  .nav_bar a {
    float: left;
    display: block;
    color: white;
    text-align: center;
    padding: 14px 16px;
    text-decoration: none;
  }
  .nav_bar a:hover {
    background-color: #ddd;
    color: black;
  }
</style>
)";


unsigned long delayTime = 5000; //задержка для хранения в памяти


//События для ws протокола
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
 
  if(type == WS_EVT_CONNECT){
 
    Serial.println("Websocket client connection received");
    globalClient = client;
 
  } else if(type == WS_EVT_DISCONNECT){
 
    Serial.println("Websocket client connection finished");
    globalClient = NULL;
 
  }
}


//получение результатов с датчиков
String getRes() {
  char buffer[512];
  JsonDocument doc;
  Serial.print("Temperature = ");
  float temperature = bme.readTemperature();
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print("Humidity = ");
  float humidity = bme.readHumidity();
 
  Serial.print(humidity);
  Serial.println(" %");
  float pressure = bme.readPressure() / 100.0F;
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");


  
  doc["team"] = "[SeaStream] Команда 1";
  
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/
  
 


  
  
  
  // // char JSONmessageBuffer[100];
  
  serializeJsonPretty(doc,   buffer);
  
  return(buffer);
  }




//настройка MQTT 
void MQTTSetup(){
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  if (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected to mqtt server");
 
    } 
  }
  
 
  
}


//Отправка данных MQTT
void MQTTLoop(){
  char buffer[512];
  String res = getRes();
  JsonDocument doc;
  
  deserializeJson(doc, res);
  serializeJsonPretty(doc,   buffer);

  Serial.println(buffer);
  if ( client.publish("/", buffer ) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
  client.loop();
  
}


//строка в число для задержки
long stringToLong(String s)
{
    char arr[12];
    s.toCharArray(arr, sizeof(arr));
    return atol(arr);
}


//настройка вай-фая и веб страниц
void WiFiSetup(){
  
  Serial.println("-- Default Test --");
  
  
  
  
  Serial.println();
  Serial.println("Starting..");

  
  
  String wifi_name = prefs.getString("wifi_name");
  String wifi_password = prefs.getString("wifi_password");
  unsigned long delayT = stringToLong(prefs.getString("delay"));
  Serial.println(delayT);
  Reset = prefs.getBool("reset");
  WiFi.mode(WIFI_AP_STA);
  
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

 



  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", html_results);
  });
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", html);
  });
  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    String input1 = request->arg("input1");
    String input2 = request->arg("input2");
    String input3 = request->arg("delay");
    wifi_ssid = input1;
    wifi_pass = input2;
    delayTimeHtml = stringToLong(input3)*1000;
    
    Serial.print("SSID: ");
    Serial.println(input1);
    Serial.print("Password: ");
    Serial.println(input2);
    Serial.println("Delay Time:");
    Serial.println(delayTimeHtml);
    Serial.println("Restarting..");
    prefs.putString("wifi_name", wifi_ssid);
    prefs.putString("wifi_password", wifi_pass);
    prefs.putULong("delay", delayTimeHtml);
    
    Reset=1;
    prefs.putBool("reset", Reset);
    request->send(200, "text/plain", "Data received successfully! Restarting...");
    
    delay(2000);
    
    ESP.restart();
  });
  server.begin();




 
  
  WiFi.begin(wifi_name.c_str(), wifi_password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connection in progress...");
    trying = prefs.getInt("trying");
    if (trying > 10) {
      prefs.clear();
      break;
    }
    prefs.putInt("trying", trying+1);
  }
  Serial.println("Connected to the Wi-Fi network.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}


//настройка всего
void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  bool status;
  status = bme.begin(0x76);
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  
  prefs.begin("my-app");
  
  
  WiFiSetup();
  MQTTSetup();
}


//постоянный обработчик событий
void loop(){ 
    
    if(globalClient != NULL && globalClient->status() == WS_CONNECTED){
      
      String randomNumber = String(getRes());
      globalClient->text(randomNumber);
      
   }
   unsigned long delayT = prefs.getULong("delay");
   
   delay(delayT);
   MQTTLoop();
}
