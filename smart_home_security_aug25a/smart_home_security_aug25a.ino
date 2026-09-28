#include "arduino_secrets.h"
/* 
 The following variables are automatically generated and updated when changes are made to the Thing
 String msg;
 float boardHumid;
 float boardTemp;
 float gasValue;
 bool led;
 Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
 which are called when their values are changed from the Dashboard.
 These functions are generated with the Thing and added at the end of this sketch.
*/
#include "thingProperties.h"
#include <DHT.h>
#include <DHT_U.h>
#include <string.h>
#include <stdlib.h>

//defining pins
#define SOUND_OUT_PIN 23
#define DHTPIN 15 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11 sensor type
DHT dht(DHTPIN, DHTTYPE);
#define myLED 19          // bulb relay
#define FIRE_OUT_PIN 4
#define PIR_OUT_PIN 22
#define VIBR_OUT_PIN 13
#define S2_PIN 21
#define S1_PIN 5
#define S0_PIN 18
#define MUX_OUT_PIN 34
#define GAS_IN_PIN 35
#define FAN_PIN 26         // fan transistor driver
float temp_thr = 40.0;     // fan turns on above this temperature (°C)

//global variables
int pins_used = 3;         // MUX channels 0-3 in use (fire, PIR, sound, vibration)
int sound_value = 0;
int fire_value = 0;
int pir_value = 0;
int vibr_value = 0;
int mux_thr = 500;         // shared threshold for all MUX channels (0-4095 ADC range)
int current_pin = 0;
float mux_read;
int digital_mux_read;
int set_pwd = 213456;
int pwd;

void setup() {
  Serial.begin(9600);
  delay(1500);
  initProperties();
  dht.begin();

  //PIN MODES
  pinMode(myLED, OUTPUT);
  pinMode(SOUND_OUT_PIN, OUTPUT);
  pinMode(FIRE_OUT_PIN, OUTPUT);
  pinMode(PIR_OUT_PIN, OUTPUT);
  pinMode(VIBR_OUT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  //MUX SETUP
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  digitalWrite(S0_PIN, LOW);
  digitalWrite(S1_PIN, LOW);
  digitalWrite(S2_PIN, LOW);

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();

  //--------SENSORS CONNECTED DIRECTLY
  boardHumid = dht.readHumidity();
  boardTemp = dht.readTemperature();

  // ---- fan auto-control (temperature threshold) ----
  if (boardTemp > temp_thr) {
    digitalWrite(FAN_PIN, HIGH);
  } else {
    digitalWrite(FAN_PIN, LOW);
  }

  // gas sensor
  gasValue = analogRead(GAS_IN_PIN);

  //-------SENSORS CONNECTED TO MUX
  // pin mapping:
  // 0: fire sensor (DO pin wired into Y0 — still read via analogRead through the MUX)
  // 1: PIR sensor
  // 2: sound sensor
  // 3: vibration sensor
  mux_read = readMux(current_pin);
  digital_mux_read = (mux_read > mux_thr) ? 1 : 0;
  Serial.printf("CHANNEL: %d, digital_mux_read: %d, mux_read: %f\n", current_pin, digital_mux_read, mux_read);

  switch (current_pin) {
    case 0:
      // fire sensor
      // NOTE: this sensor only has a DO pin (no AO). DO idles HIGH/LOW depending
      // on the module. If the fire indicator behaves backwards on your bench
      // test (triggers when there's NO fire), flip the condition below to
      // "if (fire_value == 0)" instead — that means your module is active-LOW.
      fire_value = digital_mux_read;
      if (fire_value == 1) {
        digitalWrite(FIRE_OUT_PIN, HIGH);
        msg = "fireeeeeeeeeeeeeeeeeeeeeeeeeee!!!";
        delay(50);
      } else {
        digitalWrite(FIRE_OUT_PIN, LOW);
        msg = "";
      }
      break;

    case 1:
      // PIR sensor
      pir_value = digital_mux_read;
      if (pir_value == HIGH) {
        digitalWrite(PIR_OUT_PIN, HIGH);
        delay(50);
        digitalWrite(PIR_OUT_PIN, LOW);
        msg = "chor asce";
        delay(50);
      } else {
        digitalWrite(PIR_OUT_PIN, LOW);
        delay(50);
        msg = "";
      }
      break;

    case 2:
      // sound sensor
      sound_value = digital_mux_read;
      if (sound_value == HIGH) {
        digitalWrite(SOUND_OUT_PIN, HIGH);
        msg = "Baby Crying!";
        delay(50);
        digitalWrite(SOUND_OUT_PIN, LOW);
      } else {
        digitalWrite(SOUND_OUT_PIN, LOW);
        msg = "";
      }
      break;

    case 3:
      // vibration sensor
      vibr_value = digital_mux_read;
      if (vibr_value == HIGH) {
        digitalWrite(VIBR_OUT_PIN, HIGH);
        msg = "vault security possibly compromised, vibration in vault";
        delay(50);
        digitalWrite(VIBR_OUT_PIN, LOW);
      } else {
        digitalWrite(VIBR_OUT_PIN, LOW);
        msg = "";
      }
      break;

    default:
      Serial.println("unknown mux pin");
  }

  delay(10);
  //updating pin
  if (current_pin >= pins_used) current_pin = 0;
  else current_pin += 1;
}

float readMux(int channel) {
  int controlPin[] = { S0_PIN, S1_PIN, S2_PIN };
  int muxChannel[8][3] = {
    { 0, 0, 0 },  //channel 0
    { 1, 0, 0 },  //channel 1
    { 0, 1, 0 },  //channel 2
    { 1, 1, 0 },  //channel 3
    { 0, 0, 1 },  //channel 4
    { 1, 0, 1 },  //channel 5
    { 0, 1, 1 },  //channel 6
    { 1, 1, 1 },  //channel 7
  };

  for (int i = 0; i < 3; i++) {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  int val = analogRead(MUX_OUT_PIN);
  float voltage = val;
  return voltage;
}

void onLedChange() {
  Serial.print("onLedChange called, led = ");
  Serial.println(led);
  if (led) {
    digitalWrite(myLED, HIGH);
  } else {
    digitalWrite(myLED, LOW);
  }
}

void onMsgChange() {
  int msg_int = atoi(msg.c_str());
  Serial.println(msg_int);

  if (msg_int == set_pwd) {
    msg = "access granted";
  }
}
