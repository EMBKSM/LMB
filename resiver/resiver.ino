#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 433E6

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

uint8_t device_id;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
    Serial.begin(115200);
    
    device_id = (ESP.getEfuseMac()%254) +1;

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    
    Serial.println("LoRa Receiver Test");
    
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    
    Serial.println("LoRa Initializing OK!");
    delay(2000);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint8_t receivedID = LoRa.peek();
    
    if (receivedID == 0) {
      LoRa.read();
      int send_device = LoRa.readString().toInt();
      sendRSSI(send_device);

      sendCommand(0, receivedID);
    } else if (receivedID == device_id) {
      LoRa.read();
      String receivedData = LoRa.readString();
      displayGPSData(receivedData);
    }
  }
}

void sendRSSI(int send_device_id) {
  LoRa.beginPacket();
  LoRa.write(send_device_id);
  LoRa.print(device_id);
  LoRa.print(",");
  LoRa.print(LoRa.packetRssi());
  LoRa.print(",1");
  LoRa.endPacket();
}

void displayGPSData(String data) {
    int commaIndex = data.indexOf(',');
    if (commaIndex == -1) return;
    
    String latitude = data.substring(0, commaIndex);
    String longitude = data.substring(commaIndex + 1);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Latitude: ");
    display.println(latitude);
    display.print("Longitude: ");
    display.println(longitude);
    display.display();
}

void sendCommand(uint8_t targetID, uint8_t command) {
  LoRa.beginPacket();
  LoRa.write(targetID);
  LoRa.print(command);
  LoRa.endPacket();
}