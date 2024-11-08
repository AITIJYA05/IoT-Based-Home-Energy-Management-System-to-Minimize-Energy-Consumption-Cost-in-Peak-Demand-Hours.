// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "TMPL6qgbMfr8s"
#define BLYNK_TEMPLATE_NAME "Load Monitoring"
#define BLYNK_AUTH_TOKEN "0DJ6-xOTWN1dEPONHrijUyoB8FHpRqTe"


// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial


#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "CUET--Students";
char pass[] = "1020304050";

BlynkTimer timer;

//13,32,26,  15  ,34,35   relay
//33   sensor
//14,27,25    push

#define SIM800L_RX 17
#define SIM800L_TX 16

#define button1_pin 14
#define button2_pin 27
#define button3_pin 25


#define relay1_pin 13
#define relay2_pin 32
#define relay3_pin 23
#define relay4_pin 33
#define relay5_pin 18
#define solar_pin 15

int relay1_state = 0;
int relay2_state = 0;
int relay3_state = 0;
int relay4_state = 0;
int relay5_state = 0;
int solar_state = 0;

int price_alart = 0;
int load_alart = 0;

float price_per_day = 0.0;
int p = 0;
int p_2 = 0;
//---------------------------------------
static bool price_over_8 = false;
static bool price_over_16 = false;
static bool p_2_peak = false;
static bool p_2_switch = false;
//Change the virtual pins according the rooms
#define button1_vpin V1
#define button2_vpin V2
#define button3_vpin V3
#define button4_vpin V5
#define button5_vpin V6
#define solar_vpin V4


//------------------------------------------------------------------------------
// This function is called every time the device is connected to the Blynk.Cloud
// Request the latest state from the server
BLYNK_CONNECTED() {
  Blynk.syncVirtual(button1_vpin);
  Blynk.syncVirtual(button2_vpin);
  Blynk.syncVirtual(button3_vpin);
  Blynk.syncVirtual(button4_vpin);
  Blynk.syncVirtual(button5_vpin);
  Blynk.syncVirtual(solar_vpin);
}

//--------------------------------------------------------------------------
// This function is called every time the Virtual Pin state change
//i.e when web push switch from Blynk App or Web Dashboard
BLYNK_WRITE(button1_vpin) {
  relay1_state = param.asInt();
  digitalWrite(relay1_pin, relay1_state);
}
//--------------------------------------------------------------------------
BLYNK_WRITE(button2_vpin) {
  relay2_state = param.asInt();
  digitalWrite(relay2_pin, relay2_state);
}
//--------------------------------------------------------------------------
BLYNK_WRITE(button3_vpin) {
  relay3_state = param.asInt();
  digitalWrite(relay3_pin, relay3_state);
}
//--------------------------------------------------------------------------
BLYNK_WRITE(button4_vpin) {
  relay4_state = param.asInt();
  digitalWrite(relay4_pin, relay4_state);
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
BLYNK_WRITE(button5_vpin) {
  relay5_state = param.asInt();
  digitalWrite(relay5_pin, relay5_state);
}
//--------------------------------------------------------------------------
BLYNK_WRITE(solar_vpin) {
  solar_state = param.asInt();
  digitalWrite(solar_pin, solar_state);
}


void setup() {
  // Debug console
  Serial.begin(115200);
  //--------------------------------------------------------------------
  pinMode(button1_pin, INPUT_PULLUP);
  pinMode(button2_pin, INPUT_PULLUP);
  pinMode(button3_pin, INPUT_PULLUP);
  //--------------------------------------------------------------------
  pinMode(relay1_pin, OUTPUT);
  pinMode(relay2_pin, OUTPUT);
  pinMode(relay3_pin, OUTPUT);
  pinMode(relay4_pin, OUTPUT);
  pinMode(relay5_pin, OUTPUT);
  pinMode(solar_pin, OUTPUT);
  //--------------------------------------------------------------------
  //During Starting all Relays should TURN OFF
  digitalWrite(relay1_pin, HIGH);
  digitalWrite(relay2_pin, HIGH);
  digitalWrite(relay3_pin, HIGH);
  digitalWrite(relay4_pin, HIGH);
  digitalWrite(relay5_pin, HIGH);
  digitalWrite(solar_pin, HIGH);


  Serial2.begin(9600, SERIAL_8N1, SIM800L_TX, SIM800L_RX);
  Serial2.print("AT+CMGF=1\r");  //SMS text mode
  delay(1000);
  Serial2.print("AT");
  //--------------------------------------------------------------------
  Blynk.begin(auth, ssid, pass);
  //--------------------------------------------------------------------
}

void loop() {
  while (Serial2.available()) {
    Serial.print(char(Serial2.read()));
  }
  while (Serial.available()) {
    Serial2.print(char(Serial.read()));
  }

  Blynk.run();
  timer.run();
  listen_push_buttons();

  power_calculate();
  power_calculate_2();


  price_per_day = ((p * 4.14) / 1000) * 24;
  Serial.println(price_per_day);
  Blynk.virtualWrite(V0, price_per_day);
  Blynk.virtualWrite(V7, p);

  if (price_per_day > 8 && !price_over_8) {
    price_over_8 = true;
    price_increase();
    Serial.println("price_over_8");
  }
  if (price_per_day < 8) {
    price_over_8 = false;
  }

  if (price_per_day > 16 && !price_over_16) {
    price_over_16 = true;
    price_too_high();
    Serial.println("price_over_16");
  }
  if (price_per_day < 16) {
    price_over_16 = false;
  }

  if (p_2 > 50 && !p_2_peak && price_per_day > 16) {
    p_2_peak = true;
    peak_hour();
    Serial.println("peak_hour");
  }
  if (p_2 < 50) {
    p_2_peak = false;
  }

  if (p_2 > 90 && !p_2_switch && price_per_day > 16) {
    solar_state = 1;
    digitalWrite(solar_pin, solar_state);
    Blynk.virtualWrite(solar_vpin, solar_state);  //update button state
    p_2_switch = true;
    power_switch();
    Serial.println("power_switch");
  }
  if (p_2 < 90) {
    p_2_switch = false;
  }
}


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void listen_push_buttons() {
  //--------------------------------------------------------------------------
  if (digitalRead(button1_pin) == LOW) {
    delay(200);
    control_relay(1);
    Blynk.virtualWrite(button1_vpin, relay1_state);  //update button state
  }
  //--------------------------------------------------------------------------
  else if (digitalRead(button2_pin) == LOW) {
    delay(200);
    control_relay(2);
    Blynk.virtualWrite(button2_vpin, relay2_state);  //update button state
  }
  //--------------------------------------------------------------------------
  else if (digitalRead(button3_pin) == LOW) {
    delay(200);
    control_relay(3);
    Blynk.virtualWrite(button3_vpin, relay3_state);  //update button state
  }
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM




//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void control_relay(int relay) {
  //------------------------------------------------
  if (relay == 1) {
    relay1_state = !relay1_state;
    digitalWrite(relay1_pin, relay1_state);
    Serial.print("Relay1 State = ");
    Serial.println(relay1_state);
    delay(50);
  }
  //------------------------------------------------
  else if (relay == 2) {
    relay2_state = !relay2_state;
    digitalWrite(relay2_pin, relay2_state);
    delay(50);
  }
  //------------------------------------------------
  else if (relay == 3) {
    relay3_state = !relay3_state;
    digitalWrite(relay3_pin, relay3_state);
    delay(50);
  }
  //------------------------------------------------
  else if (relay == 4) {
    relay4_state = !relay4_state;
    digitalWrite(relay4_pin, relay4_state);
    delay(50);
  }
  //------------------------------------------------
  else if (relay == 5) {
    relay5_state = !relay5_state;
    digitalWrite(relay5_pin, relay5_state);
    delay(50);
  }
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void power_calculate() {
  p = 0;
  if (relay1_state == 1) {
    p = p + 60;
  }
  if (relay2_state == 1) {
    p = p + 60;
  }
  if (relay3_state == 1) {
    p = p + 60;
  }
}

void power_calculate_2() {
  p_2 = 0;
  if (relay4_state == 1) {
    p_2 = p_2 + 60;
  }
  if (relay5_state == 1) {
    p_2 = p_2 + 60;
  }
}

void price_increase() {
  Serial2.print("AT+CMGF=1\r");  //Set the module to SMS mode
  delay(100);
  Serial2.print("AT+CMGS=\"+8801949321123\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  Serial2.print("Load Consumption Increased!");  //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  Serial2.print((char)26);  // (required according to the datasheet)
  delay(500);
  Serial2.println();
  delay(500);
}

void price_too_high() {
  Serial2.print("AT+CMGF=1\r");  //Set the module to SMS mode
  delay(100);
  Serial2.print("AT+CMGS=\"+8801949321123\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  Serial2.print("All Connected Loads are turned ON! Reduce load Consumption!");  //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  Serial2.print((char)26);  // (required according to the datasheet)
  delay(500);
  Serial2.println();
  delay(500);
}

void peak_hour() {
  Serial2.print("AT+CMGF=1\r");  //Set the module to SMS mode
  delay(100);
  Serial2.print("AT+CMGS=\"+8801949321123\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  Serial2.print("Peak Hour Started! Do you turn off any LOAD!");  //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  Serial2.print((char)26);  // (required according to the datasheet)
  delay(500);
  Serial2.println();
  delay(500);
}

void power_switch() {
  Serial2.print("AT+CMGF=1\r");  //Set the module to SMS mode
  delay(100);
  Serial2.print("AT+CMGS=\"+8801949321123\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  Serial2.print("Overloaded!!! Power Supply is Automatically Switching to Integrated Renewable Source!");  //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  Serial2.print((char)26);  // (required according to the datasheet)
  delay(500);
  Serial2.println();
  delay(500);
}
