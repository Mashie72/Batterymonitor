#include <Arduino_GFX_Library.h>
#include <NimBLEDevice.h>

// 1. DataBus för Touch-versionen:
// DC=15, CS=14, SCK=1, MOSI=2
Arduino_DataBus *bus = new Arduino_ESP32SPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);

// 2. Display-inställning:
// RST=22, BL=23
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 22 /* RST */, 1 /* rotation */, false /* IPS */,
  172, 320, 34, 0, 34, 0
);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("Startar Waveshare C6 TOUCH-version...");

  // 3. Tänd bakgrundsbelysningen på GPIO 23
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH); 

  if (!gfx->begin()) {
    Serial.println("Kunde inte initiera GFX!");
  }
  bus->beginWrite();
  bus->writeCommand(0x36); 
  bus->write(0x28); //normal landscape flip
  bus->endWrite();
  gfx->fillScreen(0x0000); 
  
  gfx->setCursor(20, 50);
  gfx->setTextColor(0xFFFF); // Vit text
  gfx->setTextSize(2);
  gfx->println("BT battery monitor");
  
  Serial.println("Display igång");
}

void loop() {
}