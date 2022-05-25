#include "time.h"
#include <WiFi.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontRobotron.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define NUM_LEDS 256
#define GAS_ANALOG 4

#define BTN_PIN        18
#define LED_PIN        12
#define COLOR_ORDER    RGB
#define CHIPSET        WS2812B

#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT  -8
#define MATRIX_TYPE    VERTICAL_ZIGZAG_MATRIX // Wie sind die LEDs angeordnet

const char* ssid       = "jonny";
const char* password   = "jonny004";
int wetterTyp;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
int buttonCount;
bool zustand = LOW;
bool alterZustand = LOW;
long int letzteFlankenZeit = 0;

int timesec = 0;
int timemin = 0;
int timehour = 0;
long int letzteZeit = 0;

int temp = 0;
int humi = 0;


String openWeatherMapApiKey = "29ff19f3a5eb6badddff46120f81f746";

String city = "Innsbruck";
String countryCode = "AT";


unsigned long lastTime = 0;
const char* wetter;
unsigned long timerDelay = 10000;

String jsonBuffer;

CRGB lels[NUM_LEDS];

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

cLEDText ScrollingMsg;

static int PixelPosition(int x, int y) {
  if (x % 2 == 0) {
    return 8 * x + y;

  }
  else
  {
    return 8 * x + 7 - y;
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  timesec = timeinfo.tm_sec;
  timemin = timeinfo.tm_min;
  timehour = timeinfo.tm_hour;
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println("-------------");
  Serial.print(timehour % 10);
  Serial.println(timehour / 10);
  Serial.print(timemin % 10);
  Serial.println(timemin / 10);
  Serial.print(timesec % 10);
  Serial.println(timesec / 10);
  Serial.println("-------------");
}
void TaskBtn( void *pvParameters );
void TaskBlink( void *pvParameters );
void TaskAnalogReadA3( void *pvParameters );
void TaskGetTime( void *pvParameters );
void TaskSetTemp( void *pvParameters );
void TaskGetTemp( void *pvParameters );
unsigned char TxtDemo[] = { EFFECT_HSV_CV "\x00\xff\xff\x50\xff\xff" "                            "};
// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    TaskBtn
    ,  "TaskBtn"   // A name just for humans
    ,  16000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  16000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    , 1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskAnalogReadA3
    ,  "AnalogReadA3"
    ,  16000  // Stack size
    ,  NULL
    ,  1 // Priority
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskGetTime
    ,  "GetTime"
    ,  16000  // Stack size
    ,  NULL
    ,  1 // Priority
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskGetTemp
    ,  "GetTemp"
    ,  16000
    ,  NULL
    ,  1
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskSetTemp
    ,  "GetTemp"
    ,  16000
    ,  NULL
    ,  1
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  FastLED.setBrightness(10); // WICHTIG - Hier wird die Helligkeit eingestellt. Am Anfang einen niedrigen Wert verwenden, und langsam hochtasten.
  FastLED.clear(true);
  ScrollingMsg.SetFont(RobotronFontData);
  ScrollingMsg.Init(&leds, leds.Width(), leds.Height() + 1, 0, 0);
  Serial.println(sizeof(TxtDemo));
  ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0xff);

  pinMode(BTN_PIN, INPUT);

}

void loop()
{
  // Empty. Things are done in Tasks.
}
/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/
void TaskBtn(void *pvParameters)
{
  (void) pvParameters;

  for(;;){
    zustand = digitalRead(BTN_PIN);
    bool steigFlanke = zustand && !alterZustand;
    bool irgendeineFlanke = (zustand != alterZustand);
    if(steigFlanke && (millis() > letzteFlankenZeit+100)){
      buttonCount++;
      Serial.println("-:-:-:-:-:-:-:-:-:-:-:-:-:-:-");
      Serial.println(buttonCount);
      Serial.println("-:-:-:-:-:-:-:-:-:-:-:-:-:-:-");
    }
    if(irgendeineFlanke) letzteFlankenZeit=millis();
    alterZustand=zustand;
    if(buttonCount == 5){
      buttonCount = 0;
    }
  }
}
void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  FastLED.addLeds<NEOPIXEL, LED_PIN>(lels, NUM_LEDS);
  FastLED.setBrightness(255);

  for (;;) {
    if(buttonCount == 0){
      vTaskResume(NULL);
    }else{
      vTaskSuspend(NULL);
    }
    if (millis() >= letzteZeit + 5000)
    {
      TxtDemo[15] = (timehour / 10) + 48;
      TxtDemo[16] = (timehour % 10) + 48;
      TxtDemo[17] = 58;
      TxtDemo[18] = (timemin / 10) + 48;
      TxtDemo[19] = (timemin % 10) + 48;
      TxtDemo[20] = 58;
      TxtDemo[21] = (timesec / 10) + 48;
      TxtDemo[22] = (timesec % 10) + 48;
      ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
      letzteZeit = millis();
    }

    if (ScrollingMsg.UpdateText() == -1) {
      ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
      vTaskDelay(500);
    }
    else {
      FastLED.show();
      
      vTaskDelay(20);
    }
  }
}
void TaskAnalogReadA3(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;)
  {
    if(buttonCount == 1){
      vTaskResume(NULL);
    }else{
      vTaskSuspend(NULL);
    }
    // read the input on analog pin A3:
    int sensorValue = analogRead(GAS_ANALOG);
    // print out the value you read:
    Serial.println(sensorValue);
    vTaskDelay(1000);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskGetTime(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;)
  {
    if(buttonCount == 2){
      vTaskResume(NULL);
    }else{
      vTaskSuspend(NULL);
    }
    vTaskDelay(1000);
    printLocalTime();
  }
}
void TaskGetTemp(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;)
  {
    if(buttonCount == 3){
      vTaskResume(NULL);
    }else{
      vTaskSuspend(NULL);
    }
    vTaskDelay(1000);
    if ((millis() - lastTime) > timerDelay) {

      if (WiFi.status() == WL_CONNECTED) {
        String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;

        jsonBuffer = httpGETRequest(serverPath.c_str());

        JSONVar myObject = JSON.parse(jsonBuffer);

        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }

        temp = int(myObject["main"]["temp"]) - 273.15;
        humi = int(myObject["main"]["humidity"]);
        int wind = int(myObject["wind"]["speed"]);
        wetter = myObject["weather"][0]["description"];

        Serial.print("Temperature: ");
        Serial.println(temp);
        Serial.print("Humidity: ");
        Serial.println(humi);
        Serial.print("Wind Speed: ");
        Serial.println(wind);
        Serial.print("Weather: ");
        Serial.println(myObject["weather"][0]["description"]);


      }
      else {
        Serial.println("WiFi Disconnected");
      }
      lastTime = millis();
    }

  }
}
void TaskSetTemp(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  FastLED.addLeds<NEOPIXEL, LED_PIN>(lels, NUM_LEDS);
  FastLED.setBrightness(255);

  for (;;) {
    if(buttonCount == 4){
      vTaskResume(NULL);
    }else{
      vTaskSuspend(NULL);
    }
    if (millis() >= letzteZeit + 3500)
    {
      int tempz = (temp / 10) + 48;
      int temps = (temp % 10) + 48;
      int humiz = (humi / 10) + 48;
      int humis = (humi % 10) + 48;
      TxtDemo[8] = tempz;
      TxtDemo[9] = temps;
      TxtDemo[10] = 67;
      TxtDemo[12] = humiz;
      TxtDemo[13] = humis;
      TxtDemo[14] = 37;
      DrawIcon(wetter);
      ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
      letzteZeit = millis();
    }
    if (ScrollingMsg.UpdateText() == -1) {
      ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
      vTaskDelay(1000);
    }
    else {
      FastLED.show();
      vTaskDelay(30);
    }
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void DrawIcon(String s) {

  Serial.println(s + "h");
  Serial.println(wetterTyp);

  if ( s.equals("scattered clouds") || s.equals("cloudy") || s.equals("broken clouds")) {
    wetterTyp = 0;
  }
  if ( s.equals("clear sky")) {
    wetterTyp = 1;
  }
  if ( s.equals("rain")) {
    wetterTyp = 2;
  }
  if ( s.equals("snow")) {
    wetterTyp = 3;
  }
  switch (wetterTyp) {
    case 0:
      lels[PixelPosition(0, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(0, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 3)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 3)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 3)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 3)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 3)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(7, 4)] = CRGB(47, 79, 79);
      lels[PixelPosition(7, 5)] = CRGB(47, 79, 79);
      FastLED.show();
      break;
    case 1:
      lels[PixelPosition(7, 7)] = CRGB(255, 150, 0);
      lels[PixelPosition(7, 6)] = CRGB(255, 150, 0);
      lels[PixelPosition(7, 5)] = CRGB(255, 150, 0);
      lels[PixelPosition(7, 4)] = CRGB(255, 150, 0);
      lels[PixelPosition(7, 3)] = CRGB(255, 150, 0);
      lels[PixelPosition(6, 7)] = CRGB(255, 150, 0);
      lels[PixelPosition(6, 6)] = CRGB(255, 150, 0);
      lels[PixelPosition(6, 5)] = CRGB(255, 150, 0);
      lels[PixelPosition(6, 4)] = CRGB(255, 150, 0);
      lels[PixelPosition(6, 2)] = CRGB(255, 150, 0);
      lels[PixelPosition(5, 7)] = CRGB(255, 150, 0);
      lels[PixelPosition(5, 6)] = CRGB(255, 150, 0);
      lels[PixelPosition(5, 5)] = CRGB(255, 150, 0);
      lels[PixelPosition(5, 1)] = CRGB(255, 150, 0);
      lels[PixelPosition(4, 7)] = CRGB(255, 150, 0);
      lels[PixelPosition(4, 6)] = CRGB(255, 150, 0);
      lels[PixelPosition(4, 4)] = CRGB(255, 150, 0);
      lels[PixelPosition(3, 7)] = CRGB(255, 150, 0);
      lels[PixelPosition(3, 3)] = CRGB(255, 150, 0);
      lels[PixelPosition(2, 6)] = CRGB(255, 150, 0);
      lels[PixelPosition(2, 2)] = CRGB(255, 150, 0);
      lels[PixelPosition(1, 5)] = CRGB(255, 150, 0);
      FastLED.show();
      break;
    case 2:
      lels[PixelPosition(0, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 8)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(2, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(3, 8)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 8)] = CRGB(47, 79, 79);
      lels[PixelPosition(4, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(5, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 5)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(6, 7)] = CRGB(47, 79, 79);
      lels[PixelPosition(7, 6)] = CRGB(47, 79, 79);
      lels[PixelPosition(1, 4)] = CRGB(0, 0, 255);
      lels[PixelPosition(1, 3)] = CRGB(0, 0, 255);
      lels[PixelPosition(3, 3)] = CRGB(0, 0, 255);
      lels[PixelPosition(3, 2)] = CRGB(0, 0, 255);
      lels[PixelPosition(5, 4)] = CRGB(0, 0, 255);
      lels[PixelPosition(5, 3)] = CRGB(0, 0, 255);
      FastLED.show();
      break;
    case 3:
      lels[PixelPosition(0, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(1, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(1, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(2, 8)] = CRGB(37, 25, 89);
      lels[PixelPosition(2, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(2, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(2, 5)] = CRGB(37, 25, 89);
      lels[PixelPosition(3, 5)] = CRGB(37, 25, 89);
      lels[PixelPosition(3, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(3, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(3, 8)] = CRGB(37, 25, 89);
      lels[PixelPosition(4, 5)] = CRGB(37, 25, 89);
      lels[PixelPosition(4, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(4, 8)] = CRGB(37, 25, 89);
      lels[PixelPosition(4, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(5, 5)] = CRGB(37, 25, 89);
      lels[PixelPosition(5, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(5, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(6, 5)] = CRGB(37, 25, 89);
      lels[PixelPosition(6, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(6, 7)] = CRGB(37, 25, 89);
      lels[PixelPosition(7, 6)] = CRGB(37, 25, 89);
      lels[PixelPosition(1, 3)] = CRGB(255, 255, 255);
      lels[PixelPosition(0, 0)] = CRGB(255, 255, 255);
      lels[PixelPosition(7, 4)] = CRGB(255, 255, 255);
      Serial.println("HI");
      lels[PixelPosition(3, 2)] = CRGB(255, 255, 255);
      lels[PixelPosition(5, 3)] = CRGB(255, 255, 255);
      FastLED.show();
      break;
  }
}
