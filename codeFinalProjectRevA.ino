#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Servo.h>


// For Arduino UNO: pin 2 = sensor TX (yelloq), pin 3 = sensor RX (green)
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
Servo myServo;

uint8_t id;  // used for enroll/delete IDs

// ------------------- Setup -------------------
void setup() {
  myServo.attach(6);
  pinMode(6, OUTPUT);
  Serial.begin(9600);
  while (!Serial);  // wait for Serial (on Leonardo etc.)
  delay(100);
  Serial.println("Adafruit Fingerprint Sensor Combo Test");

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  finger.getParameters();
  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount);
  Serial.println(" templates");
  Serial.println("\nCommands: ");
  Serial.println("Type '1' to enroll a fingerprint");
  Serial.println("Type '2' to search for a fingerprint");
  Serial.println("Type '3' to delete a fingerprint");
}

// ------------------- Main Loop -------------------
void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == '1') {
      Serial.println("Enter ID (1–127) to save this fingerprint as:");
      id = readnumber();
      if (id == 0) {
        Serial.println("ID 0 not allowed.");
        return;
      }
      while (!getFingerprintEnroll());
    }
    else if (c == '2') {
      if (getFingerprintID() != -1) {
        Serial.println("Fingerprint match!");
        myServo.write(180);   // move to 180°
        delay(3000);         // hold for 3 seconds
        myServo.write(0);    // move back to 0°
      }
    }
    else if (c == '3') {
      Serial.println("Enter ID (1–127) to delete:");
      id = readnumber();
      deleteFingerprint(id);
    }
  }
}

// ------------------- Helper Functions -------------------
uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

// Search for fingerprint
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) { Serial.println("No finger detected"); return -1; }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) { Serial.println("Image conversion failed"); return -1; }

  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) { Serial.println("No match found"); return -1; }

  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence "); Serial.println(finger.confidence);
  return finger.fingerID;
}

// Enroll new fingerprint
uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Enrolling as ID #"); Serial.println(id);

  // --- First scan ---
  Serial.println("Place finger on sensor...");
  delay(500);
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) continue; // still waiting
    if (p == FINGERPRINT_PACKETRECIEVEERR) continue;
    if (p == FINGERPRINT_IMAGEFAIL) continue;
  }
  Serial.println("Image taken");

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Image conversion failed, try again");
    return p;
  }

  // --- Ask for removal ---
  Serial.println("Remove finger...");
  delay(500);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  // --- Second scan ---
  Serial.println("Place same finger again...");
  delay(500);
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_PACKETRECIEVEERR) continue;
    if (p == FINGERPRINT_IMAGEFAIL) continue;
  }
  Serial.println("Second image taken");

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Second image conversion failed, try again");
    return p;
  }

  // --- Create model ---
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprints did not match, start over.");
    return p;
  }

  // --- Store template ---
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprint enrolled successfully!");
  } else {
    Serial.println("Failed to store fingerprint.");
  }
  return p;
}


// Delete a fingerprint
uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else {
    Serial.print("Error deleting ID #"); Serial.println(id);
  }
  return p;
}
