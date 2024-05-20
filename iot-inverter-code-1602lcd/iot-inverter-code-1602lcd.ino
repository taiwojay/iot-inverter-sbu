#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

#define wifiLED 2
#define batteryPin 33
#define relayPin 4
#define buzzPin 32


#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#if defined(ESP32)
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#elif defined(ESP8266)
//PZEM004Tv30 pzem(Serial1);
#else
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

const char* ssid = "Mayowa";
const char* password = "mayowa3553";

WiFiClient  client;

unsigned long myChannelNumber = 3;
const char * myWriteAPIKey = "A4TIYXBVOH3HJZA0";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Define the initial value of the variable "power"
int maxPower = 400;
// Define the decrement value
int decrement = 100;
// Define the interval for decreasing power (30 minute)
unsigned long interval = 1800000;
// Variable to store the last time power was decreased
unsigned long lastDecreaseTime = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  lcd.init();                      // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WELCOME");
  lcd.setCursor(0, 1);
  lcd.print("USER");

  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting");
    Serial.print(".");
    delay(2000);
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
  pinMode(wifiLED, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(batteryPin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy() * 1000;
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  float analogValue = 0;
  float batteryVoltage = 0;
  float batteryVoltagePercent = 0;

  int loadStatus = 4;

  analogValue = analogRead(batteryPin);
  batteryVoltage = (analogValue / 4095 * 3.3) * 5;

  // Get the current time
  unsigned long currentTime = millis();

  if (maxPower == 400) {
    loadStatus = 4;
  } else if ( maxPower == 300) {
    loadStatus = 3;
  } else if ( maxPower == 200) {
    loadStatus = 2;
  } else if ( maxPower == 100) {
    loadStatus = 1;
  } 
  Serial.print("Load status: "); Serial.println(loadStatus);

  // Check if it's time to decrease power
  if (currentTime - lastDecreaseTime >= interval) {
    // Decrease the value of "power" by the decrement value
    maxPower -= decrement;

    // Print the current value of "power"
    Serial.print("Current power value: ");
    Serial.println(maxPower);

    // If "power" reaches 0, reset it to 150
    if (maxPower <= 0) {
      maxPower = 80;
    }
    // Update the last decrease time
    lastDecreaseTime = currentTime;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    displayAllValues(voltage, current, power, energy, frequency, pf, batteryVoltage);
    displayBatteryLevel(batteryVoltage, batteryVoltagePercent);
    if (power > maxPower) {
      buzzNTimes(5);
    }
    Serial.println("\nConnected.");
  }
  ThingSpeak.setField(1, voltage);
  ThingSpeak.setField(2, current);
  ThingSpeak.setField(3, power);
  ThingSpeak.setField(4, energy);
  ThingSpeak.setField(5, batteryVoltage);
  ThingSpeak.setField(6, maxPower);


  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Voltage upload successful");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  displayAllValues(voltage, current, power, energy, frequency, pf, batteryVoltage);
  displayBatteryLevel(batteryVoltage, batteryVoltagePercent);
  displayWiFiStatus();
  if (power > maxPower) {
    buzzNTimes(5);
  }
  Serial.print("Max Power: "); Serial.println(maxPower);
  delay(5000);
}


void buzzNTimes(int num) {
  for (int i = 0; i < num; i++) {
    digitalWrite(buzzPin, HIGH);
    delay(1000);
    digitalWrite(buzzPin, LOW);
    delay(200);
  }
}

void displayBatteryLevel(float bvl, float bvlp) {
  Serial.print("Battery voltage:"); Serial.print(bvl, 1); Serial.println("V");
  Serial.print(bvlp, 1); Serial.println("%");
  lcd.setCursor(5, 1);
  lcd.print(bvl, 1); lcd.println("V ");
  if (bvlp > 75) {
    lcd.setCursor(10, 0);
    lcd.print("4");
  } else if (bvlp < 75 && bvlp > 50) {
    lcd.setCursor(10, 0);
    lcd.print("3");
  } else if (bvlp < 50 && bvlp > 25) {
    lcd.setCursor(10, 0);
    lcd.print("2");
  } else if (bvlp < 25 && bvlp > 10) {
    lcd.setCursor(10, 0);
    lcd.print("1");
  }
}

void displayAllValues(float volt, float curr, float powr, float eneg, float freq, float powfactor, float bvl) {
  if (isnan(volt)) {
    volt = 0;
    Serial.print("V:");
    Serial.println(volt);
    lcd.setCursor(0, 0);
    lcd.print("__"); lcd.print("V ");
    lcd.setCursor(0, 1);
    lcd.print("__"); lcd.println("W  ");
    lcd.setCursor(5, 0);
    lcd.print("__"); lcd.println("Wh  ");
    lcd.setCursor(5, 1);
    lcd.print("__"); lcd.println("V  ");

  } else if (isnan(curr)) {
    Serial.println("Error reading current");
  } else if (isnan(powr)) {
    Serial.println("Error reading power");
  } else if (isnan(eneg)) {
    Serial.println("Error reading energy");
  } else if (isnan(freq)) {
    Serial.println("Error reading frequency");
  } else if (isnan(powfactor)) {
    Serial.println("Error reading power factor");
  } else {
    Serial.print("Voltage: "); Serial.print(volt); Serial.println("V");
    Serial.print("I: "); Serial.print(curr); Serial.println("A");
    Serial.print("P: ");        Serial.print(powr);        Serial.println("W");
    Serial.print("E: ");       Serial.print(eneg, 3);     Serial.println("kWh");
    Serial.print("Frequency: ");    Serial.print(freq, 1); Serial.println("Hz");
    Serial.print("PF: ");           Serial.println(powfactor);

    lcd.setCursor(0, 0);
    lcd.print(volt, 0); lcd.print("V  ");
    lcd.setCursor(0, 1);
    lcd.print(powr, 0); lcd.println("W  ");
    lcd.setCursor(5, 0);
    lcd.print(eneg, 0); lcd.println("Wh  ");
  }
}

void displayWiFiStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Offline");
    digitalWrite(wifiLED, LOW);
    lcd.setCursor(11, 1);
    lcd.print("N");
  } else {
    Serial.println("Online");
    lcd.setCursor(11, 1);
    lcd.print("Y");
    digitalWrite(wifiLED, HIGH);
  }
}
