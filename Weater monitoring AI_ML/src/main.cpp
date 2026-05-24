#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <Ubidots.h>
#include <DHT.h>

#ifndef wifikeyconfig
#define wifikeyconfig
#define wifi_ssid "ESP8266"
#define wifi_pass "PIC16f877a"
#endif 

#ifndef iotsecureconfig
#define iotsecureconfig
#define username  "nithish14"  
#define mailid    "nithishknk14@gmail.com"
#define pasword   "PIC16f877a"
#define api       "BBUS-j0K0PWzSagP4cIncKzQbBDEQ1TrNgE"
#endif

#ifndef oledpinconfig
#define oledpinconfig
#define screen_width    128
#define screen_height   32
#define oled_scl        D1
#define oled_sda        D2
#define oled_reset      -1
#define oled_address    0x3C
#define oled_speed      400
#endif 

#ifndef dhtpinconfig
#define dhtpinconfig
#define dhtpin        D6
#define dhttype       DHT22
#define dhtinterval   1250
#endif

#ifndef ldrpinconfig
#define ldrpinconfig
#define ldrsensor D5
#endif

#ifndef rainpinconfig
#define rainpinconfig
#define rainsensor A0
#endif 

typedef enum
{
  weather_result_clear,
  weather_result_sunny,
  weather_result_mist,
  weather_result_light_drizzle,
  weather_result_light_rain,
  weather_result_overcast,
  weather_result_cloudy,
  weather_result_partly_cloudy,
  weather_result_patchy_light_drizzle,
  weather_result_fog,
  weather_result_moderate_rain,
  weather_result_rain_possible
}
weather_result_t;

DHT dht(dhtpin, dhttype);
Ubidots *ubidots{nullptr};
Adafruit_SSD1306 oled(screen_width, screen_height, &Wire, oled_reset);
SMTPSession smtp;
const char devicelabel[] = "dlab";
const int webinterval = 15000;

const char *email_sender_name = "Weather-Station";
const char *email_sender_id   = "everwiniot@gmail.com";
const char *email_sender_psk  = "xcgdaorhcbsnzxes";

const char *email_receptor_name = "Nithish";
const char *email_receptor_id   = "nithishknk14@gmail.com";
const char *email_receptor_sub  = "AI Predicted Result(s)";

const char *smtp_server = "smtp.gmail.com";
const int   smtp_port   = 465;

uint8_t rainlevel;
float rainperception;
uint8_t weatherresult;

float temperature;
float humidity;
bool islightdetected;

weather_result_t result;
uint32_t webupdateinterval;

weather_result_t get_weather_result(void)
{
  if(temperature > 29 && islightdetected && rainlevel < 20) 
  return weather_result_clear;

  if(temperature > 35 && humidity > 60 && islightdetected && rainlevel < 20)
  return weather_result_sunny;

  if(temperature < 29 && humidity > 80 && rainlevel < 50)
  return weather_result_mist;

  if(temperature < 29 && humidity > 60 && rainlevel > 50)
  return weather_result_light_drizzle;

  if(temperature < 25 && humidity > 80 && rainlevel > 60)
  return weather_result_light_drizzle;

  if(temperature < 25 && humidity > 80 && rainlevel > 80)
  return weather_result_overcast;

  if(temperature < 29 && humidity > 60 && rainlevel < 50)
  return weather_result_cloudy;

  if(temperature < 29 && humidity > 80 && rainlevel < 50)
  return weather_result_partly_cloudy;

  if(temperature < 29 && humidity > 80 && rainlevel > 50)
  return weather_result_patchy_light_drizzle;

  if(temperature < 29 && humidity > 80 && rainlevel > 50 && !islightdetected)
  return weather_result_fog;

  if(temperature > 29 && humidity > 60 && rainlevel > 50)
  return weather_result_moderate_rain;

  if(temperature > 29 && humidity > 60 && rainlevel < 50 && rainperception > 80)
  return weather_result_rain_possible;

  return weather_result_clear;
}

float get_rain_perception(void)
{
  uint8_t tresult = 0, hresult = 0, rresult = 0;

  if(temperature < 29.0F) tresult = map((unsigned)temperature, 0, 27, 50, 0);
  else tresult = 20; hresult = map((unsigned)humidity, 0, 100, 0, 80);
  rresult = map((unsigned)rainlevel, 0, 100, 0, 90);
  
  float perception = round((tresult + hresult + rresult) * 0.454545F);
  if(perception < 0.0F) perception = 0.0F; if(perception > 100.0F) perception = 100.0F;
  return perception;

}

void setup()
{
  Serial.begin(115200); while(!Serial);
  Serial.setDebugOutput(false); delay(1000);

  pinMode(rainsensor, INPUT);
  pinMode(ldrsensor, INPUT);
  pinMode(dhtpin, INPUT);
  dht.begin();

  oled.begin(SSD1306_SWITCHCAPVCC, oled_address);
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay(); oled.setCursor(0, 0);
  oled.print("IOT ENABLED ARTIFICAL\n");
  oled.print(" INTELLIGENCE AND ML \n");
  oled.print(" WEATHER MONITOR &   \n");
  oled.print(" PREDICTION SYSTEM   \n");
  oled.display(); delay(2500); oled.clearDisplay();

  Ubidots::wifiConnect(wifi_ssid, wifi_pass);
  ubidots = new Ubidots(api, UBI_TCP);
  ubidots->setDebug(false);

  webupdateinterval = millis() + webinterval;
}

void loop()
{
  islightdetected = !digitalRead(ldrsensor);
  rainlevel = map(analogRead(rainsensor), 0, 1023, 100, 0);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  rainperception = get_rain_perception();
  result = get_weather_result();

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.printf("TEMPERATURE: %.2f\r\n", temperature);
  oled.printf("   HUMIDITY: %.2f\r\n", humidity);
  oled.printf("       RAIN: %03d\r\n", rainlevel);
  oled.printf("      LIGHT: %s\r\n", (islightdetected ? "DETECT" : "NORMAL"));
  oled.display();

  if(millis() > webupdateinterval)
  {
    char buffer[100] = {0x00};

    switch(result)
    {
      case weather_result_clear:
      strcpy(buffer, "CLEAR");
      break;

      case weather_result_sunny:
      strcpy(buffer, "SUNNY");
      break;

      case weather_result_mist:
      strcpy(buffer, "MIST");
      break;

      case weather_result_light_drizzle:
      strcpy(buffer, "LIGHT DRIZZLE");
      break;

      case weather_result_light_rain:
      strcpy(buffer, "LIGHT RAIN");
      break;

      case weather_result_overcast:
      strcpy(buffer, "OVERCAST");
      break;

      case weather_result_cloudy:
      strcpy(buffer, "CLOUDY");
      break;

      case weather_result_partly_cloudy:
      strcpy(buffer, "PARTLY CLOUDY");
      break;

      case weather_result_patchy_light_drizzle:
      strcpy(buffer, "PATCHY LIGHT DRIZZLE");
      break;

      case weather_result_fog:
      strcpy(buffer, "FOG");
      break;

      case weather_result_moderate_rain:
      strcpy(buffer, "MODERATE RAIN");
      break;

      case weather_result_rain_possible:
      strcpy(buffer, "POSSIBLE RAIN");
      break;
    }

    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.printf("Perception: %.2f\r\n", rainperception);
    oled.printf("Prediction: %s\r\n", buffer);
    oled.display(); delay(500);

    char *context = (char*)malloc(sizeof(char) * 60);
    ubidots->addContext("prediction", buffer);
    ubidots->getContext(context);

    ubidots->add("temp", temperature);
    ubidots->add("hum", humidity);
    ubidots->add("rain", rainlevel);
    ubidots->add("light", islightdetected);
    ubidots->add("perc", rainperception, context);
    ubidots->send(); free(context);

    char *ptr = (char*)(malloc(512 * sizeof(char)));
    snprintf(ptr, 512, "Temperature: %.2f\r\nHumidity: %.2f\r\nRain: %03d\r\nLight: %s\r\nPrecipitation: %.2f%%\r\n", 
    temperature, humidity, rainlevel, (islightdetected ? "DETECT" : "NORMAL"), rainperception);
    strcat(ptr, "Result: "); strcat(ptr, (char*)buffer); 

    Session_Config mail;
    mail.server.host_name = smtp_server;
    mail.server.port = smtp_port;
    mail.login.email = email_sender_id;
    mail.login.password = email_sender_psk;
    mail.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = email_sender_name;
    message.sender.email = email_sender_id;
    message.subject = email_receptor_sub;
    message.addRecipient(email_receptor_name, email_receptor_id);
    message.text.content = ptr;
    message.text.charSet =  F("us-ascii");
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    smtp.connect(&mail);
    MailClient.sendMail(&smtp, &message, true);
    smtp.sendingResult.clear();
    free(ptr);

    webupdateinterval = millis() + webinterval;
  }
}
