#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"

/* ---------- PIN DEFINITIONS ---------- */
#define SOIL_PIN       34  // Analog
#define VIBRATION_PIN  27  // Digital
#define TILT_PIN       26  // Digital
//#define FLOW_PIN       25  // Interrupt capable

#define HX_DT          32
#define HX_SCK         33

/* ---------- LCD ---------- */
// Try 0x27 if 0x3F does not work
//LiquidCrystal_I2C lcd(0x27, 16, 2); 

/* ---------- HX711 ---------- */
HX711 scale;

/* ---------- VARIABLES ---------- */
//volatile int flowPulses = 0;
//float weight = 0;

/* ---------- THRESHOLDS ---------- */
// Note: ESP32 ADC is 0-4095. 
// For most soil sensors: Dry > 3000, Wet < 1500.
#define SOIL_THRESHOLD     2000 //
#define WEIGHT_THRESHOLD   0.8   // >kg
//#define FLOW_THRESHOLD     15    // pulses per second (Wind simulation)

//void IRAM_ATTR flowISR() {
//  flowPulses++;
//}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("--- Landslide System Initializing ---");

  // Vibration and Tilt often need Pullup/Pulldown to prevent ghost triggers
  pinMode(VIBRATION_PIN, INPUT_PULLDOWN); 
  pinMode(TILT_PIN, INPUT_PULLUP); // Common for tilt switches to ground when tilted
  pinMode(FLOW_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowISR, RISING);

  // Start I2C for ESP32 (SDA=21, SCL=22)
  //Wire.begin(21, 22);

  //lcd.init();
  //lcd.backlight();
  //lcd.setCursor(0, 0);
  //lcd.print("Landslide Sys");
  //lcd.setCursor(0, 1);
  //lcd.print("Calibrating...");

  // HX711 Setup
  scale.begin(HX_DT, HX_SCK);
  if (scale.wait_ready_timeout(1000)) {
      scale.set_scale(2280.f); // Replace with your real calibration factor - converts raw values into kg
      scale.tare();            // Sets current weight to zero
      Serial.println("HX711 Ready");
  } else {
      Serial.println("HX711 not found.");
  }

  delay(2000);
  lcd.clear();
}

void loop() {
  // 1. Read Sensors
  int soil      = analogRead(SOIL_PIN);
  int vibration = digitalRead(VIBRATION_PIN);
  int tilt      = digitalRead(TILT_PIN); // Low usually means tilted if using Pullup

  // 2. Measure Flow/Wind for 1 second
  // noInterrupts();
  // int currentPulses = flowPulses;
  // flowPulses = 0;
  // interrupts();

  // 3. Read Weight
  if (scale.is_ready()) {
    weight = scale.get_units(3); // Average of 3 readings
  }

  // 4. Alert Logic
  bool alert    = false; // Initially assumes system is stable
  String reason = "";

  // Adjust these conditions based on your specific sensor wiring
  if (soil < SOIL_THRESHOLD)          { alert = true; reason = "Saturation"; }
  if (vibration == HIGH)              { alert = true; reason = "Vibration";  }
  if (tilt == LOW)                    { alert = true; reason = "Tilt/Shift"; }
  if (weight > WEIGHT_THRESHOLD)      { alert = true; reason = "Mass Shift"; }
  //if (currentPulses > FLOW_THRESHOLD) { alert = true; reason = "High Wind"; }

  /* ---------- SERIAL DEBUG ---------- */
  Serial.printf("S:%d | V:%d | T:%d | W:%.2f | F:%d\n",
                soil, vibration, tilt, weight, currentPulses);

  /* ---------- LCD OUTPUT ---------- */
  lcd.setCursor(0, 0);
  if (alert) {
    lcd.print("ALERT: ");
    lcd.print(reason);
    Serial.print("!!! ALERT: ");
    Serial.println(reason);
  } else {
    lcd.print("Status: STABLE ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Soil:"); lcd.print(soil);
  lcd.print(" W:");   lcd.print(weight, 1);

  delay(1000); // Main loop delay
}
