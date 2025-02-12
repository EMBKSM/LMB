#include <queue>
#include <vector>
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

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CALL_BUTTON 32

using namespace std;

uint64_t device_id = ESP.getEfuseMac();

typedef struct device{
  uint64_t id;
  int rssi;
};

struct Compare {
    bool operator()(const device* a, const device* b) {
        return a->rssi > b->rssi;
    }
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);

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
    LoRa.endPacket();
  }

  priority_queue<device*, vector<device*>, Compare> select_device;
}