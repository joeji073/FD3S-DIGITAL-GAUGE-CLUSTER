#include <GD23ZUTX.h>
#include <SPI.h>
#include <IFCT.h>
#include <AnalogSmooth.h>
#include "gaugeassets2.h"
#include <TeensyThreads.h>
#include <EEPROM.h>
#include <EasyButton.h>
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
  Serial.println(canData.CTPS);
  //Serial.println(testStruct.y);
}

// supplied as a reference - persistent allocation required
const functionPtr callbackArr[] = { hi };
///////////////////////////////////////////////////////////////////

uint32_t startFPS=0, endFPS=0, drawT=0;   //used for loops/sec counter

const int buttonPin = 33;
const int buttonPin2 = 17;
const int resetbuttonPin = 17;
const int headlightPin = 34;
//const int poten = 38;
const int backLight = 37;
const int backLight2 = 32;
const int lowfuelPin = 38;


int lowfuelLight;

float PSI;

int GREY = 0x242424;
int BLCK = 0x000000;

int screenTrack = 0; //Change to 0 when final
int pageTrack = 0;

float turn;       //Used for RED gauge needle
float angle;      //Same as above

MoviePlayer mp;

/////////////////////////////////////////////////////////////

EasyButton button(buttonPin);
EasyButton button2(buttonPin2);
                                                           
//int buttonstate;                                           
//int lastbuttonstate = LOW;                                 
//long lastDebounceTime = 0;                                 //Button press Variables
//long debounceDelay = 250;                                  
//unsigned long previousMillis = 0;                          
                                                           
/////////////////////////////////////////////////////////////
                                                           
const unsigned long period = 50;                           
long startOdometer = 150000; 
long odometer;
long odometerAdd;                                  
float trip = 0.000000;                                    
float tripVal = 0.000000;                                  
int mph;                                                   
const float fps = (1.46667/(1000.00000/period));                 //Odometer Variables
float currentdist = 0.000000;                              
float distance = 0.000000;                                 
unsigned long ODOpreviousmillis = 0;                       
unsigned long ODOcurrentmillis;                            
                                                           
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int fuelnumReadings = 10;
int fuelreadings[fuelnumReadings];      
int fuelreadIndex = 0;              
int fueltotal = 0;                  
int fuelaverage = 0;                
int fueloldaverage;                                 //fuel level reading, change input pin to 35
int fuelReading;
const int fuelinputPin = 35;        
const long fuelreadInterval = 2000;
unsigned long fuelpreviousMillis = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(backLight, OUTPUT);
  pinMode(backLight2, OUTPUT);
  digitalWrite(backLight, LOW);
  digitalWrite(backLight2, LOW);

   Wire.begin(0);
  ///////////////////////////////////////////////////////////////// Config Parameters
  configST myConfig;
  myConfig.debug        = true;
  myConfig.callbacks    = callbackArr;
  myConfig.callbacksLen = sizeof(callbackArr) / sizeof(functionPtr);
  /////////////////////////////////////////////////////////////////
  
  myTransfer.begin(Wire, myConfig);
 
  Serial.begin(9600);
  
  button.begin();
  button2.begin();
  button.onPressedFor(1000, onPressedForDuration);
  button.onPressed(onPressed);
  button2.onPressedFor(1000, onButton2longPressed);

  GD.begin(~GD_CALIBRATE);
  LOAD_ASSETS();
  
  pinMode(headlightPin, INPUT);

  pinMode(lowfuelPin, INPUT);

  odometerAdd = EEPROM.read(0);  //uncomment when ready to use odometer

  delay(100);
  digitalWrite(backLight, HIGH);
  digitalWrite(backLight2, HIGH);
  delay(100);

  headlightDim();

  mp.begin("comp.avi");
  mp.play();

  GD.swap();

  LOAD_ASSETS();
  bitmaps.redneedle.center.x = 240;  // This moves the X axis for the needle in RED mode

  GD.Clear();
  GD.BitmapHandle(5);
  GD.cmd_loadimage(ASSETS_END, 0);
  GD.load("CENTER.jpg");

}

void loop() {

while(screenTrack == 0){  
 
  headlightDim();
  reading();
  
      GD.Clear();
      GD.Begin(BITMAPS);
      GD.Vertex2ii(5,85, 5);
      GD.ColorRGB(0xff0000);
      GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset

      GD.swap();
      GD.finish();

      delay(1000);
     
       sequence();
    }


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 1) {

  startFPS = millis();

  reading();
  odoReading();
  headlightDim();

  GD.Clear();

  button.read();
        
  GD.Begin(BITMAPS);      //begin background image
  GD.Vertex2ii(0, 5, 5);  //This draws the image (x,y,image handle)

  if (canData.CTRAC == 0) {             //Traction control light, replace with, if(traction retard is above 1 degree)
    GD.Begin(RECTS);          //Draws a black rectangle over background
    GD.ColorRGB(0x000000);    //
    GD.Vertex2ii(160, 300);   //
    GD.Vertex2ii(310, 350);   //
  }

  if (lowfuelLight == LOW){
    GD.Begin(RECTS);          //Low fuel light....uncomment when done
    GD.ColorRGB(0x000000);    //Change to black when done
    GD.Vertex2f(280*10, 750*10);   //
    GD.Vertex2f(360*10, 840*10);   //    
  }


  GD.ColorRGB(0xff0000);
  GD.cmd_gauge(242,422,270,12544,9,5,(map(canData.CRPM, 0, 9000,1170, 2170)),1000); //RPM gauge needle
  GD.cmd_gauge(350,655,110,12544,9,5,(map(fuelReading, 0, 100,250, 600)),1000); //Gas level needle
  
  GD.ColorRGB(0xffffff);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset

  GD.ColorRGB(0xfaf9ff); //this is a slight blue white since the led temperature doesn't match the smaller LCD's
  GD.printNfloat(390, 455, canData.CVSS, 0, 0); //Speedo numbers, the last number is the handle...See top of gaugeassets.h

  if (pageTrack == 0){
    GD.printNfloat(370, 535, odometer, 0,1); //Odometer reading
  }

  else if (pageTrack == 1){
    GD.printNfloat(347, 535, tripVal, 1,1); //Trip reading
  }

  
  GD.finish();
  GD.swap();

  endFPS = millis();
  drawT = 1000/(endFPS-startFPS);

  Serial.println(fuelinputPin);
  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 2) {

  GD.Clear();

  reading();
  odoReading();
  headlightDim();

  button.read();
        
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 5, 5);
/*
    if (lowfuelLight == HIGH){
    GD.Begin(RECTS);          //Low fuel light....uncomment when done
    GD.ColorRGB(0xFFFFFF);    //
    GD.Vertex2ii(160, 360);   //
    GD.Vertex2ii(310, 370);   //    
  }
*/

    int fuelboxlevel = map(fuelReading, 0, 100, 749, 825);
    
    GD.Begin(RECTS);          //FUEL LEVEL BOX
    GD.ColorRGB(0xFF0000);    //
    GD.Vertex2f(695*10, fuelboxlevel*10);   //
    GD.Vertex2f(800*10, 826*10);   //

  GD.ColorRGB(0xffffff);
  GD.printNfloat(410, 340, canData.CVSS, 0, 0);   //Speedo numbers
  GD.printNfloat(345, 490, canData.CTPS, 0,1); //TPS reading
  GD.printNfloat(225, 470, canData.CGEAR, 0, 0);   //Gear numbers

  if (pageTrack == 0){
    GD.printNfloat(400, 430, odometer, 0,1); //Odometer reading
  }

  else if (pageTrack == 1){
    GD.printNfloat(400, 430, tripVal, 1,1); //Trip reading
  }

  GD.ColorRGB(0xff0000);
  GD.cmd_gauge(242,422,270,12544,9,5,(map(canData.CRPM, 0, 9000,997, 1997)),1000); //RPM gauge needle

  GD.ColorRGB(0xffffff);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
  
  GD.swap();
  GD.finish();
     
  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 3) {

  GD.Clear();
 
  reading();
  odoReading();
  headlightDim();

  button.read();
       
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 5, 5);

  GD.ColorRGB(0xffffff);
  draw_speedo(GD.w / 2, 418, angle);
  angle = DEGREES(turn);
  turn = (map(canData.CRPM, 0, 9000,-45, 225));

  GD.ColorRGB(0xffffff);
  GD.cmd_number(230, 375, NUMBERFONT_HANDLE, OPT_CENTER, canData.CVSS);   //VSS
  GD.cmd_number(230, 455, NUMBERFONT_HANDLE, OPT_CENTER, PSI);

  if (pageTrack == 0){
    GD.printNfloat(275, 515, odometer, 0,1); //Odometer reading
  }

  else if (pageTrack == 1){
    GD.printNfloat(252, 510, tripVal, 1,1); //Trip reading
  }
        
  GD.ColorRGB(0xffffff);
  GD.cmd_gauge(242,675,150,12544,9,5,(map(fuelReading, 0, 100,570, 1400)),2000); //gas gauge needle
        
  GD.swap();
  GD.finish();
     
}

while(screenTrack == 4) {

  startFPS = millis();
  
  reading();
  odoReading();
   
  if (digitalRead(headlightPin) == HIGH){
    GD.wr(REG_PWM_DUTY, 0);
  }

  else if (digitalRead(headlightPin) == LOW){
    GD.wr(REG_PWM_DUTY, 128);
  }


  GD.Clear();

  button.read();
        
  GD.Begin(BITMAPS);      //begin background image
  GD.Vertex2ii(0, 5, 5);  //This draws the image (x,y,image handle)

  GD.ColorRGB(0xff0000);
  GD.cmd_gauge(242,422,270,12544,9,5,(map(canData.CRPM, 0, 9000,1170, 2170)),1000); //RPM gauge needle
  GD.cmd_gauge(350,655,110,12544,9,5,(map(fuelReading, 0, 100,250, 600)),1000); //Gas level needle
  
  GD.ColorRGB(0xffffff);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset

  GD.ColorRGB(0xfaf9ff); //this is a slight blue white since the led temperature doesn't match the smaller LCD's
  GD.printNfloat(390, 455, canData.CVSS, 0, 0); //Speedo numbers, the last number is the handle...See top of gaugeassets.h

  if (pageTrack == 0){
    GD.printNfloat(370, 535, odometer, 0,1); //Odometer reading
  }

  else if (pageTrack == 1){
    GD.printNfloat(347, 535, tripVal, 1,1); //Trip reading
  }

  
  GD.finish();
  GD.swap();

  endFPS = millis();
  drawT = 1000/(endFPS-startFPS);
     
  
}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void reading() {

  if (canData.CKPA > canData.CBARO) {
    PSI = (canData.CKPA-canData.CBARO)*.145, 1;
  }

  if (canData.CKPA <= canData.CBARO) {
    PSI = (canData.CKPA-canData.CBARO)/3.38, 0;
  }
  
  fuelLevel();

  button2.read();

  lowfuelLight = digitalRead(lowfuelPin);

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sequence() {                      ///Gauge Sequence Code
    int seqdelay = 50;
    
    GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER1.jpg");

    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER2.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER2.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER3.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER4.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER5.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER6.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

        GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER7.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER8.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER9.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER10.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER11.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER12.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER13.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER14.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER15.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER16.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER17.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER18.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
    GD.swap();

            GD.Clear();
    GD.BitmapHandle(5);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("CENTER19.jpg");
    GD.Begin(BITMAPS);
    delay(seqdelay);

    GD.Vertex2ii(0, 5, 5);
    GD.ColorRGB(0xff0000);
    GD.cmd_gauge(242,422,260,12544,9,5,1170,1000);

      GD.ColorRGB(0xFFFFFF);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((242-35), (422-35), 2); // Needle center asset
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

void odoReading() {

  ODOcurrentmillis = millis();
  
  if (ODOcurrentmillis - ODOpreviousmillis >= period) {
  
  //sensorvalue = analogRead(sensorPin);
  mph = canData.CVSS;
  
  currentdist = mph * fps;
  
  distance = distance + currentdist; 
  trip = trip + currentdist;
  
  if (trip >= 528){
    tripVal = tripVal + 0.1;
    trip = 0;
  }
    
  if (distance >= 5280){  
    odometerAdd++;
    EEPROM.write(0, odometerAdd); //uncomment when ready to use odometer
    distance = 0;
  }

  odometer = startOdometer + odometerAdd;
  
  ODOpreviousmillis = ODOcurrentmillis;           
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void draw_speedo(int x, int y, int angle)   //This is for the Red gauge, inits and controls the RPM pointer.
{
  bitmaps.redneedle.draw(x, y, angle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void onPressed()
{
  pageTrack = 0;
 // Serial.print("screenTrack = ");
 // Serial.println(screenTrack);
    if (screenTrack == 1){
      GD.Clear();
      GD.BitmapHandle(5);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("FERRAIMODE.jpg");
      screenTrack++;
      //Serial.print("screenTrack = ");
      //Serial.println(screenTrack);
    }

    else if (screenTrack == 2){
      GD.Clear();
      GD.BitmapHandle(5);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("REDMODE.jpg");
      screenTrack++;
      //Serial.print("screenTrack = ");
      //Serial.println(screenTrack);
    }

    else if (screenTrack == 3){
      GD.Clear();
      GD.BitmapHandle(5);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("WHITE.jpg");
      screenTrack++;
      //Serial.print("screenTrack = ");
     // Serial.println(screenTrack);
    }

    else if (screenTrack == 4){
      GD.Clear();
      GD.BitmapHandle(5);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("CENTER19.jpg");
      screenTrack = 1;
     // Serial.print("screenTrack = ");
     // Serial.println(screenTrack);
    }
}

void onPressedForDuration()
{
  pageTrack++;
   
  if (pageTrack >= 2){
    pageTrack = 0;  
    }

 // Serial.print("pageTrack = ");
  //Serial.println(pageTrack);
}

void onButton2longPressed(){
  tripVal = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fastFuel(){
 
  fueltotal = fueltotal - fuelreadings[fuelreadIndex];
  fuelreadings[fuelreadIndex] = analogRead(fuelinputPin);
  fueltotal = fueltotal + fuelreadings[fuelreadIndex];
  fuelreadIndex = fuelreadIndex + 1;

  if (fuelreadIndex >= fuelnumReadings) {
    fuelreadIndex = 0;
    screenTrack = 1;
    }

  fuelaverage = fueltotal / fuelnumReadings;
  fueloldaverage = fuelaverage;
}

void fuelLevel(){

  unsigned long fuelcurrentMillis = millis();

  if (fuelcurrentMillis - fuelpreviousMillis >= fuelreadInterval) {
    fuelpreviousMillis = fuelcurrentMillis;
    fueltotal = fueltotal - fuelreadings[fuelreadIndex];
    fuelreadings[fuelreadIndex] = analogRead(fuelinputPin);
    fueltotal = fueltotal + fuelreadings[fuelreadIndex];
    fuelreadIndex = fuelreadIndex + 1;

    if (fuelreadIndex >= fuelnumReadings) {
      fuelreadIndex = 0;
    }

  fuelaverage = fueltotal / fuelnumReadings;

  if (fuelaverage < fueloldaverage){
    fueloldaverage = fuelaverage;
  }

  }

  fuelReading = map(fueloldaverage, 35, 1000, 0, 100);
  //Serial.print(fuelReading);
  //Serial.print("...");
  //Serial.print(fueloldaverage);
  //Serial.print("...");
  //Serial.println(fuelaverage);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
