#include <GD23ZUTX.h>
#include <SPI.h>
#include <IFCT.h>
#include <AnalogSmooth.h>
#include "leftasset.h"
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

//static CAN_message_t rxmsg;

int MAT,BOOST,CLT,VSS,ETHL,OILP,OILT,FUELP,FUELT,BATT,
    COOLP,EGO,TOTCOR,BOOSTTAR,AFR,BARO,TPS,MPG,STATUS,RPM;

int GREY = 0x242424;
int BLACK = 0x000000;
int WHITE = 0xFFE6B5;
int RED = 0xFC3909;

//const int backLight = 37;     //Probably don't need...it's used on the Center display to control backlight
const int buttonPin = 33;     //Action button
const int buttonPin2 = 17;    //Reset button pin
const int headlightPin = 34;    //Dims backlight when headlights are turned on
//const int poten = 38;         //Not connected anymore, deleted when final   

//float TPS;        //used during testing, delete after
//float CTPS;

//float BARO, MAT, CLT, BATT, EGOCOR, TOTCOR, OILTEMP, FUELP, BOOSTTAR, ETHL, MPG, FUELTEMP, COOLPRESS, OILP, STATUS;
//float CBARO, CMAT, CCLT, CBATT, CEGOCOR, CTOTCOR, COILTEMP, CFUELP, CBOOSTTAR, CETHL, CMPG, CFUELTEMP, CCOOLPRESS, COILP;

int screenTrack = 0;
int pageTrack = 0;

int buttonstate;
int lastbuttonstate = LOW;
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 250;    // the debounce time; increase if the output flickers
unsigned long previousMillis = 0;

EasyButton button(buttonPin);
EasyButton button2(buttonPin2);


//////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  //pinMode(backLight, OUTPUT);        //Can delete, this is used on Center gauge to control backlight
  //digitalWrite(backLight, LOW);

  //Can0.setBaudRate(500000);

  Serial.begin(9600);

  Wire.begin(0);
  ///////////////////////////////////////////////////////////////// Config Parameters
  configST myConfig;
  myConfig.debug        = true;
  myConfig.callbacks    = callbackArr;
  myConfig.callbacksLen = sizeof(callbackArr) / sizeof(functionPtr);
  /////////////////////////////////////////////////////////////////
  
  myTransfer.begin(Wire, myConfig);

  button.begin();
  button.onPressedFor(1000, onPressedForDuration);
  button.onPressed(onPressed);

  GD.begin();
  LOAD_ASSETS();

  pinMode(headlightPin, INPUT);      

  headlightDim();   //read if headlight is already on 

  LOAD_ASSETS();

  GD.Clear();
  GD.BitmapHandle(3);
  GD.cmd_loadimage(ASSETS_END, 0);
  GD.load("a.jpg");

  delay(5300);    //change this delay to match the main gauge so the needles turn on at the same time

}

///////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

while(screenTrack == 0) {
    headlightDim();
    ////canData();
    reading();

      GD.Clear();
      GD.Begin(BITMAPS);
      GD.Vertex2ii(5,85,3);
      GD.ColorRGB(RED);
      GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);

      GD.ColorRGB(WHITE);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change

      GD.swap();
      GD.finish();
    

      delay(1000);
     
      sequence();

  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 1) {           //Analog Coolant temp gauge

  //canData();
  reading();
  headlightDim();

  int GCLT = canData.CCLT;

      if (GCLT < 100){
        GCLT = 100;
      }

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(RED);
  GD.cmd_gauge(120,197,130,12544,9,5,(map(GCLT, 100, 220,1170, 2170)),1000); //fuel pressure gauge needle

  GD.ColorRGB(WHITE);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change


  GD.ColorRGB(WHITE);
  GD.printNfloat(200, 216, canData.CCLT, 0, NUMBERFONT_HANDLE); //fuel pressure number
  
  GD.swap();
  GD.finish();


}

while(screenTrack == 100) {           //Digital fuel pressure

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CCLT);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 2) {           //Analog Fuel Temp gauge

  //canData();
  reading();
  headlightDim();

  int GFUELT = canData.CFUELTEMP;

    if (GFUELT < 40) {
      GFUELT = 40;
    }

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(RED);
  GD.cmd_gauge(120,197,130,12544,9,5,(map(GFUELT, 40, 160,1170, 2170)),1000); //main gauge needle

  GD.ColorRGB(WHITE);
  GD.Begin(BITMAPS);
  GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change

  GD.ColorRGB(WHITE);
  GD.printNfloat(200, 216, canData.CFUELTEMP, 0, 0); //Speedo numbers

  GD.swap();
  GD.finish();
}

  while(screenTrack == 200) {           //Digital Fuel Temp gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CFUELTEMP);

  GD.swap();
  GD.finish();

 

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 3) {           //Analog Oil Temp Gauge

  //canData();
  reading();
  headlightDim();

  int GOILT = canData.COILTEMP;

    if (GOILT < 100) {
      GOILT = 100;
    }

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(RED);
  GD.cmd_gauge(120,197,130,12544,9,5,(map(GOILT, 100, 220,1170, 2170)),1000); //main gauge needle

      GD.ColorRGB(WHITE);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change


  GD.ColorRGB(WHITE);
  GD.printNfloat(200, 216, canData.COILTEMP, 0, 0); //Speedo numbers

  GD.swap();
  GD.finish();

  }

  while(screenTrack == 300) {           //Digital Oil Temp Gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.COILTEMP);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 4) {           //Analog Air temp gauge

  //canData();
  reading();
  headlightDim();

  int GMAT = canData.CMAT;

    if (GMAT < 60){
      GMAT = 60;
    }

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(RED);
  GD.cmd_gauge(120,197,130,12544,9,5,(map(GMAT, 60, 180,1170, 2170)),1000); //main gauge needle

      GD.ColorRGB(WHITE);
      GD.Begin(BITMAPS);
      GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change


  GD.ColorRGB(WHITE);
  GD.printNfloat(200, 216, canData.CMAT, 0, 0); //Speedo numbers

  GD.swap();
  GD.finish();

  }

  while(screenTrack == 400) {           //Digital Air Temp Gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CMAT);
  
  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 5) {           //Digital Ethanol Content Gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CETHL);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 6) {           //Digital Battery Gauge

  //canData();
  reading();
  headlightDim();

  float GBATT = (((float)canData.CBATT)/10.0);
  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.printNfloat(127, 190, GBATT, 1, 1);

  GD.swap();
  GD.finish();

  Serial.println(GBATT);

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 7) {           //Digital EGO Correction gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CEGOCOR);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 8) {           //Digital Total fuel correction gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CTOTCOR);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 9) {           //Digital IGN Advance

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

  GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)

  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CIGNADV);

  GD.swap();
  GD.finish();

  }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

while(screenTrack == 10) {           //MPG gauge

  //canData();
  reading();
  headlightDim();

  GD.Clear();

  button.read();

   GD.Begin(BITMAPS);
  GD.Vertex2ii(5,85,3);  //Will need to add image handle once assets are loaded (x,y,image handle)


  GD.ColorRGB(WHITE);
  GD.cmd_number(110, 230, 1, OPT_CENTER, canData.CMPG);

  GD.swap();
  GD.finish();

  }

 while(screenTrack == 1000){         //bongo cat easter egg
    

    GD.Clear();

    button.read();  
    GD.BitmapHandle(3);                   //Double check image handle for background jpegs
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("BONGO_CAT2.jpg");  
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);  

    delay(200);

    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);                   //Double check image handle for background jpegs
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("BONGO_CAT.jpg");  
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);  

    delay(200);

    GD.swap();

    GD.finish();

  }

}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void onPressed()
{
  pageTrack = 0;
 // Serial.print("screenTrack = ");
 // Serial.println(screenTrack);
    if (screenTrack == 1){
      GD.Clear();
      GD.BitmapHandle(3);                   //Double check image handle for background jpegs
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("4.jpg");                     //Load Fuel Temp BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
      Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }

    else if (screenTrack == 2){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("5.jpg");                     //Load Oil Temp BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }

    else if (screenTrack == 3){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("6.jpg");                       //Air Temp BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }

    else if (screenTrack == 4){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("7.jpg");                     //Load Ethanol BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }

        else if (screenTrack == 5){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("8.jpg");                       //Load Battery BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }

    else if (screenTrack == 6){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("9.jpg");                             //Load EGO Corr BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 7){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("10.jpg");                              //Load Total Fuel Corr BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 8){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("11.jpg");                            //Load Boost target BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 9){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("12.jpg");                            //Load MPG BG
      screenTrack++;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 10){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("1.jpg");                           //Return back to coolant temp BG
      screenTrack = 1;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 100){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("4.jpg");                           //Return back to coolant temp BG
      screenTrack = 2;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }    

    else if (screenTrack == 200){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("5.jpg");                           //Return back to coolant temp BG
      screenTrack = 3;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }   

    else if (screenTrack == 300){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("6.jpg");                           //Return back to coolant temp BG
      screenTrack = 4;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    } 

    else if (screenTrack == 400){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("7.jpg");                           //Return back to coolant temp BG
      screenTrack = 5;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }  

    else if (screenTrack == 1000){
      GD.Clear();
      GD.BitmapHandle(3);
      GD.cmd_loadimage(ASSETS_END, 0);
      GD.load("3.jpg");                           //Return back to coolant temp BG
      screenTrack = 1;
      Serial.print("screenTrack = ");
      Serial.println(screenTrack);
            Serial.print("pageTrack = ");
      Serial.println(pageTrack);
    }   
}

void onPressedForDuration()
{
   
  if (screenTrack == 1){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("1a.jpg");
    screenTrack = 100;  
    }

  else if (screenTrack == 100){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("1.jpg");
    screenTrack = 1;  
    }

  else if (screenTrack == 2){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("4a.jpg");    
    screenTrack = 200;  
    }

  else if (screenTrack == 200){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("4.jpg");    
    screenTrack = 2;  
    }

  else if (screenTrack == 3){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("5a.jpg");    
    screenTrack = 300;  
    }

  else if (screenTrack == 300){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("5.jpg");    
    screenTrack = 3;  
    }

  else if (screenTrack == 4){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("6a.jpg");    
    screenTrack = 400;  
    }

  else if (screenTrack == 400){
    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("6.jpg");    
    screenTrack = 4;  
    }
  
  Serial.print("pageTrack = ");
  Serial.println(pageTrack);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
      /*
void canData() {

  if ( Can0.read(rxmsg) ) {
    switch (rxmsg.id) { // Using IDs from 1512 as Megasquirt CAN broadcast frames for Simplified Dash Broadcasting.
      // EAch frame represents a data group http://www.msextra.com/doc/pdf/Megasquirt_CAN_Broadcast.pdf
      case 1521:
        STATUS = (float)(word(rxmsg.buf[3],rxmsg.buf[3]));  // Status of engine state
      break;
      case 1522:
        CBARO = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
        CMAT = (float)(word(rxmsg.buf[4], rxmsg.buf[5]));  //divide by 10
        CCLT = (float)(word(rxmsg.buf[6], rxmsg.buf[7]));  //divide by 10
      break;
      case 1523:
        CTPS = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
        CBATT = (float)(word(rxmsg.buf[2], rxmsg.buf[3]));  //divide by 10
      break;
      case 1524:
        CEGOCOR = (float)(word(rxmsg.buf[2], rxmsg.buf[3]));  //divide by 10
      break;
      case 1526:
        CTOTCOR = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
      break;
      case 1533:
        COILTEMP = (float)(word(rxmsg.buf[4], rxmsg.buf[5]));  //divide by 10
        CCOOLPRESS = (float)(word(rxmsg.buf[6], rxmsg.buf[7]));  //divide by 10
      break;
      case 1535:
        COILP = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
        CFUELP = (float)(word(rxmsg.buf[2], rxmsg.buf[3]));  //divide by 10
      break;
      case 1537:
        CBOOSTTAR = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
      break;
      case 1567:
        CETHL = (float)(word(rxmsg.buf[0], rxmsg.buf[1]));  //divide by 10
      break;
      case 1572:
        CMPG = (float)(word(rxmsg.buf[6], rxmsg.buf[7]));  //divide by 10, units is l/km
      break;
      case 1573:
        CFUELTEMP = (float)(word(rxmsg.buf[4], rxmsg.buf[5]));  //divide by 10
      break;
    }
}
}
*/



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reading() {
/*
  TPS = CTPS/10;
  BARO = CBARO/10;
  MAT = CMAT/10;
  CLT = CCLT/10;
  BATT = CBATT/10;
  EGOCOR = CEGOCOR/10;
  TOTCOR = CTOTCOR/10;
  OILTEMP = COILTEMP/10;
  COOLPRESS = CCOOLPRESS/10;
  OILP = COILP/10;
  FUELP = CFUELP/10;
  BOOSTTAR = (CBOOSTTAR/10); //in KPA, do conversion
  ETHL = CETHL/10;
  MPG = (CMPG * 2.35);    //in L/KM, do conversion
  FUELTEMP = CFUELTEMP/10;    //ALready in degrees F
*/
  button2.read();


    //int TPS = canData.CTPS;
    /*
    int RPM = canData.CRPM;
    int STATUS = canData.STATUS;
    int BARO = canData.CBARO;
    int KPA = canData.CKPA;
    int AFR = canData.CAFR;
    int MAT = canData.CMAT;
    int CLT = canData.CCLT;
    int BATT = canData.CBATT;
    int EGOCOR = canData.CEGOCOR;
    int TOTCOR = canData.CTOTCOR;
    int OILT = canData.COILTEMP;
    int COOLP = canData.CCOOLPRESS;
    int COIL = canData.COILP;
    int FUELP = canData.CFUELP;
    int BOOSTTAR = canData.CBOOSTTAR;
    int ETHL = canData.CETHL;
    int MPG = canData.CMPG;
    int FUELT = canData.CFUELTEMP;
*/
    
}

/////////////////////////////////////////////////////////////////////////////////////////////

void headlightDim () {

  if (digitalRead(headlightPin) == HIGH){
    GD.wr(REG_PWM_DUTY, 15);
  }

  else if (digitalRead(headlightPin) == LOW){
    GD.wr(REG_PWM_DUTY, 75);
  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sequence() {                      ///Gauge Sequence Code
    int seqdelay = 100;

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("b.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("c.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("d.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("e.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("f.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("g.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("h.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("i.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("j.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("k.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("l.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("m.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("n.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    GD.Clear();
    GD.BitmapHandle(3);
    GD.cmd_loadimage(ASSETS_END, 0);
    GD.load("1.jpg");   
    delay(seqdelay);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(5,85,3);
    GD.ColorRGB(RED);
    GD.cmd_gauge(120,197,130,12544,9,5,1170,1000);
    GD.ColorRGB(WHITE);
    GD.Begin(BITMAPS);
    GD.Vertex2ii((120-18), (197-18), 2);   // Needle center asset, the 2 might need to change
    GD.swap();

    screenTrack = 1;

}

///////////////////////////////////////////
