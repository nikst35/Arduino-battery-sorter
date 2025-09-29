#include <Servo.h>
#include "ADS1X15.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ----- senzorji in zaslon -----
ADS1115 ADS(0x48);                   // ADC za meritve napetosti
LiquidCrystal_I2C lcd(0x3F, 20, 4);  // 20x4 LCD zaslon

// ----- pini in konstante -----
constexpr uint8_t PIN_SERVO_HOLDER   = 3;   // servo, ki premika nosilec baterije
constexpr uint8_t PIN_SERVO_GRIPPER  = 2;   // servo, ki prime baterijo
constexpr uint8_t PIN_DROP[4]        = {5, 6, 7, 8}; // 4 izmetna mesta
constexpr uint8_t PIN_LOAD_RELAY     = 12;  // rele, ki vklopi obremenitev

// kote za servote
constexpr int ANGLE_DROP_OPEN    = 100;
constexpr int ANGLE_DROP_CLOSED  = 15;

// pozicije za nosilec in prijemalo
constexpr int HOLDER_MIN   = 55;
constexpr int HOLDER_PICK  = 55;
constexpr int HOLDER_MID   = 100;
constexpr int HOLDER_MAX   = 140;

constexpr int GRIPPER_OPEN     = 40;
constexpr int GRIPPER_CONTACT  = 80;
constexpr int GRIPPER_CLOSED   = 90;

// meritve
constexpr int   NUM_READS          = 20;    // povprečenja
constexpr float SENSE_RESISTOR_OHM = 4.9f;  // znana upornost za izračun notranje R
constexpr float ADS_FSR_V          = 4.096f;
constexpr float ADS_COUNTS         = 32767.0f;

// ----- servo motorji -----
Servo servoHolder;
Servo servoGripper;
Servo drop[4];

// števec baterij
long batteryCount = -1;

// ----- funkcije -----

// gladko premikanje serva med dvema kotoma
void smoothMove(Servo& s, int from, int to, int step, int delayMs) {
  if (from == to) { s.write(to); return; }
  int dir = (to > from) ? step : -step;
  for (int p = from; p != to; p += dir) {
    s.write(p);
    delay(delayMs);
  }
  s.write(to);
}

// pripravi LCD zaslon na novo meritev
void lcdHeader() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("-V1:        |");
  lcd.setCursor(0,1); lcd.print("-V2:        |");
  lcd.setCursor(0,2); lcd.print("-Res:       |");
  lcd.setCursor(0,3); lcd.print("-Box:       |");
  lcd.setCursor(15,0); lcd.print("GEN1");
  lcd.setCursor(15,2); lcd.print("Cnt:");
  lcd.setCursor(16,3); lcd.print(batteryCount);
}

// prebere napetost z ADC in povpreči več meritev
float readVoltageAveraged(uint8_t channel, int blocks, int samples, int rawMin = 10000) {
  long sumBlocks = 0;
  for (int b = 0; b < blocks; ++b) {
    long sum = 0;
    if (ADS.readADC(channel) < rawMin) break; // če ni kontakta, prekini
    for (int i = 0; i < samples; ++i) {
      sum += ADS.readADC(channel);
      delay(1);
    }
    sumBlocks += (sum / samples);
  }
  long avgCounts = sumBlocks / max(1, blocks);
  return (avgCounts * ADS_FSR_V) / ADS_COUNTS;
}

// zapre vse izmetne predale
void setAllDroppersClosed() {
  for (int i = 0; i < 4; ++i) drop[i].write(ANGLE_DROP_CLOSED);
}

// izbere ustrezen predal glede na izmerjeno notranjo upornost
void selectDropperByResistance(float r) {
  setAllDroppersClosed();
  int binIndex = 4; // privzeto: "slaba celica"

  if (r <= 0.12f)           binIndex = 0;
  else if (r <= 0.15f)      binIndex = 1;
  else if (r <= 0.18f)      binIndex = 2;
  else if (r <= 0.21f)      binIndex = 3;

  if (binIndex < 4) drop[binIndex].write(ANGLE_DROP_OPEN);
  lcd.setCursor(6,3); lcd.print(binIndex + 1);  // pokaže izbran predal
  delay(1000);
}

// ----- setup -----
void setup() {
  ADS.begin();
  ADS.setGain(1);

  lcd.begin();
  lcd.backlight();
  Serial.begin(115200);

  // init servo motorje
  servoHolder.attach(PIN_SERVO_HOLDER);
  servoGripper.attach(PIN_SERVO_GRIPPER);
  for (int i = 0; i < 4; ++i) drop[i].attach(PIN_DROP[i]);

  // začetne pozicije
  servoHolder.write(HOLDER_MID);
  servoGripper.write(GRIPPER_OPEN);
  setAllDroppersClosed();

  pinMode(PIN_LOAD_RELAY, OUTPUT);
  digitalWrite(PIN_LOAD_RELAY, LOW);

  // intro zaslon
  lcd.clear();
  lcd.setCursor(1,0); lcd.print("------------------");
  lcd.setCursor(0,1); lcd.print("Reusable technologies");
  lcd.setCursor(7,2); lcd.print("GEN 1");
  lcd.setCursor(1,3); lcd.print("------------------");
  delay(3000);
}

// ----- glavni cikel -----
void loop() {
  batteryCount++;

  // 1) premik nosilca do baterije
  smoothMove(servoHolder, HOLDER_MID, HOLDER_PICK, 1, 20);
  delay(300);

  // 2) dvig baterije
  smoothMove(servoHolder, HOLDER_PICK, HOLDER_MID + 5, 1, 20);
  delay(300);

  // 3) zapri prijemalo, da se baterija dotakne sond
  smoothMove(servoGripper, GRIPPER_OPEN, GRIPPER_CONTACT, 1, 40);
  delay(300);

  // pripravi LCD
  lcdHeader();

  // --- meritve ---
  float V1 = readVoltageAveraged(0, NUM_READS, NUM_READS);
  V1 = roundf(V1 * 1000.0f) / 1000.0f;

  // če ni napetosti → prazno mesto
  if (V1 < 3.0f) {
    lcd.setCursor(6,0); lcd.print("Null");
    lcd.setCursor(6,1); lcd.print("----");
    lcd.setCursor(6,2); lcd.print("----");
    lcd.setCursor(6,3); lcd.print("----");
    batteryCount--;
    smoothMove(servoGripper, GRIPPER_CONTACT, GRIPPER_OPEN, 1, 40);
    delay(200);
    return;
  }

  lcd.setCursor(6,0); lcd.print(V1, 3);

  // 4) vklopi obremenitev
  digitalWrite(PIN_LOAD_RELAY, HIGH);
  delay(500);

  // 5) meri napetost pod obremenitvijo
  float V2 = readVoltageAveraged(0, NUM_READS, NUM_READS);
  V2 = (V2 < 3.0f) ? 0.0f : roundf(V2 * 1000.0f) / 1000.0f;

  // 6) izračun notranje upornosti
  float Rint = 0.0f;
  if (V2 > 0.01f && V1 > V2) {
    Rint = ((V1 - V2) / V2) * SENSE_RESISTOR_OHM;
  }

  // izpisi za nadzor
  Serial.print("V1: "); Serial.println(V1,3);
  Serial.print("V2: "); Serial.println(V2,3);
  Serial.print("R  : "); Serial.println(Rint,4);
  Serial.println("--------");

  lcd.setCursor(6,1); lcd.print(V2, 3);
  lcd.setCursor(6,2); lcd.print(Rint, 3);

  // 7) izklopi obremenitev
  digitalWrite(PIN_LOAD_RELAY, LOW);
  delay(300);

  // 8) spusti baterijo in odloži v pravi predal
  smoothMove(servoGripper, GRIPPER_CONTACT, GRIPPER_OPEN, 1, 40);
  smoothMove(servoHolder, HOLDER_MID + 5, HOLDER_MAX, 1, 20);
  selectDropperByResistance(Rint);

  // 9) vrni nosilec v začetni položaj
  smoothMove(servoHolder, HOLDER_MAX, HOLDER_MID, 1, 20);
}
