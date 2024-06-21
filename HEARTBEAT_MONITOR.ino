#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Initialize the LCD with the I2C address 0x27 and dimensions (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize SoftwareSerial for GSM module (TX, RX)
SoftwareSerial gsmSerial(2, 3);

// Define pins for the potentiometers and buzzer
const int heartRatePin = A0; // Potentiometer 1 for heart rate
const int spO2Pin = A1; // Potentiometer 2 for SpO2
const int buzzerPin = 8; // Buzzer pin

// Define thresholds for abnormalities
const int heartRateThreshold = 100; // Heart rate above 100 bpm is abnormal
const int spO2Threshold = 95; // SpO2 below 95% is abnormal

// State variables to track the status of alerts
bool heartRateAlertSent = false;
bool spO2AlertSent = false;

void setup() {
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight

  Serial.begin(9600); // Initialize serial communication for debugging
  gsmSerial.begin(9600); // Initialize GSM serial communication

  // Set up the buzzer pin
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off initially

  // Test GSM module with basic AT command
  gsmSerial.println("AT");
  delay(1000);
  if (gsmSerial.available()) {
    Serial.println("GSM Module Ready");
    while (gsmSerial.available()) {
      Serial.write(gsmSerial.read());
    }
  } else {
    Serial.println("GSM Module Not Responding");
  }

  lcd.setCursor(0, 0);
  lcd.print("SpO2 & HR Monitor");
  delay(1000); // Short initializing message delay

  lcd.clear(); // Clear the LCD screen

  pinMode(heartRatePin, INPUT);
  pinMode(spO2Pin, INPUT);
}

void loop() {
  // Read analog values from the potentiometers
  int heartRateValue = analogRead(heartRatePin);
  int spO2Value = analogRead(spO2Pin);

  // Convert potentiometer values to usable data
  int heartRate = map(heartRateValue, 0, 1023, 60, 120); // Map to a typical heart rate range (60 - 120 bpm)
  int spO2 = map(spO2Value, 0, 1023, 90, 100); // Map to a typical SpO2 range (90% - 100%)

  // Display the values on the LCD
  lcd.setCursor(0, 0);
  lcd.print("SpO2: ");
  lcd.print(spO2);
  lcd.print("%   "); // Add spaces to ensure the display is cleared properly

  lcd.setCursor(0, 1);
  lcd.print("HR: ");
  lcd.print(heartRate);
  lcd.print(" bpm "); // Add spaces to ensure the display is cleared properly

  // Check for abnormalities and handle alerts
  if (heartRate > heartRateThreshold && !heartRateAlertSent) {
    sendSMSAlert("Alert! Heart rate above 100 bpm: " + String(heartRate) + " bpm");
    heartRateAlertSent = true; // Mark alert as sent
    soundAlarm(); // Sound the buzzer for the abnormal heart rate
  } else if (heartRate <= heartRateThreshold) {
    heartRateAlertSent = false; // Reset alert status when heart rate returns to normal
  }

  if (spO2 < spO2Threshold && !spO2AlertSent) {
    sendSMSAlert("Alert! SpO2 below 95%: " + String(spO2) + "%");
    spO2AlertSent = true; // Mark alert as sent
    soundAlarm(); // Sound the buzzer for the abnormal SpO2
  } else if (spO2 >= spO2Threshold) {
    spO2AlertSent = false; // Reset alert status when SpO2 returns to normal
  }

  // Turn off the buzzer if no abnormal conditions are detected
  if (!heartRateAlertSent && !spO2AlertSent) {
    digitalWrite(buzzerPin, LOW); // Ensure the buzzer is off
  }

  // Update display every 200 ms
  delay(200);
}

void sendSMSAlert(String message) {
  Serial.println("Sending SMS: " + message);
  
  gsmSerial.println("AT+CMGF=1"); // Set GSM to text mode
  delay(100);
  gsmSerial.println("AT+CMGS=\"+1234567890\""); // Replace with recipient's phone number
  delay(100);
  gsmSerial.print(message); // Send the alert message
  delay(100);
  gsmSerial.write(26); // Ctrl+Z to send the SMS
  delay(5000); // Wait for the SMS to be sent

  // Print the GSM response
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}

void soundAlarm() {
  digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
  delay(1000); // Sound the alarm for 1 second
  digitalWrite(buzzerPin, LOW); // Turn off the buzzer
}
