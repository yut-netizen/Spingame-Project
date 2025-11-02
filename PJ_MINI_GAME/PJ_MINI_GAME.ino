// ----- Blynk Config -----
#define BLYNK_TEMPLATE_ID "TMPL6RNZlEF4b"
#define BLYNK_TEMPLATE_NAME "Spin Machine"
#define BLYNK_AUTH_TOKEN "TjnW6mp9TuATcBE3nb4p5c5H7MlyPKhx"

// ----- Libraries -----
#include <WiFiS3.h>              // สำหรับ Arduino UNO R4 WiFi
#include <BlynkSimpleWifi.h>
#include <Wire.h>
#include <EEPROM.h>              // UNO R4 มี EEPROM จำลองใน Flash อยู่แล้ว
#include <LiquidCrystal_I2C.h>

// ----- Custom LCD -----
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----- WiFi -----
char auth[] = "TjnW6mp9TuATcBE3nb4p5c5H7MlyPKhx";
char ssid[] = "gg";
char pass[] = "cccccccc";

// ----- Custom Characters -----
byte Heart[8] = {0b00000,0b01010,0b11111,0b11111,0b01110,0b00100,0b00000,0b00000};
byte Bell[8]  = {0b00100,0b01110,0b01110,0b01110,0b11111,0b00000,0b00100,0b00000};
byte Sound[8] = {0b00001,0b00011,0b00101,0b01001,0b01001,0b01011,0b11011,0b11000};
byte Lock[8]  = {0b01110,0b10001,0b10001,0b11111,0b11011,0b11011,0b11111,0b00000};
byte Alien[8] = {0b11111,0b10101,0b11111,0b11111,0b01110,0b01010,0b11011,0b00000};
byte Skull[8] = {0b00000,0b01110,0b10101,0b11011,0b01110,0b01110,0b00000,0b00000};

// ----- Note Definitions -----
#define NOTE_C3 131
#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_FS3 185
#define NOTE_F3 175
#define NOTE_E3 165
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_C6 1047

// ----- Pins -----
const int spinButtonPin = 2;
const int infoButtonPin = 3;
const int buzzerPin = 8;

// ----- Game Variables -----
int playerCredits = 1000;
int highScore = 0;
int lastWin = 0;
const int betCost = 3;

// ----- EEPROM -----
#define EEPROM_ADDR 0

// ----- Virtual Reels -----
byte virtualReel[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,3,3,3,3,
  4,4,4,4,4,
  5,5,5,
  1,1,
  2
};
const int virtualReelSize = sizeof(virtualReel);
byte reel[3];

// ----- State -----
int currentScreen = 0;
const int totalScreens = 5;
unsigned long lastButtonPress = 0;

// ----- Jackpot Melody -----
int championMelody[] = { NOTE_D5, NOTE_CS5, NOTE_D5, NOTE_CS5, NOTE_A4, 0, NOTE_FS4, NOTE_B4, NOTE_FS4 };
int championNoteDurations[] = { 1,4,4,2,2,4,4,2,1 };

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // ----- WiFi + Blynk -----
  Blynk.begin(auth, ssid, pass);

  // ----- Pins -----
  pinMode(spinButtonPin, INPUT_PULLUP);
  pinMode(infoButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  randomSeed(analogRead(A0));

  // ----- LCD -----
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.createChar(0, Heart);
  lcd.createChar(1, Bell);
  lcd.createChar(2, Sound);
  lcd.createChar(3, Lock);
  lcd.createChar(4, Alien);
  lcd.createChar(5, Skull);

  highScore = loadHighScore();
  updateDisplay();
}

// ----- Main Loop -----
void loop() {
  Blynk.run();

  if (digitalRead(infoButtonPin) == LOW && millis() - lastButtonPress > 250) {
    lastButtonPress = millis();
    tone(buzzerPin, NOTE_C6, 50);
    currentScreen = (currentScreen + 1) % totalScreens;
    updateDisplay();
  }

  if (digitalRead(spinButtonPin) == LOW && millis() - lastButtonPress > 250) {
    lastButtonPress = millis();
    tone(buzzerPin, NOTE_C5, 50);

    if (playerCredits < betCost) {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("GAME OVER");
      lcd.setCursor(0,1); lcd.print("Credits: "); lcd.print(playerCredits);
      updateBlynk();
      while (true);
    }

    currentScreen = 0;
    playerCredits -= betCost;
    runSpinAnimation();
    int winnings = calculateWinnings();
    playerCredits += winnings;
    displayWinnings(winnings);
    delay(winnings == 1000 ? 10000 : 600);
    updateDisplay();
    updateBlynk();
  }
}

// ----- LCD & Display -----
void displaySymbol(byte id) { lcd.write(id <= 5 ? id : '?'); }

void updateDisplay() {
  lcd.clear();
  switch (currentScreen) {
    case 0: lcd.setCursor(0,0); lcd.print("Credits: "); lcd.print(playerCredits);
            lcd.setCursor(0,1); lcd.print("Spin Cost: "); lcd.print(betCost); break;
    case 1: lcd.setCursor(0,0); displaySymbol(0); lcd.print(" - - = 2");
            lcd.setCursor(0,1); displaySymbol(0); lcd.print(" "); displaySymbol(0); lcd.print(" - = 5"); break;
    case 2: lcd.setCursor(0,0); displaySymbol(0); lcd.print(" "); displaySymbol(0); lcd.print(" "); displaySymbol(0); lcd.print(" = 15");
            lcd.setCursor(0,1); displaySymbol(3); lcd.print(" "); displaySymbol(3); lcd.print(" "); displaySymbol(3); lcd.print(" = 40"); break;
    case 3: lcd.setCursor(0,0); displaySymbol(4); lcd.print(" "); displaySymbol(4); lcd.print(" "); displaySymbol(4); lcd.print(" = 80");
            lcd.setCursor(0,1); displaySymbol(5); lcd.print(" "); displaySymbol(5); lcd.print(" "); displaySymbol(5); lcd.print(" = 250"); break;
    case 4: lcd.setCursor(0,0); displaySymbol(1); lcd.print(" "); displaySymbol(1); lcd.print(" "); displaySymbol(1); lcd.print(" = 500");
            lcd.setCursor(0,1); displaySymbol(2); lcd.print(" "); displaySymbol(2); lcd.print(" "); displaySymbol(2); lcd.print(" = 1000"); break;
  }
}

// ----- Spin Animation -----
void runSpinAnimation() {
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Spinning...");
  for (int i = 0; i < 10; i++) {
    tone(buzzerPin, random(150, 300), 15);
    lcd.setCursor(5,1);
    for (int j=0;j<3;j++){ displaySymbol(virtualReel[random(virtualReelSize)]); lcd.print(" "); }
    delay(60);
  }
  for (int i=0;i<3;i++){ reel[i] = virtualReel[random(virtualReelSize)]; tone(buzzerPin, NOTE_G4, 50); delay(100); }
  lcd.setCursor(5,1); displaySymbol(reel[0]); lcd.print(" "); displaySymbol(reel[1]); lcd.print(" "); displaySymbol(reel[2]);
}

// ----- Sound -----
void playWinnerSound(){ tone(buzzerPin, NOTE_C5,125); delay(130); tone(buzzerPin, NOTE_E5,125); delay(130); tone(buzzerPin, NOTE_G5,125); }
void playMediumWinSound(){ tone(buzzerPin, NOTE_G4,100); delay(120); tone(buzzerPin, NOTE_C5,100); delay(120); tone(buzzerPin, NOTE_E5,100); delay(120); tone(buzzerPin, NOTE_G5,100); delay(120); tone(buzzerPin, NOTE_C6,250); }
void playJackpotSound(){ for(int i=0;i<sizeof(championMelody)/sizeof(championMelody[0]);i++){ int d=1000/championNoteDurations[i]; tone(buzzerPin,championMelody[i],d); delay(d*1.3); noTone(buzzerPin);} }
void playLoserSound(){ tone(buzzerPin, NOTE_G3,250); delay(280); tone(buzzerPin, NOTE_C3,300); }

// ----- Win Logic -----
int calculateWinnings(){
  if (reel[0]==reel[1] && reel[1]==reel[2]){
    switch(reel[0]){
      case 2:return 1000;
      case 1:return 500;
      case 5:return 250;
      case 4:return 80;
      case 3:return 40;
    }
  }
  if (reel[0]==0 && reel[1]==0 && reel[2]==0)return 15;
  if (reel[0]==0 && reel[1]==0)return 5;
  if (reel[0]==0)return 2;
  return 0;
}

// ----- Display Winnings -----
void displayWinnings(int winnings){
  lcd.clear(); lcd.setCursor(0,1); lcd.print("Credits: "); lcd.print(playerCredits);
  lcd.setCursor(0,0);
  if (winnings>0){
    if(winnings==1000){ lcd.print("!! JACKPOT !! "); playJackpotSound(); }
    else if(winnings==500){ lcd.print("BIG WIN! "); lcd.print(winnings); playMediumWinSound(); }
    else { lcd.print("YOU WON "); lcd.print(winnings); playWinnerSound(); }
  } else { lcd.print("Try Again..."); playLoserSound(); }
  lastWin=winnings;
  if (playerCredits>highScore){ highScore=playerCredits; saveHighScore(highScore); }
}

// ----- EEPROM -----
void saveHighScore(int score){ EEPROM.put(EEPROM_ADDR, score); }
int loadHighScore(){ int score; EEPROM.get(EEPROM_ADDR, score); if(score<0||score>999999)score=0; return score; }

// ----- Blynk -----
void updateBlynk(){
  static unsigned long lastSend=0;
  if(millis()-lastSend>2000){
    Blynk.virtualWrite(V0, playerCredits);
    Blynk.virtualWrite(V1, highScore);
    Blynk.virtualWrite(V2, lastWin);
    lastSend=millis();
  }
}
