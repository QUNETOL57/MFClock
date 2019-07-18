//===========================================================================================
#include <ESP8266WiFi.h>
#include <PubSubClient.h>//mqtt
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP085.h>// BMP180
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <GyverButton.h> //кнопки
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>
//===========================================================================================
//это нужно для подключения дополнительных символов дисплея
          #if defined(ARDUINO) && ARDUINO >= 100
          #define printByte(args)  write(args);
          #else
          #define printByte(args)  print(args,BYTE);
          #endif
//===========================================================================================
#define PIN_PHOTO_SENSOR A0
#define BTN1 14    // (D5) Кнопка 1
#define BTN2 12    // (D6) Кнопка 2
#define BTN3 13    // (D7) Кнопка 3
#define DEBUG true //Вывод в Serial
int mode = 0; //выбор начального экрана 
int modesave = 0;
int al_pos = 1;
//===========================================================================================
const char *ssid = "----"; // Имя роутера
const char *pass = "----"; // Пароль роутера

const char *mqtt_server = "----"; // Имя сервера MQTT
const int mqtt_port = ----; // Порт для подключения к серверу MQTT
const char *mqtt_user = "----"; // Логи для подключения к серверу MQTT
const char *mqtt_pass = "----"; // Пароль для подключения к серверу MQTT

const char *mqtt_topic_temp = "----"; // Топик для отправки температуры
const char *mqtt_topic_hum = "----"; // Топик для отправки влажности
//===========================================================================================

int timezone = 3 * 3600;//Временная зона 3 для МСК
int dst = 0;

WiFiClient wclient; 
PubSubClient client(wclient, mqtt_server, mqtt_port);
Adafruit_BMP085 bmp;
LiquidCrystal_I2C lcd(0x3F,16,2);
GButton butt1(BTN1);
GButton butt2(BTN2);
GButton butt3(BTN3);
SoftwareSerial mp3Serial(D4, D3); // RX, TX
byte BC[8][8] = 
{
  {0B11111,0B11111,0B11011,0B11011,0B11011,0B11011,0B11111,0B11111},
  {0B11000,0B11000,0B11000,0B11000,0B11000,0B11000,0B11111,0B11111},
  {0B11111,0B11111,0B11011,0B11011,0B11011,0B11011,0B11011,0B11011},
  {0B00011,0B00011,0B00011,0B00011,0B00011,0B00011,0B11111,0B11111},
  {0B11111,0B11111,0B11000,0B11000,0B11000,0B11000,0B11111,0B11111},
  {0B11111,0B11111,0B00011,0B00011,0B00011,0B00011,0B11111,0B11111},
  {0B00011,0B00011,0B00011,0B00011,0B00011,0B00011,0B00011,0B00011},
  {0B11011,0B11011,0B11011,0B11011,0B11011,0B11011,0B11111,0B11111}
};

byte AL[8][8] = 
{
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000},
  {0B11111,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000}
};

byte GR[8][8] = 
{
  {0B11111,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111},
  {0B00000,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111},
  {0B00000,0B00000,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111},
  {0B00000,0B00000,0B00000,0B11111,0B11111,0B11111,0B11111,0B11111},
  {0B00000,0B00000,0B00000,0B00000,0B11111,0B11111,0B11111,0B11111},
  {0B00000,0B00000,0B00000,0B00000,0B00000,0B11111,0B11111,0B11111},
  {0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B11111,0B11111},
  {0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B11111}
};
int stat[2][13]; //25Возьмем 2х мерный массив, где один массив время(контроль действительности значения) и второй массив наши данные

char BD1 [10] = {2,6,5,5,7,4,1,2,0,0};
char BD2 [10] = {7,6,1,3,6,3,7,6,7,3};
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
// Добавление 0
String updatenumbers(int numb){
  if(numb <= 9)
    return "0" + String(numb);
  else
    return String(numb); 
}
//===========================================================================================
// День недели 
String day_week(int wday){
  switch(wday){ 
    case 1: return String("Monday"); break; 
    case 2: return String("Tuesday"); break; 
    case 3: return String("Wednesday"); break; 
    case 4: return String("Thursday"); break; 
    case 5: return String("Friday"); break; 
    case 6: return String("Saturday"); break; 
    case 7: return String("Sunday"); break; 
  }
}
//===========================================================================================
//Дебаг и вывод Serial 
void debag(String thour,String tmin,String tsec,String dday,String dmon,int dyear,int temp,int pres){
  Serial.print("Temperature = ");
  Serial.print(temp);
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(pres);
  Serial.println(" mm");
  Serial.print(dday);
  Serial.print("/");
  Serial.print(dmon);
  Serial.print("/");
  Serial.print(dyear);
  Serial.print(" ");
  Serial.print(thour);
  Serial.print(":");
  Serial.print(tmin);
  Serial.print(":");
  Serial.print(tsec);
  Serial.println();
}
//===========================================================================================
void small_screen(String thour,String tmin,String dday,String dmon,int temp,int pres, int wday){
//  lcd.clear();
  //time
  lcd.setCursor(0,0);              
  lcd.print(thour);   
  lcd.print(":");
  lcd.print(tmin);
  //day
  lcd.setCursor(6,0);
  lcd.print(day_week(wday)); 
  //dete
  lcd.setCursor(9,0);
  lcd.print(" " + dday); 
//  lcd.print(dday); 
  lcd.setCursor(12,0);
  lcd.print("/");
  lcd.setCursor(13,0);
  lcd.print(dmon); 
  //temp
  lcd.setCursor(0,1);
  lcd.print("Temp:");
  lcd.setCursor(5,1);         
  lcd.print(temp); 
  lcd.setCursor(8,1); 
  lcd.print("Pres:");
  lcd.setCursor(13,1);
  lcd.print(pres);
}
void bigtime_screen(int thour, int tmin,int tsec,String dday,String dmon,int dyear,int wday){
  for (int i=0;i<8;i++) lcd.createChar(i,BC[i]);//.Создание символов часов
  byte h1 = thour/10;
  byte h2 = thour%10;
  byte m1 = tmin/10;
  byte m2 = tmin%10;
  byte s1 = tsec/10;
  byte s2 = tsec%10;
  
  lcd.setCursor(0,0);
  lcd.print(BD1[h1]);
  lcd.print(BD1[h2]);
  lcd.print((char)0xa5);
  lcd.print(BD1[m1]);
  lcd.print(BD1[m2]);
  
  lcd.print((char)(s1+'0'));
  lcd.print((char)(s2+'0'));
    
  lcd.setCursor(0,1);
  lcd.print(BD2[h1]);
  lcd.print(BD2[h2]);
  lcd.print((char)0xa5);
  lcd.print(BD2[m1]);
  lcd.print(BD2[m2]);

  lcd.setCursor(8,0);
  lcd.print(dday); 
  lcd.setCursor(10,0);
  lcd.print("/");
  lcd.setCursor(11,0);
  lcd.print(dmon); 
  lcd.setCursor(13,0);
  lcd.print("/");
  lcd.setCursor(14,0);
  lcd.print(dyear - 2000);
  
  lcd.setCursor(7,1);
  lcd.print(day_week(wday));  
}
void temp_screen(int temp,int pres,int wday){
  lcd.setCursor(0,0);
  lcd.print(wday);  
}
void alarm_screen(){
  for (int i=0;i<8;i++) lcd.createChar(i,AL[i]);//.Создание символов часов
   //time
  int h1 = 0;
  int h2 = 0;
  int m1 = 0;
  int m2 = 0;
  
  lcd.setCursor(1,0);
  lcd.print(h1);
  lcd.print(h2);                 
  lcd.setCursor(3,0);
  lcd.print(":");
  lcd.setCursor(4,0);
  lcd.print("00");
  lcd.setCursor(1,al_pos);
  lcd.write(1);
  if (butt1.isPress()) {
    if (mode == 4){
      switch(al_pos){
        case 1: ++h1; break;
        case 2: ++h2; break;
        case 4: ++m1; break;
        case 5: ++m2; break;  
      }
    }
  }
}
void graph_screen(int temp){
  for (int i=0;i<8;i++) lcd.createChar(i,GR[i]);//.Создание символов графика
  lcd.setCursor(0,0);
  lcd.print("Temp now: +");
  lcd.print(temp);
  grafik(0, 1, 0); //строим график, вызов усложнен для построения разных графиков из массива, первое число номер массива, затем периодичность выборки данных, потом начальное значение в массиве. Мы берем первый массив(0), берем каждое второе значение(2) и начинаем с начала(0)
  
}
//===========================================================================================
void grafik(int x, int y, int z) {
  lcd.setCursor(0, 1);
  lcd.print("Temp");
      
  int minx = stat[x][0];
  int maxy = stat[x][0];
  for(int i=z; i<=12; i++) {
    minx = min(minx, stat[x][i]);
    maxy = max(maxy, stat[x][i]);
  }
  for (int i=z; i <= 12; i += y){
    if (stat[1][i] == 0){
      lcd.print("-");                                   //если значений нет
    } else {
      lcd.printByte(map(stat[x][i], minx, maxy, 7, 0))
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
  Serial.println("Setting up mp3 player");
  mp3_set_serial (mp3Serial);  
  // Delay is required before accessing player. From my experience it's ~1 sec
  delay(1000); 
  mp3_set_volume (15);
  Serial.println("NEXT step....");
  
//  for (int i=0;i<8;i++) lcd.createChar(i,BC[i]);//.Создание символов часов
}
//===========================================================================================
void loop() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int val = analogRead(PIN_PHOTO_SENSOR);
  int thour = p_tm->tm_hour;
  int tmin = p_tm->tm_min; 
  int tsec = p_tm->tm_sec;
  String u_thour = updatenumbers(p_tm->tm_hour);
  String u_tmin = updatenumbers(p_tm->tm_min);
  String u_tsec = updatenumbers(p_tm->tm_sec);
  String u_dday = updatenumbers(p_tm->tm_mday);
  int wday = p_tm->tm_wday;
  String u_dmon = updatenumbers(p_tm->tm_mon + 1);
  int dyear = p_tm->tm_year + 1900;
  int temp = bmp.readTemperature();
  int pres = bmp.readPressure()/131;

  
  if(DEBUG)debag(u_thour,u_tmin,u_tsec,u_dday,u_dmon,dyear,temp,pres);

  butt1.tick();
  if (butt1.isPress() and mode != 4) {    // правильная отработка нажатия с защитой от дребезга
    if (++mode >= 4) mode = 0; 
    lcd.clear();
  }
  butt3.tick();
  if (butt3.isPress() and mode != 4) {    // правильная отработка нажатия с защитой от дребезга
    if (--mode < 0) mode = 3;
    lcd.clear();
  }
  butt2.tick();
  if (butt2.isHolded()) {
    if (mode == 0 or mode == 1){
      lcd.clear();
      modesave = mode;
      mode = 4;  
    }else if (mode == 4){
      lcd.clear();
      mode = 0;
    }
  }
  
  switch (mode) {
    case 0: small_screen(u_thour,u_tmin,u_dday,u_dmon,temp,pres,wday);
      break;
    case 1: bigtime_screen(thour,tmin,tsec,u_dday,u_dmon,dyear,wday);
      break;
    case 2: temp_screen(temp,pres,wday);
      break;
    case 3: graph_screen(temp);
      break;
    case 4: alarm_screen();
      break;
  }  
  // Значения для графика
  if (((millis()/3600000)-stat[1][11]) >= 1.0){ //заполняем текущее значение раз 1 час (3600000)
       //сдвигаем массив влево, чтобы освободить предпоследнюю ячейку
    int i=0;
    for (i = 0; i < 12; i++) stat[0][i] = stat[0][i+1];
    for (i = 0; i < 12; i++) stat[1][i] = stat[1][i+1];
    Serial.println(bmp.readTemperature()); //Записываем значения давления
    stat[0][11] = bmp.readTemperature();
    stat[1][11] = millis()/3600000;   
  }
  
}
//===========================================================================================
