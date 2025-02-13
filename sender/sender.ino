#include <queue>
#include <vector>
#include <memory>

#include <SPI.h>
#include <LoRa.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define CALL_BUTTON 32

#define RX_PIN 17  // GPS TX -> ESP32 RX
#define TX_PIN 13  // GPS RX -> ESP32 TX

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define TIMEOUT 3000

using namespace std;

uint8_t device_id;
unsigned long lastPacketTime = 0;

HardwareSerial gpsSerial(2);  // ESP32의 UART2 사용
TinyGPSPlus gps;

typedef struct device{
  uint8_t id;
  int rssi;
};

struct CompareUnique {
    bool operator()(const unique_ptr<device>& a, const unique_ptr<device>& b) {
        return a->rssi > b->rssi;
    }
};

priority_queue<unique_ptr<device>, vector<unique_ptr<device>>, CompareUnique> select_device;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  device_id = (ESP.getEfuseMac()%254) +1;

  pinMode(CALL_BUTTON, INPUT_PULLUP);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER ");
  display.display();
  
  Serial.println("LoRa Sender Test");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

void loop() {

  if (digitalRead(CALL_BUTTON) == LOW) {
    LoRa.beginPacket();
    LoRa.write(0);
    LoRa.print(device_id);
    LoRa.endPacket();
  }
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint8_t targetID = LoRa.read();
    if (targetID == device_id) {
      String datas = LoRa.readString();
      slice_string(datas);
      lastPacketTime = millis();
    }
  }
  if (!select_device.empty() && millis() - lastPacketTime > TIMEOUT) {
    while (gpsSerial.available() == 0) {
        delay(10);
    }
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }
    if (gps.location.isUpdated()) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print("Latitude: ");
      display.println(gps.location.lat(), 6);
      display.print("Longitude: ");
      display.println(gps.location.lng(), 6);
      display.display();


      String temp = "";
      temp += gps.location.lat();
      temp += ',';
      temp += gps.location.lng();
      sendCommand(select_device.top()->id, temp);

      while(!select_device.empty()){
        select_device.pop();
      }
    }
  }
  gpsSerial.flush();
}

void sendCommand(uint8_t targetID, String command) {
  LoRa.beginPacket();
  LoRa.write(targetID);
  LoRa.print(command);
  LoRa.endPacket();
}

void slice_string(String ss){
  auto new_device = make_unique<device>();
  
  int pos = ss.indexOf(',');
  new_device->id = ss.substring(0, pos).toInt();
  String temp = ss.substring(pos+1);

  pos = temp.indexOf(',');
  new_device->rssi = temp.substring(0, pos).toInt();
  temp = temp.substring(pos+1);

  if(temp.toInt() == 1){
    select_device.push(move(new_device));
  }
}