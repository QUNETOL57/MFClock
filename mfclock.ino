//===========================================================================================
#include <ESP8266WiFi.h>
#include <PubSubClient.h>//mqtt
#include <time.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>// BMP180
#include <LiquidCrystal_I2C.h>
//===========================================================================================
const char *ssid = "Xiaomi_42BF"; // Имя роутера
const char *pass = "QEKb446C"; // Пароль роутера

const char *mqtt_server = "m10.cloudmqtt.com"; // Имя сервера MQTT
const int mqtt_port = 15289; // Порт для подключения к серверу MQTT
const char *mqtt_user = "mxhzqhmv"; // Логи для подключения к серверу MQTT
const char *mqtt_pass = "lBXUXyo_Kj-S"; // Пароль для подключения к серверу MQTT

const char *mqtt_topic_temp = "home/room1/temp/temp_room1"; // Топик для отправки температуры
const char *mqtt_topic_hum = "home/room1/hum/hum_room1"; // Топик для отправки влажности

//===========================================================================================
#define PIN_PHOTO_SENSOR A0
int timezone = 3 * 3600;//Временная зона 
int dst = 0;

WiFiClient wclient; 
PubSubClient client(wclient, mqtt_server, mqtt_port);
Adafruit_BMP085 bmp;
LiquidCrystal_I2C lcd(0x3F,16,2);
//===========================================================================================
void wifi_connect(){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
    
    if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
    Serial.println("WiFi connected");
  }
}
//===========================================================================================
// Функция получения времени по интернету
void internet_time(){
  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
  Serial.println("Waiting for Internet time");
  while(!time(nullptr)){
     Serial.print(".");
     delay(1000);
  }
  Serial.println("\nTime response....OK");     
}
//===========================================================================================
// Функция получения данных от сервера
void callback(const MQTT::Publish& pub)
{
  String payload = pub.payload_string();
  String topic = pub.topic();
  
  Serial.print(pub.topic()); // выводим в сериал порт название топика
  Serial.print(" => ");
  Serial.println(payload); // выводим в сериал порт значение полученных данных
}
//===========================================================================================
// Функция отправки показаний
//void data_send() {
//  delay(5000);
//  do{
//    temp = dht.readTemperature();        // Считывание температуры в градусах цельсия
//    hum = dht.readHumidity();           // Считывание влажности в процентах
//    Serial.print("Temp: ");
//    Serial.println(temp);
//    Serial.print("Hum: ");
//    Serial.println(hum);
//    delay(1000);
//  } while(temp != temp and hum != hum);
//  
//  client.publish(mqtt_topic_temp, String(temp));
//  client.publish(mqtt_topic_hum, String(hum));
//  delay(5000);
//}
//===========================================================================================
// Функция подключения к MQTT
void mqtt_connect(){
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.print("Connecting to MQTT server ");
      Serial.print(mqtt_server);
      Serial.println("...");
      if (client.connect(MQTT::Connect("smarthouse").set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server ");
        client.set_callback(callback);
        // подписываемся под топики
        client.subscribe("mqtt_topic_temp");
      } else {
        Serial.println("Could not connect to MQTT server"); 
      }
    }
    
    if (client.connected()){
//      client.loop();
//      data_send();
    }
  
  }
}
//===========================================================================================
void setup() {  
  Serial.begin(115200);
  Serial.setTimeout(1000);// Инициализация подключения
  Serial.println();
  Serial.println("------------------------------------------------");
  Serial.println("Device started");
  Serial.println("BMP180 initialize");
  bmp.begin();
  wifi_connect();
  mqtt_connect();
  internet_time();
  lcd.init();                      // Инициализация дисплея  
  lcd.backlight();                 // Подключение подсветки
  Serial.println("------------------------------------------------");
}
//===========================================================================================
void loop() {
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure()/131);
  Serial.println(" mm");
  
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  Serial.print(p_tm->tm_mday);
  Serial.print("/");
  Serial.print(p_tm->tm_mon + 1);
  Serial.print("/");
  Serial.print(p_tm->tm_year + 1900);
  
  Serial.print(" ");
  
  Serial.print(p_tm->tm_hour);
  Serial.print(":");
  Serial.print(p_tm->tm_min);
  Serial.print(":");
  Serial.println(p_tm->tm_sec);
  
  delay(1000);
//  int val = analogRead(PIN_PHOTO_SENSOR);
//  lcd.setCursor(0,0);              // Установка курсора в начало первой строки
//  lcd.print("Hello");       // Набор текста на первой строке
//  lcd.setCursor(0,1);              // Установка курсора в начало второй строки
//  lcd.print(String(bmp.readTemperature()));       // Набор текста на второй строке

}
//===========================================================================================
