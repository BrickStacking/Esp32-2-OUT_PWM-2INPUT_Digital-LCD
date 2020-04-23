///////--- Add private library  ----------
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
///////--- Declare private define  ----------
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
///////--- Declare private class  ----------
OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sen1(&oneWire_in);
DallasTemperature sen2(&oneWire_out);
///////--- Declare private varible  ----------
int PWM_FREQUENCY = 5000;
int PWM_CHANNEL1 = 0;
int PWM_CHANNEL2 = 1;
int PWM_RESOUTION = 10;
bool Connected2Blynk = false;
int dem1, dem2, dem3, dem4, dem5;
int pwm1, pwm2, pwm1_low = 300, pwm1_medium = 600, pwm1_high = 1000, pwm2_low = 300, pwm2_medium = 600, pwm2_high = 1000;
int mode_control1, mode_control2;
float temp1, temp2;
unsigned long time1, time2;
int page_lcd = 0, last_page_lcd = 0;
BlynkTimer timer;
///////--- Declare private function ----------
void get_tem(void);
void out_pwm();
void display_lcd(void);
void re_update_to_blynk(void);
void button_processing();
void Heat_control_fan();
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

//////////--- Using for Blynk------/////////
BLYNK_WRITE(V2)
{
  if (param.asInt() == 0)
  {
    mode_control1 = 0;
  }
  else
    mode_control1 = 1;
  // process received value
}

BLYNK_WRITE(V3)
{
  if (param.asInt() == 0)
  {
    mode_control2 = 0;
  }
  else
    mode_control2 = 1;
  // process received value
}

BLYNK_WRITE(V6)
{
  if (1 == mode_control1)
  {
    pwm1 = param.asInt();
    Serial.println("Update PWM1");
  }
  else
  {
    Serial.println("No update PWM1");
  }

  // process received value
}

BLYNK_WRITE(V7)
{
  if (1 == mode_control2)
  {
    pwm2 = param.asInt();
    Serial.println("Update PWM2");
  }
  else
  {
    Serial.println("No update PWM2");
  }

  // process received value
}

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
  Blynk.config("0758562a1dbc4e79b67633d4316d4947", "gith.cf", 8442);
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
  time1 = millis();
  time2 = millis();
  lcd.clear();
  re_update_to_blynk(); //Set Blynk app to factory
}

/*--------- Start Loop--------------------*/
//----------------------------------------//
void loop()
{
  // ledcWrite(PWM_CHANNEL1, 400);
  // ledcWrite(PWM_CHANNEL2, 800);
  // Serial.print(digitalRead(BT1));
  // Serial.print(digitalRead(BT2));
  // Serial.print(digitalRead(BT3));
  // Serial.print(digitalRead(BT4));
  // Serial.println(digitalRead(BT5));
  Blynk.run();
  button_processing();
  Heat_control_fan();

  if (millis() - time1 > 5000)
  {
    get_tem();
    time1 = millis();
  }

  if (millis() - time2 > 5000)
  {
    lcd.clear();
    display_lcd();
    time2 = millis();
  }
  out_pwm();
  if (last_page_lcd != page_lcd)
  {
    lcd.clear();
    display_lcd();
    last_page_lcd = page_lcd;
  }
}

/*--------- End Loop--------------------*/
//----------------------------------------//
void button_processing()
{
  //--- Button 1
  if (0 == digitalRead(BT1))
  {
    dem1++;
  }
  if (1 == digitalRead(BT1))
  {
    dem1 = 0;
  }
  if (2 == dem1)
  {
    Serial.println("Button1 press");
    page_lcd--;
    if (page_lcd < 0)
    {
      page_lcd = 0;
    }
  }

  //--- Button 2
  if (0 == digitalRead(BT2))
  {
    dem2++;
  }
  if (1 == digitalRead(BT2))
  {
    dem2 = 0;
  }
  if (2 == dem2)
  {
    Serial.println("Button2 press");
    page_lcd++;
    if (page_lcd >= 9)
    {
      page_lcd = 0;
    }
  }

  //--- Button 3
  if (0 == digitalRead(BT3))
  {
    dem3++;
  }
  if (1 == digitalRead(BT3))
  {
    dem3 = 0;
  }
  if (2 == dem3)
  {
    Serial.println("Button 3 press");
    if (page_lcd == 1) //Mode 1
    {
      lcd.clear();
      mode_control1 = 0; //Set to auto
      display_lcd();
      Blynk.virtualWrite(V2, mode_control1);
      Blynk.virtualWrite(V3, mode_control2);
    }

    if (page_lcd == 2) //Mode 2
    {
      lcd.clear();
      mode_control2 = 0; //Set to auto
      display_lcd();
      Blynk.virtualWrite(V2, mode_control1);
      Blynk.virtualWrite(V3, mode_control2);
    }

    if (page_lcd == 3) // Change PWM1_LOW
    {
      lcd.clear();
      pwm1_low -= 100; //Set to auto
      if (pwm1_low < 0)
      {
        pwm1_low = 0;
      }
      display_lcd();
    }

    if (page_lcd == 4) // Change PWM1_Medium
    {
      lcd.clear();
      pwm1_medium -= 100; //Set to auto
      if (pwm1_medium < 0)
      {
        pwm1_medium = 0;
      }
      display_lcd();
    }

    if (page_lcd == 5) // Change PWM1_High
    {
      lcd.clear();
      pwm1_high -= 100; //Set to auto
      if (pwm1_high < 0)
      {
        pwm1_high = 0;
      }
      display_lcd();
    }

    if (page_lcd == 6) // Change PWM2_LOW
    {
      lcd.clear();
      pwm2_low -= 100; //Set to auto
      if (pwm2_low < 0)
      {
        pwm2_low = 0;
      }
      display_lcd();
    }

    if (page_lcd == 7) // Change PWM1_Medium
    {
      lcd.clear();
      pwm2_medium -= 100; //Set to auto
      if (pwm2_medium < 0)
      {
        pwm2_medium = 0;
      }
      display_lcd();
    }

    if (page_lcd == 8) // Change PWM1_High
    {
      lcd.clear();
      pwm2_high -= 100; //Set to auto
      if (pwm2_high < 0)
      {
        pwm2_high = 0;
      }
      display_lcd();
    }
  }

  //--- Button 4
  if (0 == digitalRead(BT4))
  {
    dem4++;
  }
  if (1 == digitalRead(BT4))
  {
    dem4 = 0;
  }
  if (2 == dem4)
  {
    Serial.println("Button4 press");
    if (page_lcd == 1) //Mode 1
    {
      lcd.clear();
      mode_control1 = 1; //Set to hand
      display_lcd();
      Blynk.virtualWrite(V2, mode_control1);
      Blynk.virtualWrite(V3, mode_control2);
    }

    if (page_lcd == 2) //Mode 2
    {
      lcd.clear();
      mode_control2 = 1; //Set to hand
      display_lcd();
      Blynk.virtualWrite(V2, mode_control1);
      Blynk.virtualWrite(V3, mode_control2);
    }

    if (page_lcd == 3) // Change PWM1_LOW
    {
      lcd.clear();
      pwm1_low += 100; //Set to auto
      if (pwm1_low > 1023)
      {
        pwm1_low = 1023;
      }
      display_lcd();
    }

    if (page_lcd == 4) // Change PWM1_Medium
    {
      lcd.clear();
      pwm1_medium += 100; //Set to auto
      if (pwm1_medium > 1023)
      {
        pwm1_medium = 1023;
      }
      display_lcd();
    }

    if (page_lcd == 5) // Change PWM1_High
    {
      lcd.clear();
      pwm1_high += 100; //Set to auto
      if (pwm1_high > 1023)
      {
        pwm1_high = 1023;
      }
      display_lcd();
    }

    if (page_lcd == 6) // Change PWM2_LOW
    {
      lcd.clear();
      pwm2_low += 100; //Set to auto
      if (pwm2_low > 1023)
      {
        pwm2_low = 1023;
      }
      display_lcd();
    }

    if (page_lcd == 7) // Change PWM1_Medium
    {
      lcd.clear();
      pwm2_medium += 100; //Set to auto
      if (pwm2_medium > 1023)
      {
        pwm2_medium = 1023;
      }
      display_lcd();
    }

    if (page_lcd == 8) // Change PWM1_High
    {
      lcd.clear();
      pwm2_high += 100; //Set to auto
      if (pwm2_high > 1023)
      {
        pwm2_high = 1023;
      }
      display_lcd();
    }
  }

  //--- Button 5
  if (0 == digitalRead(BT5))
  {
    dem5++;
  }
  if (1 == digitalRead(BT5))
  {
    dem5 = 0;
  }
  if (2 == dem5)
  {
    Serial.println("Button5 press");
    page_lcd = 0;
    lcd.clear();
    display_lcd();
  }
}
void get_tem(void)
{

  Serial.print("Requesting temperatures...");
  sen1.requestTemperatures();
  sen2.requestTemperatures();
  Serial.println(" done");

  Serial.print("Inhouse: ");
  temp1 = sen1.getTempCByIndex(0);
  Serial.println(temp1);

  Serial.print("Outhouse: ");
  temp2 = sen2.getTempCByIndex(0);
  Serial.println(temp2);
  Blynk.virtualWrite(V0, temp1);
  Blynk.virtualWrite(V1, temp2);
}

void out_pwm()
{
  ledcWrite(PWM_CHANNEL1, pwm1);
  ledcWrite(PWM_CHANNEL2, pwm2);
  Blynk.virtualWrite(V4, pwm1);
  Blynk.virtualWrite(V5, pwm2);
}

void myTimerEvent()
{
  Connected2Blynk = Blynk.connected();
  if (!Connected2Blynk)
  {
    Serial.println("Not connected to Blynk server");
  }
  else
  {
    Serial.println("Still connected to Blynk server");
  }
}

void display_lcd(void)
{
  if (0 == page_lcd) //Main page
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");

    lcd.setCursor(0, 1);
    lcd.print("M1:");
    if (mode_control1 == 0)
    {
      lcd.print("Auto");
    }
    else
    {
      lcd.print("Hand");
    }

    lcd.setCursor(10, 1);
    lcd.print("M2:");
    if (mode_control2 == 0)
    {
      lcd.print("Auto");
    }
    else
    {
      lcd.print("Hand");
    }

    lcd.setCursor(0, 2);
    lcd.print("H1:");
    lcd.print(temp1);
    lcd.setCursor(10, 2);
    lcd.print("H2:");
    lcd.print(temp2);

    lcd.setCursor(0, 3);
    lcd.print("PWM1:");
    lcd.print(pwm1);
    lcd.setCursor(10, 3);
    lcd.print("PWM2:");
    lcd.print(pwm2);
  }
  if (1 == page_lcd) //Mode1
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print("Config Mode1: ");
    //lcd.setCursor(5, 2);
    if (mode_control1 == 0)
    {
      lcd.print("Auto");
    }
    else
    {
      lcd.print("Hand");
    }
  }

  if (2 == page_lcd) //Mode2
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print("Config Mode2: ");
    //lcd.setCursor(5, 2);
    if (mode_control2 == 0)
    {
      lcd.print("Auto");
    }
    else
    {
      lcd.print("Hand");
    }
  }

  if (3 == page_lcd) //PWM1_LOW
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 1 Low");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm1_low);
  }

  if (4 == page_lcd) //PWM1_Medium
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 1 Medium");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm1_medium);
  }

  if (5 == page_lcd) //PWM1_HIGH
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 1 High");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm1_high);
  }

  if (6 == page_lcd) //PWM2_LOW
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 2 Low");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm2_low);
  }

  if (7 == page_lcd) //PWM2_Medium
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 2 Medium");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm2_medium);
  }

  if (8 == page_lcd) //PWM2_High
  {
    lcd.setCursor(2, 0);
    lcd.print("Heat Controling");
    lcd.setCursor(0, 1);
    lcd.print(" Fan Speed 2 High");
    lcd.setCursor(6, 2);
    lcd.print("Value:");
    lcd.print(pwm2_high);
  }
}

void re_update_to_blynk()
{
  Blynk.virtualWrite(V0, temp1);
  Blynk.virtualWrite(V1, temp2);
  Blynk.virtualWrite(V2, mode_control1);
  Blynk.virtualWrite(V3, mode_control2);
  Blynk.virtualWrite(V4, pwm1);
  Blynk.virtualWrite(V5, pwm2);
}

void Heat_control_fan()
{
  ///////////////////////
  if (0 == mode_control1)
  {
    if (temp1 < 25)
    {
      pwm1 = pwm1_low;
    }

    if (temp1 < 35 && temp1 >= 25)
    {
      pwm1 = pwm1_medium;
    }

    if (temp1 > 35)
    {
      pwm1 = pwm1_high;
    }
  }
  ///////////////////////
  if (0 == mode_control2)
  {
    if (temp2 < 25)
    {
      pwm2 = pwm2_low;
    }

    if (temp2 < 35 && temp2 >= 25)
    {
      pwm2 = pwm2_medium;
    }

    if (temp2 > 35)
    {
      pwm2 = pwm2_high;
    }
  }
}