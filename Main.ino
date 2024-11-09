/*********
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <Wire.h> //для определения датчика
#include <Adafruit_Sensor.h>//работа
#include <Adafruit_BME280.h> // с датчиком
#include <ArduinoJson.h> //для отправки данных в json виде
#include <WiFi.h> // подключение к вай фаю
#include <PubSubClient.h> // для подключения к mqtt


#define ssid "Wifi" //WiFi SSID(имя вай-фая)
#define password "Password" //WiFi password(пароль от Вай-Фая)
#define mqttServer "server.url" //Host(Хост, для подключения к mqtt) 
#define mqttPort 9999 //Port (Порт для подключения к mqtt)
#define mqttUser "user" // пользователь mqtt
#define mqttPassword "password" // пароль от mqtt
#define SEALEVELPRESSURE_HPA (1013.25) //Определение давления на уровне моря 


WiFiClient espClient; // инициализация WifiClient'a
PubSubClient client(espClient); //Инициализация для дальнейшей работы с интернетом


Adafruit_BME280 bme; // I2C #Датчик
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime; //инициализация переменной задержки перед следующим отправлением


void setup() {

  Serial.begin(115200); //скорость для серийного монитора и передачи данных
  Serial.println(F("BME280 test")); //проверка вывода

  bool status; // переменная для статуса, проверка работы


  WiFi.begin(ssid,password); //Начало подключения к вай-фаю
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.println("Connecting to WiFi..."); //подключение к файваю каждые 500 милисекунд пока не будет подключен

  }


  Serial.println("Connected to the WiFi network"); //Выдаем информацио о подключении


  client.setServer(mqttServer, mqttPort); // подключение к mqtt серверу
  while (!client.connected()) { 
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected to mqtt server"); //подключение к Mqtt
 
    } 
    delay(500);
  }
 
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76); //Инициализация датчика погоды  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); //Если не найден датчик, то пишем соответствующее оповещение
    while (1);
  }


  Serial.println("-- Default Test --");
  delayTime = 10000;
  Serial.println();
}

//основной поток
void loop() { 

  SendAndPrintValues(); //пишем и отправляем данные на сервер
  delay(delayTime);

}


void SendAndPrintValues() {

  char buffer[512]; //буфер для хранения json массива
  
  float temperature = bme.readTemperature(); //получаем температуру

  Serial.print("Temperature = "); 
  Serial.print(temperature);
  Serial.println(" *C");
  

  float humidity = bme.readHumidity(); //получаем влажность
  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");


  float pressure = bme.readPressure() / 100.0F; //получаем давление на уровне моря
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");


  JsonDocument doc; //создаём json объект
  doc["team"] = "TeamName"; //добавляем данные
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/

  serializeJsonPretty(doc,   buffer); //данные не в одну строчку


  // Serial.println("Sending message to MQTT topic..");
  // Serial.println(JSONbuffer);
  if ( client.publish("/", buffer) == true) {
  
    Serial.println("Success sending message"); //если успешно отправились данные

  } 
  else {

    Serial.println("Error sending message"); //если данные не отправились

  }
 
  client.loop(); //запускаем поток
  Serial.println("-------------");
 
}
