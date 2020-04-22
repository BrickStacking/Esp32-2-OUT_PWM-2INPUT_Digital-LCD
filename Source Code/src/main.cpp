#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClient.h>
WiFiClient client;
#if defined(ESP8266)
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#else
#include <WiFi.h> //https://github.com/esp8266/Arduino
#endif
//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
Ticker ticker, sent_speed;
LiquidCrystal_I2C lcd(0x3F, 20, 4);
#define ONE_WIRE_BUS_1 17
#define ONE_WIRE_BUS_2 16
#define PWM1 18
#define PWM2 19
#define BT1 32 //Redefine
#define BT2 33
#define BT3 25
#define BT4 26
#define BT5 27
#define BUILTIN_LED 2
#define BLYNK_PRINT Serial
OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sen1(&oneWire_in);
DallasTemperature sen2(&oneWire_out);

int PWM_FREQUENCY = 5000;
int PWM_CHANNEL1 = 0;
int PWM_CHANNEL2 = 0;
int PWM_RESOUTION = 10;
bool Connected2Blynk = false;
int dem1, dem2, dem3, dem4, dem5;
BlynkTimer timer;
void get_tem(void);
void out_pwm();
void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED); // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);    // set pin to the opposite state
}
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}
void myTimerEvent();
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("Esp32 Bicycle", "1234554320"))
  {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  Serial.println("Start Blynk");
  Blynk.config("75ca160b7a4b43a8b5e200e556afcad1", "gith.cf", 8442);
  timer.setInterval(10000L, myTimerEvent);
  Serial.println("Done !"); //  Serial.println("Start Blynk");


  ledcSetup(PWM_CHANNEL1, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(PWM1, PWM_CHANNEL1);

  ledcSetup(PWM_CHANNEL2, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(PWM2, PWM_CHANNEL2);

  pinMode(BT1, INPUT_PULLUP);
  pinMode(BT2, INPUT_PULLUP);
  pinMode(BT3, INPUT_PULLUP);
  pinMode(BT4, INPUT_PULLUP);
  pinMode(BT5, INPUT_PULLUP);
  sen1.begin();
  sen2.begin();
  lcd.clear();
  lcd.init();
  lcd.backlight(); //Bật đèn nền
  lcd.home();
  lcd.setCursor(2, 0);
  lcd.print("Staring Program !");
  lcd.setCursor(1, 1);
  for (int i = 0; i < 12; i++)
  {
    lcd.print(".");
    delay(70);
  }
}

void loop()
{
  Blynk.run();
  Serial.print(digitalRead(BT1));
  Serial.print(digitalRead(BT2));
  Serial.print(digitalRead(BT3));
  Serial.print(digitalRead(BT4));
  Serial.println(digitalRead(BT5));
  get_tem();
  ledcWrite(PWM_CHANNEL1, 500);
  ledcWrite(PWM_CHANNEL2, 500);
  delay(3000);
  ledcWrite(PWM_CHANNEL1, 1023);
  ledcWrite(PWM_CHANNEL2, 1023);
  delay(3000);
}

void get_tem(void)
{

  Serial.print("Requesting temperatures...");
  sen1.requestTemperatures();
  sen2.requestTemperatures();
  Serial.println(" done");

  Serial.print("Inhouse: ");
  Serial.println(sen1.getTempCByIndex(0));

  Serial.print("Outhouse: ");
  Serial.println(sen2.getTempCByIndex(0));
}

void out_pwm()
{
  ledcWrite(PWM_CHANNEL1, 500);
  ledcWrite(PWM_CHANNEL2, 500);
  delay(2000);
  ledcWrite(PWM_CHANNEL1, 1023);
  ledcWrite(PWM_CHANNEL2, 1023);
  delay(2000);
}

void myTimerEvent()
{
 Connected2Blynk = Blynk.connected();
 if (!Connected2Blynk) {
   Serial.println("Not connected to Blynk server");
 }
 else {
   Serial.println("Still connected to Blynk server");
 }
}