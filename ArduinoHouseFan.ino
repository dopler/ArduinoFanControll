#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams.
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 **************************************************************************/

#include <Adafruit_GFX.h>               // Core graphics library
#include <Adafruit_ST7735.h>            // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h>            // Hardware-specific library for ST7789
#include <SPI.h>

#ifdef ADAFRUIT_HALLOWING
  #define TFT_CS        39              // Hallowing display control pins: chip select
  #define TFT_RST       37              // Display reset
  #define TFT_DC        38              // Display data/command select
  #define TFT_BACKLIGHT  7              // Display backlight pin
#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5
#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        10
  #define TFT_RST        8              // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         9
#endif

/*
ST7735_BLACK      ST77XX_BLACK
ST7735_WHITE      ST77XX_WHITE
ST7735_RED        ST77XX_RED
ST7735_GREEN      ST77XX_GREEN
ST7735_BLUE       ST77XX_BLUE
ST7735_CYAN       ST77XX_CYAN
ST7735_MAGENTA    ST77XX_MAGENTA
ST7735_YELLOW     ST77XX_YELLOW
ST7735_ORANGE     ST77XX_ORANGE
*/

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. For Arduino Uno: MOSI = pin 11 and
// SCLK = pin 13. This is the fastest mode of operation and is required if
// using the breakout board's microSD card.

// For 1.44" and 1.8" TFT with ST7735 (including HalloWing) use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// For 1.54" TFT with ST7789:
//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// OPTION 2 lets you interface the display using ANY TWO or THREE PINS,
// tradeoff being that performance is not as fast as hardware SPI above.
//#define TFT_MOSI 11  // Data out
//#define TFT_SCLK 13  // Clock out
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

float p = 3.1415926;

//-----------------------------------PIN AND VRIABLE DECLARATION------------------------------------------------------
const int selectButton = 2;             //button select length of time
const int fanButton = 3;                //button select which fans are on
const int startButton = 4;              //button to start the timer and activate fans
const int conOne = 6;                   //control pin 1 for first relay, active high
const int conTwo = 7;                   //control pin 2 for second relay, active high
volatile int selection;                 //variable that holds time selection
volatile int fanSelection = 0;          //variable that holds current fan selection option
int timeSelection[] = {8, 6, 4, 2, 1};  //the amount of time selected
int timePos [] = {40, 60, 80, 100, 120};//the position of the filled in circle

int buttonState;                        // the current reading from the input pin
int lastButtonState = HIGH;             // the previous reading from the input pin

//Timer Variables
bool timeToggle = false;                //controls timer state
int timeVal [] = {8, 6, 4, 2, 1};
int currMin = -1;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;     // the last time the output pin was toggled
unsigned long debounceDelay = 50;       // the debounce time; increase if the output flickers

//-----------------------------------PIN AND VARIABLE SETUP-----------------------------------------------------------
void setup(void) 
{
  Serial.begin(9600);
  Serial.print(F("Hello! ST77xx TFT Test"));

#ifdef ADAFRUIT_HALLOWING
  // HalloWing is a special case. It uses a ST7735R display just like the
  // breakout board, but the orientation and backlight control are different.
  tft.initR(INITR_HALLOWING);        // Initialize HalloWing-oriented screen
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH); // Backlight on
#else
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

  // OR use this initializer (uncomment) if using a 1.44" TFT:
  //tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab

  // OR use this initializer (uncomment) if using a 0.96" 180x60 TFT:
  //tft.initR(INITR_MINI160x80);  // Init ST7735S mini display

  // OR use this initializer (uncomment) if using a 1.54" 240x240 TFT:
  //tft.init(240, 240);           // Init ST7789 240x240
#endif

  pinMode(selectButton, INPUT_PULLUP);
  pinMode(fanButton, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);
  pinMode(conOne, OUTPUT);
  digitalWrite(conOne, LOW);
  pinMode(conTwo, OUTPUT);
  digitalWrite(conTwo, LOW);
  attachInterrupt(digitalPinToInterrupt(selectButton), InterruptTime, FALLING);
  attachInterrupt(digitalPinToInterrupt(fanButton), InterruptFanSelection, FALLING);
  Serial.println(F("Initialized"));
  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;
  Serial.println(time, DEC);
  delay(500);
  
  // Setup UI
  selection = 0;
  DrawUI();
  DrawFanStatus(fanSelection);
  DrawTimeSelection();
  setTime(0,0,0,1,1,11);
  currMin = -1;
  drawText("START", ST77XX_GREEN, 65, 130);
}

//-----------------------------------MAIN LOOP------------------------------------------------------------------------
void loop() 
{
  //any input after time has started will stop and reset the timer
  int reading = digitalRead(startButton);
  DebounceButton(reading);
  if(timeToggle)// if timer active
  {
    DrawTime();
    if(timeVal[selection] <= hour())
    {
      // time has been reached, stop functions
      //Serial.println("stop now");
      StopTime();         //stop and reset the timer
    }
  }
  Alarm.delay(100);       //use alarm.delay instead of delay() for timer purposes
}

//-------------------------------START AND STOP TIMER (MAIN FUNCTION)------------------------------------------------
void DebounceButton(int bVal)
{
  if (bVal != lastButtonState) 
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) 
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // if the button state has changed:
    if (bVal != buttonState) 
    {
      buttonState = bVal;
      // only toggle the LED if the new button state is HIGH
      if (bVal == LOW) 
      {
        if(timeToggle == false)
        {
          StartFanTimer();
        }
        else
        {
          StopTime();
        }
      }
    }
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = bVal;
}

void StartFanTimer()
{
  //fan timer will not start unless there is a fan that will turn on
  if(fanSelection != 0)
  {
    setTime(0,0,0,1,1,11);      // resets time to 0
    timeToggle = true;          //activates timer in main loop()
    currMin = -1;               //allows initial drawing of time (00:00)
    ActivateRelay();
  }
}

void StopTime()
{
  timeToggle = false;                               //toggle timer off
  setTime(0,0,0,1,1,11);                            //reset timer
  currMin = -1;                                     //reset to allow initial draw again (0:00)
  tft.fillRect(65, 130, 70, 20, ST77XX_BLACK);      //overwrite drawn timer
  drawText("START", ST77XX_GREEN, 65, 130);         //draw "Start"
  DeactivateRelay();
  //----------------------------------- TO DO-------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
  //----------------------------DEACTIVATE RELAYS---------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
}

//-------------------------------DRAW BASIC UI THAT DOES NOT CHANGE VIA INPUT----------------------------------------
void DrawUI()
{
  tft.fillScreen(ST77XX_BLACK);                     //blank out screen
  //DRAW TEXT ELEMENTS--------------------------------------------
  tft.setTextSize(2);                               //set text size for labels
  drawText("Time", ST77XX_WHITE, 5, 10);            //draw "time" and both "fan" labels
  drawText("FAN 1", ST77XX_WHITE, 70, 10);
  drawText("FAN 2", ST77XX_WHITE, 70, 60);
  
  //DRAW EMPTY CIRCLES--------------------------------------------
  for(int i = 0; i < 5; i++)
  {
    //tft.drawCircle(X pos, Y pos, radius, color);
    tft.drawCircle(10, timePos[i],5,ST77XX_WHITE);
  }
  //DRAW CIRCLE LABELS--------------------------------------------
  tft.setTextSize(1);
  drawText("8hrs", ST77XX_WHITE, 20, 38);
  drawText("6hrs", ST77XX_WHITE, 20, 58);
  drawText("4hrs", ST77XX_WHITE, 20, 78);
  drawText("2hrs", ST77XX_WHITE, 20, 98);
  drawText("1hrs", ST77XX_WHITE, 20, 118);
}

void drawText(char * text, uint16_t color, int xPos, int yPos)
{
  tft.setCursor(xPos, yPos);
  tft.setTextColor(color);
  tft.setTextWrap(false);
  tft.print(text);
}

//-------------------------FAN FUNCTIONS FOR CHANGING OPTIONS, ACTIVATING, AND DRAWING TO UI---------------------------
void ChangeFanStatus()
{
  fanSelection = (fanSelection + 1) % 4;
  DrawFanStatus(fanSelection);
  StopTime();
}

void DrawFanStatus(int setting)
{
  tft.fillRect(70, 30, 70, 20, ST77XX_BLACK);
  tft.fillRect(70, 80, 70, 20, ST77XX_BLACK);
  tft.setTextSize(2);
  switch(setting)
  {
    case 1:
      drawText("ON", ST77XX_GREEN, 70, 30);
      drawText("OFF", ST77XX_RED, 70, 80);
      break;
    case 2:
      drawText("OFF", ST77XX_RED, 70, 30);
      drawText("ON", ST77XX_GREEN, 70, 80);
      break;
    case 3:
      drawText("ON", ST77XX_GREEN, 70, 30);
      drawText("ON", ST77XX_GREEN, 70, 80);
      break;
    default:
      drawText("OFF", ST77XX_RED, 70, 30);
      drawText("OFF", ST77XX_RED, 70, 80);
      break;
  }
}

void IncrementSelection()
{
  selection = (selection + 1) % 5;
  DrawTimeSelection();
  StopTime();
}

void InterruptFanSelection()
{
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 200)
 {
   ChangeFanStatus();
 }
 last_interrupt_time = interrupt_time;
}

void ActivateRelay()
{
  if(fanSelection == 1 || fanSelection == 3)
  {
    digitalWrite(conOne, HIGH);
  }
  if(fanSelection == 2 || fanSelection == 3)
  {
    digitalWrite(conTwo, HIGH);
  }
}

void DeactivateRelay()
{
  digitalWrite(conOne, LOW);
  digitalWrite(conTwo, LOW);
}

//-------------------------------TIME SELECTION FUNCTIONS FOR CHANGING OPTIONS AND DRAWING TO UI--------------------------------
void DrawTimeSelection()
{
  for(int i = 0; i < 5; i++)
  {
    tft.fillCircle(10, timePos[i], 3, ST77XX_BLACK);
  }
  tft.fillCircle(10, timePos[selection], 3, ST77XX_WHITE);
}

void InterruptTime()
{
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 200)
 {
   IncrementSelection();
 }
 last_interrupt_time = interrupt_time;
}

//-------------------------------DRAW TIME UI-----------------------------------------------------------------------------------
void DrawTime()
{
  // IF THE CURRENT MINUTE HASNT CHANGED, DONT REDRAW.  PREVENTS FLICKERING WITH DELAYS
  if(currMin == minute())
  {
    return;
  }
  //REDRAW THE TIMER WITH THE UPDATED TIME.
  //overwrite with black
  tft.fillRect(65, 130, 70, 20, ST77XX_BLACK);
  tft.setTextSize(2);                     //set text size for timer
  
  //FORMAT AND CONCATENATE TIME------------------------------------
  char hr[32];
  char mn[16];
  char sc[16];
  itoa(hour(), hr, 10);
  itoa(minute(), mn, 10);
  itoa(second(), sc, 10);
  strcat(hr,":");
  if(minute() < 10)                       //if the minute is less than 10 add a leading '0' to minutes column
  {
    strcat(hr,"0");
  }
  drawText(strcat(hr,mn), ST77XX_GREEN, 65, 130);       //draw the new updated time
  currMin = minute();                                   //update the saved time to stop flickering
}
