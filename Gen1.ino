#include <Servo.h>
#include "ADS1X15.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

ADS1115 ADS(0x48);
LiquidCrystal_I2C lcd(0x3F,20,4);

const int numReads=20;
const float resistor=4.9;

Servo servoCellHolder;  // create servo object to control a servo
Servo servoGrapper;
Servo servoDropper1;
Servo servoDropper2;      
Servo servoDropper3;
Servo servoDropper4;

int DO = 100; //dropper open
int DC = 15; //dropper closed
int batteryCnt=-1;

int dropperSelect (float resistance){
  if (resistance<=0.12){
    servoDropper1.write(DO);
    lcd.setCursor (6, 3);lcd.print(1);
    delay(1200);
  }
 else if (resistance<=0.15){
    servoDropper1.write(DC);
    servoDropper2.write(DO);
    lcd.setCursor (6, 3);lcd.print(2);
    delay(2200);
  }
 else if (resistance<=0.18){
    servoDropper2.write(DC);servoDropper1.write(DC);
    servoDropper3.write(DO);
    lcd.setCursor (6, 3);lcd.print(3);
    delay(3200);
  }
 else if (resistance<=0.21){
    servoDropper2.write(DC);servoDropper3.write(DC);servoDropper1.write(DC);
    servoDropper4.write(DO);
    lcd.setCursor (6, 3);lcd.print(4);
    delay(4200);
  }
  else{
    servoDropper1.write(DC);servoDropper2.write(DC);servoDropper3.write(DC);servoDropper4.write(DC);
    lcd.setCursor (6, 3);lcd.print(5);
  }         
}

void setup() {
 
  ADS.begin();
  ADS.setGain(1);
  lcd.begin();
  lcd.backlight();
  Serial.begin(115200);

  servoCellHolder.write(100);servoGrapper.write(40);
  servoDropper1.write(DC);servoDropper2.write(DC);
  servoDropper3.write(DC);servoDropper4.write(DC);

  servoCellHolder.attach(3); // middle:100, min:55, max:130
  servoGrapper.attach(2);// closed:90, open:40
  servoDropper1.attach(5); //closed:17, open:100
  servoDropper2.attach(6);
  servoDropper3.attach(7);
  servoDropper4.attach(8);
  
  pinMode(12, OUTPUT);

  lcd.clear();
  lcd.setCursor (1, 0);lcd.print("------------------");
  lcd.setCursor (0, 1);lcd.print("Reusable technologys");
  lcd.setCursor (7, 2);lcd.print("GEN 1");
  lcd.setCursor (1, 3);lcd.print("------------------");
  delay(10000);
}

void loop() {

  servoDropper1.detach();servoDropper2.detach();servoDropper3.detach();servoDropper4.detach();
  batteryCnt+=1;

  lcd.clear();
  lcd.setCursor (0, 0);lcd.print("-V 1:       |");
  lcd.setCursor (0, 1);lcd.print("-V 2:       |");
  lcd.setCursor (0, 2);lcd.print("-Res:       |");
  lcd.setCursor (0, 3);lcd.print("-Box:       |");
  lcd.setCursor (15, 0);lcd.print("GEN1");
  lcd.setCursor (15, 2);lcd.print("Cnt:");
  lcd.setCursor (16, 3);lcd.print(batteryCnt);
  // - - - - - - - - - + + - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - -

  long int sensSum=0;
  long int sensAve=0;
  long int UltimateSensAve=0;
  long int UltimateSum=0;

  float Voltage1=0;
  float V1=0;
  float Voltage2=0;
  float V2=0;
  float resistance=0;

  int totalSensorValue=0;
  int avrageReading=0;
  int pos;


  digitalWrite(12, LOW);
  

 
  delay(500);
  //pick up battery and place it in grapper:
  for (pos=100; pos>=55; pos--){
      servoCellHolder.write(pos);              
      delay(20);                      
      }
  delay(500);
   
  for (pos=55; pos<=105; pos++) {
      servoCellHolder.write(pos);              
      delay(20);
      }
  delay(500);
     
  for (pos=40; pos<=80; pos++) { //battery connects to grapper
      servoGrapper.write(pos);              
      delay(40);
      }
  delay(500);


  for(int t=0;t<numReads;t++){
    sensSum=0;
    sensAve=0;
    if (ADS.readADC(0)<10000){
      break;
    }
      for (int k=0;k<numReads;k++){
        sensSum += ADS.readADC(0);
        delay(1);
      }
  sensAve = sensSum/numReads;
  //Serial.println(sensAve);
  UltimateSum = UltimateSum + sensAve; 
}

  UltimateSensAve = UltimateSum/numReads;
  V1 = (UltimateSensAve * 4.096 / 32767);//25550
  Voltage1=round(V1*1000)/1000.0;

  if (Voltage1 < 3){
  lcd.setCursor (6, 0);
  lcd.print("Null");
  lcd.setCursor (6, 1);
  lcd.print("----");
  lcd.setCursor (6, 2);
  lcd.print("----");
  lcd.setCursor (6, 3);
  lcd.print("----");
    Voltage1 =0;
    batteryCnt-=1;
    for (pos=90; pos>=40; pos--) {
      servoGrapper.write(pos);              
      delay(40);
      }
  }
  else{
  lcd.setCursor (6, 0);lcd.print(Voltage1,3);
  UltimateSensAve=0; UltimateSum=0;

 digitalWrite(12, HIGH);

 delay(500);

   for(int t=0;t<numReads;t++){
    sensSum=0;
    sensAve=0;
    if (ADS.readADC(0)<10000){
      break;
    }
      for (int k=0;k<numReads;k++){
        sensSum += ADS.readADC(0);
        delay(1);
      }
  sensAve = sensSum/numReads;
  UltimateSum = UltimateSum + sensAve; 
}
  UltimateSensAve = UltimateSum/numReads;
  V2 = (UltimateSensAve * 4.096 / 32767);//25550
  Voltage2=round(V2*1000)/1000.0;
  if (Voltage2 < 3){
    Voltage2 =0;
  }
 
  resistance = ((Voltage1-Voltage2)/Voltage2)*resistor;

  Serial.print("Voltage1: ");Serial.println(Voltage1,3);
  Serial.print("Voltage2: ");Serial.println(Voltage2,3);

  lcd.setCursor (6, 1);lcd.print(Voltage2,3);
  lcd.setCursor (6, 2);lcd.print(resistance);


  Serial.println("--------");
  Serial.print("Resistance: ");Serial.println(resistance);   
  Serial.println("---done");

  digitalWrite(12, LOW);  
  delay(500);
 
  for (pos=80; pos>=40; pos--) {
      servoGrapper.write(pos);              
      delay(40);
      }
 
  for (pos=105; pos<=140; pos++) {
    servoCellHolder.write(pos);              
    delay(20);                      
    }
  servoDropper1.attach(5); //closed:17, open:100
  servoDropper2.attach(6);
  servoDropper3.attach(7);
  servoDropper4.attach(8);

  dropperSelect(resistance);
  }                          
}
