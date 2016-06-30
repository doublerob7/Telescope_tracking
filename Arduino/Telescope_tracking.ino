/* ******* Telescope Tracking Script **********************

written by Robert Ressler 
for ME 4020 Mechatronics

Provides tracking to dobsonian or other Alt/Azimuth mounted
telescopes with only the Arduino. No computer control 
nessessary, and no GO-TO functionality implemented.
*********************************************************/


//Included Libraries
#include <Adafruit_MotorShield.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Keypad.h>
#include "Arduino.h"

//Declaring globals
float latitude, declination, altitude, azimuth;
float threshold = (1.8 * 3600);
char setKey, Key;
int STATUS;
int lastTime;
String lcdString;
float errorAlt, errorAz;
float degToRad = (2*3.1415926)/360;
//String MODE = String("SINGLE"+"DOUBLE"+"INTERLEAVE"+"MICROSTEP");

//Initialize LCD on Arduino pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//Setup for Keypad class
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6}; //connect to the column pinouts of the keypad

//Initialize an instance of the Keypad class
Keypad keyPad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//Initialize an instance of the Motorshield class
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

//Create motors
Adafruit_StepperMotor *altMotor = AFMS.getStepper(200,1);
Adafruit_StepperMotor *azMotor = AFMS.getStepper(200,2);



//************************ setup() *********************
void setup() {
  lcd.begin(16,2);
  lcdRefresh("   Welcome!"); lcd.print("Any key to cont");
  keyPad.waitForKey();
  
  //input and convert Latitude
  String deg = getInput("Lat Deg"); 
  latitude = deg.toFloat() * 3600; 
  String arcmin = getInput("Lat ArcM"); 
  latitude += arcmin.toFloat() * 60; 
  String arcsec = getInput("Lat ArcS"); 
  latitude += arcsec.toFloat(); 
  String mant = getInput("Lat dec"); 
  latitude += mant.toFloat()/(pow(10,mant.length())); 
  
  lcdRefresh("Latitude:    #->");
  printDegrees(latitude);
  Key = keyPad.waitForKey();
  
  AFMS.begin();
  
  alignTelescope();
  
  lastTime = micros();
  
}



//********************** alignTelescope() ***********************
void alignTelescope(){
  //subroutine to use the keypad to point the telescope at the north star
  lcdRefresh("Align w Polaris"); lcd.print("2 4 6 8 to slew");
  if (Key == '#') { Key = '!';}
  
  while (Key != '#') {
    Key = keyPad.getKey();
    if (Key) {setKey = Key;}
    STATUS = keyPad.getState();
    
    if (STATUS == 2) {
      slew();
      lcdRefresh("Aligned?    #->"); lcd.print("2 4 6 8 to slew");
    }
  }
  
  altitude = latitude;
  azimuth = 180.0 * 3600;
  
}



//*********************** loop() **************************
void loop() {
  
  lcdOrient();

  Key = keyPad.getKey();
  STATUS = keyPad.getState();
  if (Key) { setKey = Key; } 
  if (STATUS == 2) { slew(); } 
  
  altMotor->setSpeed(4);
  azMotor->setSpeed(4);
  
  if (altitude < 0.0) { altitude = 0.0; altMotor->release();}
  else if (altitude > (90.0*3600.0)) { altitude = (90.0*3600.0); altMotor->release(); }
  
  if (azimuth < 0.0) { azimuth += (360.0*3600.0); azMotor->release(); }
  else if (azimuth > (360.0*3600.0)) { azimuth -= (360.0*3600.0); azMotor->release(); }
  

  //errorAlt += calculateError(0);
  //errorAz += calculateError(1);
  
  //lcdRefresh("ErrorAlt"); lcd.print(errorAlt); delay(100);
  
  if (abs(errorAlt) > threshold) 
  {
    int i;
    if (errorAlt > 0) { 
      altMotor->onestep(FORWARD,SINGLE); 
      errorAlt -= threshold;
      altitude += threshold; 
      lastTime = micros();
    }
    else if (errorAlt < 0) { 
      altMotor->onestep(BACKWARD,SINGLE); 
      errorAlt += threshold; 
      altitude -= threshold;  
      lastTime = micros();
      }
  }
  
  if (abs(errorAz) > threshold) 
  {
    int i;
    if (errorAz > 0) { 
      azMotor->onestep(FORWARD,SINGLE); 
      errorAz -= threshold; 
      azimuth += threshold; 
      lastTime = micros();  }
    else if (errorAz < 0) { 
      azMotor->onestep(BACKWARD,SINGLE); 
      errorAz += threshold; 
      azimuth -= threshold; 
      lastTime = micros();  }
  }
  delay(100);
}



//************************* getInput() *************************
String getInput(String value){
  //subroutine to get input from keypad. This will hold the program until '#' is entered.
  
  lcdString.remove(0); lcdString = "Enter "; lcdString += value; lcdString += ":";
  lcdRefresh( "" ); 
  
  String inputString;
  Key = keyPad.waitForKey();
  
  while (Key != '#')  {
    if(Key && Key != '*')  { lcd.print(Key); inputString += Key; }
    else if (Key == '*' )  {  //Backspace functionality
      inputString.remove(inputString.length() - 1);
      lcdRefresh( "" );
      lcd.print(inputString);
    }
    Key = keyPad.getKey();
  }
  return inputString;
}



//************************ lcdRefresh() ***************************
void lcdRefresh(String string) {
  lcd.clear(); 
  if (string != "") {
    lcdString.remove(0);
    lcdString = string;
  }
  else{}
  lcd.print(lcdString);
  lcd.setCursor(0,1); lcd.blink();
}



//************************ lcdOrient() ***************************
void lcdOrient() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Al "); printDegrees(altitude);
  lcd.setCursor(0,1); lcd.print("Az "); printDegrees(azimuth);
}



//*********************** calculateError() *************************
float calculateError(int orient){
  //calculate error in positioning
  
  float deltaAlt, deltaAz, deltah, deltaTime, dec;
  int omegaEarth = 15;
  
  latitude /= 3600;
  altitude /= 3600;
  azimuth /= 3600;
  
  latitude *= degToRad;
  altitude *= degToRad;
  azimuth *= degToRad;
  
  deltaTime = (micros()-float(lastTime))*pow(10,-6);
  deltah = omegaEarth * deltaTime;
  
  dec = asin( (sin(latitude)*sin(altitude)) - ( cos(latitude)*cos(altitude)*cos(azimuth) ) );
  
  deltaAlt = asin( ( sin(latitude)*sin(dec) ) + ( cos(latitude)*cos(dec)*cos(deltah) ) );
  deltaAz = atan( sin(deltah) / ( ( cos(deltah)*sin(latitude) ) - ( tan(dec)*cos(deltah) ) ) );
  
  deltaAlt /= degToRad; deltaAlt *= 3600;
  deltaAz /= degToRad; deltaAz *= 3600;
  
  latitude /= degToRad;
  altitude /= degToRad;
  azimuth /= degToRad;
  
  latitude *= 3600;
  altitude *= 3600;
  azimuth *= 3600;
  
  if (orient == 0) { return deltaAlt;}
  else if (orient == 1) { return deltaAz;}
  
}



//*********************** slew() **********************************
void slew(){
  //This function will override the current tracking and rapidly slew the telescope
  int startTime;
  int i;
  STATUS = keyPad.getState();
  //Adafruit_StepperMotor *altMotor = AFMS.getStepper(200,1);
  //Adafruit_StepperMotor *azMotor = AFMS.getStepper(200,2);
  altMotor->setSpeed(50);
  azMotor->setSpeed(50);
  
  startTime = millis();
  while (STATUS != 3) {
    
    if      ( setKey == '2' ) { lcdRefresh("Alt +"); i = 0; altMotor->onestep(FORWARD,SINGLE); altitude += threshold; }
    else if ( setKey == '4' ) { lcdRefresh("Az -"); i = 1; azMotor->onestep(BACKWARD,SINGLE); azimuth -= threshold; }
    else if ( setKey == '6' ) { lcdRefresh("Az +"); i = 1; azMotor->onestep(FORWARD,SINGLE); azimuth += threshold; }
    else if ( setKey == '8' ) { lcdRefresh("Alt -"); i = 0; altMotor->onestep(BACKWARD,SINGLE); altitude -= threshold; }
    
    if (i == 0) { printDegrees(altitude); }
    else if (i == 1) { printDegrees(azimuth); }
    
    keyPad.getKey();
    STATUS = keyPad.getState();
    lcd.setCursor(15,0); lcd.print(STATUS);
    lcd.setCursor(15,1); lcd.print((millis() - startTime)/1000);
    delay(10);
  }
}



//*********************** printDegrees() **************************
void printDegrees(float value) {
  float deg = int(value / 3600); 
  value -= (deg * 3600);
  lcd.print(int(deg)); lcd.print(" ");
  float arcmin = int(value / 60); 
  value -= (arcmin * 60);
  lcd.print(int(arcmin)); lcd.print(" ");
  float arcsec = value; 
  lcd.print(arcsec);
}



/************************ deltaT() *****************************
int deltaT() {
  int thisTime;
  thisTime = micros();
  int deltaTime = thisTime - lastTime;
  lastTime = thisTime;
  return deltaTime;
}*/
