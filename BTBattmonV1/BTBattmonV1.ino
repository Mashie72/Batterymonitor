#include <Arduino_GFX_Library.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// 1. DataBus för Touch-versionen:
// DC=15, CS=14, SCK=1, MOSI=2
Arduino_DataBus* bus = new Arduino_ESP32SPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);

// 2. Display-inställning:
// RST=22, BL=23
Arduino_GFX* gfx = new Arduino_ST7789(
  bus, 22 /* RST */, 1 /* rotation */, false /* IPS */,
  172, 320, 34, 0, 34, 0);

// Vi sparar UUID för JBD-batteriet
static BLEUUID serviceUUID("FFE0");
static BLEUUID charUUID_Write("FFE3");
static BLEUUID charUUID_Read("FFE4");

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Kolla om enheten har rätt Service UUID
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.print("Hittade JBD Batteri: ");
      Serial.println(advertisedDevice.toString().c_str());
    }
  }
};

// JBD UUIDs


void setup() {
  Serial.begin(115200);
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  if (!gfx->begin()) {
    Serial.println("Kunde inte initiera GFX!");
  }
  bus->beginWrite();
  bus->writeCommand(0x36);
  bus->write(0x28);  //normal landscape flip
  bus->endWrite();
  gfx->fillScreen(0x0000);
  gfx->setTextColor(0xFFE0);  // Gul
  gfx->setTextSize(2);
  gfx->setCursor(10, 10);
  gfx->println("BMS SCANNER");

  BLEDevice::init("ESP32-C6-Monitor");
}

void loop() {
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);

  gfx->fillScreen(0x0000);
  gfx->setCursor(10, 10);
  gfx->setTextSize(2);
  gfx->println("SCANNING...");

  BLEScanResults* foundDevices = pBLEScan->start(4, false);

  for (int i = 0; i < foundDevices->getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);

    if (device.isAdvertisingService(serviceUUID)) {
      gfx->fillScreen(0x0000);
      gfx->setCursor(10, 10);
      gfx->printf("Connecting to:\n%s", device.getName().c_str());

      // Försök ansluta
      BLEClient* pClient = BLEDevice::createClient();
      if (pClient->connect(&device)) {
        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService) {
          BLERemoteCharacteristic* pWriteChar = pRemoteService->getCharacteristic(charUUID_Write);
          BLERemoteCharacteristic* pReadChar = pRemoteService->getCharacteristic(charUUID_Read);

          if (pWriteChar && pReadChar) {
            // Skicka "Läs Status"-kommando (Hex)
            uint8_t readCmd[] = { 0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77 };
            pWriteChar->writeValue(readCmd, sizeof(readCmd));

            // Läs svaret som en Arduino String
            String value = pReadChar->readValue();

            if (value.length() >= 20) {
              // Vi använder value[index] för att hämta byten
              // Vi måste casta till (uint8_t) för att matten ska bli rätt
              uint8_t b4 = (uint8_t)value[4];
              uint8_t b5 = (uint8_t)value[5];
              uint8_t b23 = (uint8_t)value[23];

              float volt = (b4 << 8 | b5) / 100.0;
              int soc = b23;

              gfx->fillScreen(0x0000);

              // Skriv ut Volt
              gfx->setCursor(10, 50);
              gfx->setTextSize(4);
              gfx->setTextColor(0x07E0);  // Grön
              gfx->printf("%.2f V", volt);

              // Skriv ut SOC
              gfx->setCursor(10, 120);
              gfx->setTextSize(3);
              gfx->setTextColor(0xFFFF);  // Vit
              gfx->printf("SOC: %d%%", soc);

              Serial.printf("Batteri data: %.2fV, %d%%\n", volt, soc);
            }
          }
        }
        pClient->disconnect();  // Viktigt att koppla ner så nästa batteri kan prata
      }
      delay(5000);  // Vänta 5 sek innan nästa skanning
    }
  }
  pBLEScan->clearResults();
}