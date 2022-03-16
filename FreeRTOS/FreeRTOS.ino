#include "time.h"
#include <WiFi.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontRobotron.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define NUM_LEDS 256
#define GAS_ANALOG 4

#define LED_PIN        12
#define COLOR_ORDER    RGB
#define CHIPSET        WS2812B

#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT  -8
#define MATRIX_TYPE    VERTICAL_ZIGZAG_MATRIX // Wie sind die LEDs angeordnet

const char* ssid       = "sheeshtof";
const char* password   = "asdasdasd";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

int timeDate = 0;
long int letzteZeit = 0;
int zahl = 48;

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
  timeDate = timeinfo.tm_sec;
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(timeDate);
}

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskAnalogReadA3( void *pvParameters );
void TaskGetTime( void *pvParameters );
unsigned char TxtDemo[] = { EFFECT_HSV_CV "\x00\xff\xff\x50\xff\xff" "    KRISTOF"};
// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);

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

  
  /*
    ScrollingMsg.SetFont(RobotronFontData);
    ScrollingMsg.Init(&leds, leds.Width(), leds.Height() + 1, 0, 0);
    ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
    ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0xff);
  */
}

void loop()
{
  // Empty. Things are done in Tasks.
}
/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  FastLED.addLeds<NEOPIXEL, LED_PIN>(lels, NUM_LEDS);
  FastLED.setBrightness(255);

  for (;;) {

    if(millis()>=letzteZeit+2500)
    {
      zahl++;if(zahl>57) zahl=48;
      TxtDemo[15]=zahl;
      ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
      letzteZeit = millis();
    }
    
    if (ScrollingMsg.UpdateText() == -1){
    ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
    vTaskDelay(500);
    }
    else
      FastLED.show();
    vTaskDelay(20);
  }
}
void TaskAnalogReadA3(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  /*
    AnalogReadSerial
    Reads an analog input on pin A3, prints the result to the serial monitor.
    Graphical representation is available using serial plotter (Tools > Serial Plotter menu)
    Attach the center pin of a potentiometer to pin A3, and the outside pins to +5V and ground.

    This example code is in the public domain.
  */

  for (;;)
  {
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
    vTaskDelay(1000);
    printLocalTime();
  }
}
