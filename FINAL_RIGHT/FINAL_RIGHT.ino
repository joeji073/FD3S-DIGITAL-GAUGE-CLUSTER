#include <GD23ZUTX.h>
#include <SPI.h>
#include <IFCT.h>
#include <AnalogSmooth.h>
#include "rightassets.h"
#include <TeensyThreads.h>
#include <EEPROM.h>
#include <EasyButton.h>
#include "FastLED.h"
#include "I2CTransfer.h"

I2CTransfer myTransfer;

struct STRUCT {
      uint8_t CTPS, CBARO, CAFR, CMAT, CCLT, CBATT, CEGOCOR, CTOTCOR, 
      COILTEMP, CCOOLPRESS, COILP, CFUELP, CETHL, CMPG, CFUELTEMP, CIGNADV, CGEAR, CVSS, CTRAC, CAFRTAR;
      uint16_t CRPM, CKPA;
} canData;

/////////////////////////////////////////////////////////////////// Callbacks
void hi()
{
  myTransfer.rxObj(canData);
 
  //Serial.println(testStruct.y);
}

// supplied as a reference - persistent allocation required
const functionPtr callbackArr[] = { hi };
///////////////////////////////////////////////////////////////////

//int MAT,BOOST,CLT,VSS,ETHL,OILP,OILT,FUELP,FUELT,BATT,
//    COOLP,EGO,TOTCOR,BOOSTTAR,AFR,BARO,TPS,MPG,STATUS,RPM;

uint32_t startFPS=0, endFPS=0, drawT=0;   //used for loops/sec counter

const int SHTLED = 36;
const int buttonPin = 33;  
const int buttonPin2 = 17; 
const int headlightPin = 34;
const int wire = 32;
const int poten = 38;
//const int CANafr = 22;
//const int CANboost = 23;
const int seatbeltPin = 15;
const int brakelightPin = 39;
//const int highbeamPin = 18;

int shiftlightVal = 7500;
int cltlightVal = 205;
int matlightVal = 180;

int GREY = 0x242424;
int BLCK = 0x000000;

int screenTrack = 0;
int pageTrack = 0;

float PSI; 
float peakBoost = 0;

float val, GAFR;

int seatbeltLight, brakeLight, highbeamLight, lowgasLight;

EasyButton button(buttonPin);
EasyButton button2(buttonPin2);

//int buttonstate;
//int lastbuttonstate = LOW;
//long lastDebounceTime = 0;  // the last time the output pin was toggled
//long debounceDelay = 250;    // the debounce time; increase if the output flickers
//unsigned long previousMillis = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NUM_LEDS 1
#define DATA_PIN 29
CRGB leds[NUM_LEDS];

unsigned long RedledpreviousMillis = 0; 
const long Redledinterval = 125;           // interval at which to blink (milliseconds)
bool RedledState = false;

unsigned long OrgledpreviousMillis = 0; 
const long Orgledinterval = 500;           // interval at which to blink (milliseconds)
bool OrgledState = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {

 // Can0.setBaudRate(500000);
//  CAN_filter_t allPassFilter;
//  allPassFilter.id=0;
//  allPassFilter.ext=1;
//  allPassFilter.rtr=0;
//    for (uint8_t filterNum = 4; filterNum < 16;filterNum++)
 //     {
 //       CANbus.setFilter(allPassFilter,filterNum); 
 //     }

   Wire.begin(0);
  ///////////////////////////////////////////////////////////////// Config Parameters
  configST myConfig;
  myConfig.debug        = true;
  myConfig.callbacks    = callbackArr;
  myConfig.callbacksLen = sizeof(callbackArr) / sizeof(functionPtr);
  /////////////////////////////////////////////////////////////////
  
  myTransfer.begin(Wire, myConfig);

  button.begin();
  button2.begin();
  button.onPressedFor(1000, onPressedForDuration);
  button.onPressed(onPressed);
  button2.onPressedFor(1000, onButton2longPressed);
  button2.onPressed(onButton2Pressed);
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  GD.begin();
  LOAD_ASSETS();
  
  pinMode(SHTLED, OUTPUT);
 // pinMode(buttonpin, INPUT);
  pinMode(headlightPin, INPUT);
 // pinMode(wire, INPUT_PULLUP);   /////////Remove once final
 // pinMode(poten, INPUT); //////////
 // pinMode(CANafr, INPUT);
 // pinMode(CANboost, INPUT);
  pinMode(seatbeltPin, INPUT);
  pinMode(brakelightPin, INPUT);
  //pinMode(highbeamPin, INPUT);

  headlightDim();

  //shiftlightVal = EEPROM.read(0);
  //cltlightVal = EEPROM.read(4);
  //matlightVal = EEPROM.read(8);
  

  LOAD_ASSETS();

  GD.Clear();
  GD.BitmapHandle(2);
  GD.cmd_loadimage(ASSETS_END, 0);
  GD.load("RIGHTGAUGE11.jpg");

  delay(5400);     //10200ms delay 

}

void loop() {
 
while(screenTrack == 0){  

  reading();
  headlightDim();
  
      //canData();
      GD.Clear();
      GD.Begin(BITMAPS);
      GD.Vertex2ii(0, 0, 2);
      GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
      GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

      GD.swap();
      GD.finish();
     
      delay(1000);

       sequence();
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 1) {

  GAFR = ((float)canData.CAFR/10.0);
    
    if (GAFR > 19) {
      GAFR = 19;
    }

  headlightDim();
  reading();
  
  button.read();
  button2.read();
  
  GD.Clear();
          
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, 2);
  
            //High Beam warning light
      GD.Begin(RECTS);          
      GD.ColorRGB(0x000000);
      GD.Vertex2ii(0, 335);
      GD.Vertex2ii(67, 375);

    if (seatbeltLight == HIGH) {             //Seat belt warning light
      GD.Begin(RECTS);          
      GD.ColorRGB(0x000000);
      GD.Vertex2ii(69, 335);
      GD.Vertex2ii(112, 375);
    }


    if (brakeLight == HIGH) {             //Brake warning light
      GD.Begin(RECTS);          
      GD.ColorRGB(0x000000);
      GD.Vertex2ii(115, 330);
      GD.Vertex2ii(160, 380);
    }
  
  GD.ColorRGB(0xff0000);
  GD.cmd_gauge(158,221,125,12544,9,5,(map(canData.CKPA, 0, 306,1170, 2170)),1000); //boost gauge needle
  GD.cmd_gauge(335,388,125,12544,9,5,(map(GAFR, 10, 19,1170, 2170)),1000); //afr gauge needle
  
  GD.ColorRGB(0xFFFFFF);
  
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

  if (pageTrack == 0){
    GD.ColorRGB(0xfaf9ff);
    GD.printNfloat(380 , 180, PSI, 0, NUMBERFONT_HANDLE); //BOOST numbers
    GD.Begin(RECTS);          
    GD.ColorRGB(0x000000);
    GD.Vertex2ii(355, 248);
    GD.Vertex2ii(410, 280);
    }

  else if (pageTrack == 1){
    GD.ColorRGB(0xFFA500);
    GD.printNfloat(380 , 180, peakBoost, 0, NUMBERFONT_HANDLE); //Peak BOOST numbers
    }
  
  GD.ColorRGB(0xfaf9ff);
  GD.printNfloat(170 , 410, GAFR, 1, NUMBERFONT_HANDLE); //AFR numbers

  //Serial.println(reads);

  GD.swap();
  GD.finish();

  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 2) {

  GD.Clear();

  button.read();

  headlightDim();
        
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, 2);
  
  if (pageTrack == 0){
    button2.read();
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 240, NUMBERFONT_HANDLE, OPT_CENTER, "SHIFT LIGHT");
    GD.ColorRGB(0x262626);
    GD.cmd_text(242, 310, NUMBERFONT_HANDLE, OPT_CENTER, "CLT WARNING");
    GD.cmd_text(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, "MAT WARNING");
  }

  else if (pageTrack == 1){
    button2.read();
    GD.ColorRGB(0x262626);
    GD.cmd_text(242, 240, NUMBERFONT_HANDLE, OPT_CENTER, "SHIFT LIGHT");
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 310, NUMBERFONT_HANDLE, OPT_CENTER, "CLT WARNING");
    GD.ColorRGB(0x262626);
    GD.cmd_text(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, "MAT WARNING");
  }

  else if (pageTrack == 2){
    button2.read();
    GD.ColorRGB(0x262626);
    GD.cmd_text(242, 240, NUMBERFONT_HANDLE, OPT_CENTER, "SHIFT LIGHT");
    GD.cmd_text(242, 310, NUMBERFONT_HANDLE, OPT_CENTER, "CLT WARNING");
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, "MAT WARNING");

  }

  else if (pageTrack == 3){
    button2.read();
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 300, NUMBERFONT_HANDLE, OPT_CENTER, "SHIFT LIGHT");
    GD.cmd_number(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, shiftlightVal);

  }

  else if (pageTrack == 4){
    button2.read();
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 300, NUMBERFONT_HANDLE, OPT_CENTER, "CLT WARNING");
    GD.cmd_number(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, cltlightVal);

  }

  else if (pageTrack == 5){
    button2.read();
    GD.ColorRGB(0xffffff);
    GD.cmd_text(242, 300, NUMBERFONT_HANDLE, OPT_CENTER, "MAT WARNING");
    GD.cmd_number(242, 380, NUMBERFONT_HANDLE, OPT_CENTER, matlightVal);

  }

  GD.swap();
  GD.finish();
     
  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reading() {

  //RPM = CRPM/10;  //Used to trigger shift light
  //VSS = CVSS/10;  //Can probably remove once final
  //AFR = CAFR/10;
 // TPS = CTPS/10;  
  //KPA = CKPA/10;
 // BARO = CBARO/10;

  //KPA = (map(reads, 2, 2046,0, 306.843));
  //RPM = (map(reads, 2, 2046,0, 9000));

  if (canData.CKPA > canData.CBARO) {
    PSI = (canData.CKPA-canData.CBARO)*.145, 1;
  }

  if (canData.CKPA <= canData.CBARO) {
    PSI = (canData.CKPA-canData.CBARO)/3.38, 0;
  }

  if (PSI > peakBoost){
    peakBoost = PSI;
  }
  
  if (GAFR > 12 && canData.CRPM > 4000 && canData.CTPS > 97) {
    RedledWarning();
  }

  else if (canData.COILP < 15 && canData.CRPM > 1000) {
    RedledWarning();
  }

  else if (canData.COILP < 50 && canData.CRPM > 4000) {
    RedledWarning();
  }

  else if (canData.CFUELP < 40 && canData.CRPM > 4000) {
    RedledWarning();
  }

  else if (canData.CBATT < 12 && canData.CRPM > 1000) {
    OrgledWarning();
  }

  else if (canData.CCLT >= cltlightVal){
    OrgledWarning();
  }

  else if (canData.CMAT >= matlightVal){
    OrgledWarning(); 
  }

  

  else {
    leds[0] = CRGB::Black;
    FastLED.show();
  }

  if (canData.CRPM >= shiftlightVal){
    leds[0] = CRGB::White;
    FastLED.show();
  }





  seatbeltLight = digitalRead(seatbeltPin);   //Readings for warning lights
  brakeLight = digitalRead(brakelightPin);
//  highbeamLight = digitalRead(highbeamPin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sequence() {                      ///Gauge Sequence Code
    int seqdelay = 120;     //This delay needs tweeking
    
    GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE10.jpg");

    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE9.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE8.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE7.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE6.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE5.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE4.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

        GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE3.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

            GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE2.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

            GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE1.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();

            GD.Clear();
    GD.BitmapHandle(2);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("RIGHTGAUGE.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 0, 2);
    GD.ColorRGB(0xff0000);
      GD.cmd_gauge(158,221,125,12544,9,5,1170,1000); //boost gauge needle
      GD.cmd_gauge(335,388,125,12544,9,5,1170,1000); //afr gauge needle

  GD.ColorRGB(0xFFFFFF);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((158-22), (221-22), 1); // Needle center asset, the -22 is half of the image width
  GD.Vertex2ii((335-22), (388-22), 1); // Needle center asset, the -22 is half of the image width

    GD.swap();
        
    screenTrack = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void headlightDim () {

  if (digitalRead(headlightPin) == HIGH){
    GD.wr(REG_PWM_DUTY, 25);
  }

  else if (digitalRead(headlightPin) == LOW){
    GD.wr(REG_PWM_DUTY, 128);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void onPressed()
{
  pageTrack = 0;
 // Serial.print("screenTrack = ");
 // Serial.println(screenTrack);
    if (screenTrack == 1){
      GD.Clear();
      GD.BitmapHandle(2);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("BLACK.jpg");
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
    }

    else if (screenTrack == 2){
      GD.Clear();
      GD.BitmapHandle(2);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("RIGHTGAUGE.jpg");
      screenTrack = 1;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
    }

}

void onPressedForDuration()
{
   
  if (screenTrack == 1 && pageTrack == 0){
    pageTrack = 1;  
    }

  else if (screenTrack == 1 && pageTrack == 1){
    pageTrack = 0;  
    }

  else if (screenTrack == 2 && pageTrack == 0){
    pageTrack = 1;  
    }

  else if (screenTrack == 2 && pageTrack == 1){
    pageTrack = 2;  
    }

  else if (screenTrack == 2 && pageTrack == 2){
    pageTrack = 0;  
    }
    
  Serial.print("pageTrack = ");
  Serial.println(pageTrack);
}

void onButton2longPressed(){

  if (screenTrack == 1 && pageTrack == 1){
    peakBoost = 0;
    }

  else if (screenTrack == 2 && pageTrack == 0){
    pageTrack = 3;  
    }

  else if (screenTrack == 2 && pageTrack == 1){
    pageTrack = 4;  
    }

  else if (screenTrack == 2 && pageTrack == 2){
    pageTrack = 5;  
    }


  else if (screenTrack == 2 && pageTrack == 3){
    EEPROM.update(0,shiftlightVal);
    pageTrack = 0;  
    }

  else if (screenTrack == 2 && pageTrack == 4){
    EEPROM.update(4,cltlightVal);
    pageTrack = 1;  
    }

  else if (screenTrack == 2 && pageTrack == 5){
    EEPROM.update(8,matlightVal);
    pageTrack = 2;  
    }

}

void onButton2Pressed(){

  if (screenTrack == 2 && pageTrack == 3){
    shiftlightVal = shiftlightVal + 100;
    
    if (shiftlightVal > 8500){
      shiftlightVal = 7500;
      }
  }

  else if (screenTrack == 2 && pageTrack == 4){
    cltlightVal = cltlightVal + 1;

    if (cltlightVal > 220){
      cltlightVal = 205;
      }
  }

  else if (screenTrack == 2 && pageTrack == 5){
    matlightVal = matlightVal + 1;

    if (matlightVal > 170){
      matlightVal = 180;
      }
  }

}

/////////////////////////////////////////////////////////////////

void RedledWarning() {

  unsigned long RedledcurrentMillis = millis();

  if (RedledcurrentMillis - RedledpreviousMillis >= Redledinterval) {
    // save the last time you blinked the LED
    RedledpreviousMillis = RedledcurrentMillis;

  if(RedledState == false) {
        RedledState = true;
        leds[0] = CRGB::Red;
        FastLED.show();
    } else {
        RedledState = false;
        leds[0] = CRGB::Black;
        FastLED.show();
    }

}
}

void OrgledWarning() {

  unsigned long OrgledcurrentMillis = millis();

  if (OrgledcurrentMillis - OrgledpreviousMillis >= Orgledinterval) {
    // save the last time you blinked the LED
    OrgledpreviousMillis = OrgledcurrentMillis;

  if(OrgledState == false) {
        OrgledState = true;
        leds[0] = CRGB::OrangeRed;
        FastLED.show();
    } else {
        OrgledState = false;
        leds[0] = CRGB::Black;
        FastLED.show();
    }

}
}
