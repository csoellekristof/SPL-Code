#include "time.h"
#include<FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontRobotron.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define GAS_ANALOG 4

#define LED_PIN        12
#define COLOR_ORDER    RGB
#define CHIPSET        WS2812B

#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT  -8
#define MATRIX_TYPE    VERTICAL_ZIGZAG_MATRIX // Wie sind die LEDs angeordnet

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

cLEDText ScrollingMsg;

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskAnalogReadA3( void *pvParameters );
const unsigned char TxtDemo[] = { EFFECT_HSV_CV "\x00\xff\xff\x50\xff\xff" "    KRISTOF"};
// the setup function runs once when you press reset or power the board
void setup() {
  
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    , 1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskAnalogReadA3
    ,  "AnalogReadA3"
    ,  2048  // Stack size
    ,  NULL
    ,  1 // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  FastLED.setBrightness(10); // WICHTIG - Hier wird die Helligkeit eingestellt. Am Anfang einen niedrigen Wert verwenden, und langsam hochtasten.
  FastLED.clear(true);

  ScrollingMsg.SetFont(RobotronFontData);
  ScrollingMsg.Init(&leds, leds.Width(), leds.Height() + 1, 0, 0);
  ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0xff);
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

  for(;;){
    if (ScrollingMsg.UpdateText() == -1){
    ScrollingMsg.SetText((unsigned char *)TxtDemo, sizeof(TxtDemo) - 1);
    delay(20);
  }
  else
    FastLED.show();
  delay(20);
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
    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}
