#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "qrcode.h"
#include "Bitcoin.h"
#include "EEPROM.h"

#define LED_BUILTIN 2
#define PREV_BUTTON 13
#define NEXT_BUTTON 27

GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
HDPublicKey pub("vpub5ZaeHNQhVAsp4CP6RNKafWA5kNz7dPymXHznNFz7LNe5w26AYKYcvW1w8MDVaF5wDMpqSvU5NS4NxH1PqQ1QgAZGUmikmC4uNWTcF1Wt12J");

uint32_t ind = 0;
#define MAGIC 0x12345678

void setup()
{
  pinMode(PREV_BUTTON, INPUT_PULLUP);
  pinMode(NEXT_BUTTON, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW); // another GND to avoid soldering

  EEPROM.begin(10);
  if(EEPROM.readULong(0) == MAGIC){ // magic, check if we stored index already
    ind = EEPROM.readULong(4);
  }else{
    EEPROM.writeULong(0, MAGIC);
    EEPROM.writeULong(4, ind);
    EEPROM.commit();
  }
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  delay(100);
  display.init(115200);
  showAddress(pub.child(0).child(ind).address());
}

void loop(){
  digitalWrite(LED_BUILTIN, LOW);
  if(!digitalRead(NEXT_BUTTON)){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100); // debounce
    ind++;
    EEPROM.writeULong(4, ind);
    EEPROM.commit();
    showAddress(pub.child(0).child(ind).address());
  }
  if(!digitalRead(PREV_BUTTON) && ind > 0){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    ind--;
    EEPROM.writeULong(4, ind);
    EEPROM.commit();
    showAddress(pub.child(0).child(ind).address());
  }
  delay(100);
}

void showAddress(String addr){
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = addr.length();
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }
  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, (String("bitcoin:")+addr).c_str());

  int width = 17 + 4*qrSize;
  int scale = 150/width;
  int padding = (200 - width*scale)/2;

  display.setFullWindow();
  display.firstPage();

  int16_t tbx, tby; uint16_t tbw, tbh;
  String path = String("m/84'/1'/0'/0/")+ind;
  display.getTextBounds(path, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x0 = ((display.width() - tbw) / 2) - tbx;
  uint16_t y0 = 20;
  
  do{
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(x0, y0);
    display.print(path);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if(qrcode_getModule(&qrcode, x, y)){
              display.fillRect(padding+scale*x, 10 + padding+scale*y, scale, scale, GxEPD_BLACK);
            }
        }
    }
  }while(display.nextPage());
}
