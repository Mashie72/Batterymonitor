#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// 1. DataBus för Touch-versionen:
// DC=15, CS=14, SCK=1, MOSI=2
Arduino_DataBus *bus = new Arduino_ESP32SPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);

// 2. Display-inställning:
// RST=22, BL=23
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 22 /* RST */, 1 /* rotation */, false /* IPS */,
  172, 320, 34, 0, 34, 0
);

// Vi sparar UUID för JBD-batteriet
static BLEUUID serviceUUID("FFE0");
static BLEUUID charUUID_Write("FFE3");
static BLEUUID charUUID_Read("FFE4");

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // Kolla om enheten har rätt Service UUID
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            Serial.print("Hittade JBD Batteri: ");
            Serial.println(advertisedDevice.toString().c_str());
        }
    }
};

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
  // Initiera BLE
  BLEDevice::init("ESP32-C6-Monitor");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
 Serial.println("Skannar efter ALLA enheter...");
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  // Vi skannar i 5 sekunder
  BLEScanResults *foundDevices = pBLEScan->start(5, false);
  
  gfx->fillScreen(0x0000);
  gfx->setCursor(10, 10);
  gfx->setTextColor(0xFFE0); // Gul
  gfx->setTextSize(2);
  gfx->println("ENHETER FUNNA:");
  gfx->setTextSize(1);

  int y = 40;
  for (int i = 0; i < foundDevices->getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    
    // Vi skriver ut ALLA enheter vi hittar för att testa
    String name = device.getName().c_str();
    if (name.length() == 0) name = "Okand enhet"; // Många enheter har inget namn

    gfx->setCursor(10, y);
    gfx->setTextColor(0x07E0); // Grön
    gfx->print(name);
    
    // Skriv ut signalstyrka (RSSI)
    gfx->setTextColor(0xF800); // Rödaktig
    gfx->printf(" (%d dBm)", device.getRSSI());
    
    y += 10;
    gfx->setCursor(10, y);
    gfx->setTextColor(0xAAAA); // Grå
    gfx->println(device.getAddress().toString().c_str());
    
    y += 5; // Flytta ner till nästa enhet

    // Stoppa om vi fyller skärmen
    if (y > 170) break;
  }

  pBLEScan->clearResults();
  delay(1000);
}