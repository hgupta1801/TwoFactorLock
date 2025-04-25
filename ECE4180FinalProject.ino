#include <SPI.h>            // For MFRC522 communication
#include <MFRC522.h>        // MFRC522 library
#include <Wire.h>           // For I2C communication (MPR121)
#include <Adafruit_MPR121.h> // MPR121 library
#include <Servo.h>          // For Servo motor control

// --- RFID Configuration ---
#define RST_PIN 9           
#define SS_PIN 10           
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522
byte authorizedUID[] = {0xF4, 0xB6, 0x46, 0x03}; // The correct tag

// ---  Keypad Configuration ---
Adafruit_MPR121 cap = Adafruit_MPR121(); // Create MPR121 instance
uint16_t lastTouched = 0;
uint16_t currentTouched = 0; 

// --- Keypad Mapping ---
char keypadMap[12] = {
  '0', '1', '2',  
  '3', '4', '5',  
  '6', '7', '8',  
  '9', '*', '#'  //10 is *, 11 is #
};
const String correctPin = "1801"; // The required 4-digit PIN
String enteredPin = "";           
const int pinLength = 4;          // length of pin

// --- Servo Configuration ---
#define SERVO_PIN 3         
Servo myServo;              // Create servo object
const int LOCKED_ANGLE = 0;   
const int UNLOCKED_ANGLE = 90; 
const int UNLOCK_DURATION = 3000; // wait time between unlock and lock

// state machine
enum AccessState {
  STATE_WAITING_FOR_PIN,
  STATE_WAITING_FOR_TAG
};
AccessState currentState = STATE_WAITING_FOR_PIN; // Initial state

// Bit value macro definition for older IDE versions or boards
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

void setup() {
  // Initialize Serial communication
  Serial.begin(9600);
  while (!Serial) { delay(10); } // Wait for serial port connection
  Serial.println("Initializing Access Control System...");
  
  // Initialize Servo
  myServo.attach(SERVO_PIN); 
  myServo.write(LOCKED_ANGLE); 
  Serial.println("Servo Initialized and set to LOCKED position.");
  delay(5000); 

  // Initialize SPI for RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("MFRC522 Initialized.");

  // I2C for Keypad
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring or I2C address!");
    while (1);
  }
  Serial.println("MPR121 Initialized.");
  delay(100);
  lastTouched = cap.touched(); // Read initial state

  // Initial prompt (Directly printed)
  Serial.println("\nPlease enter the 4-digit PIN using touch pads (0-9). Use '*' to clear:");
}

void loop() {
  // State machine logic
  switch (currentState) {
    case STATE_WAITING_FOR_PIN:
      handlePinEntry();
      break;

    case STATE_WAITING_FOR_TAG:
      handleTagScan();
      break;
  }
}

// --- State Handling Functions ---

void handlePinEntry() {
  char key = readKeypad();
  if (key) {
    if (key == '*') {
      Serial.println("\nPIN Cleared. Enter PIN:");
      resetPinEntry();
    } else if (key == '#') {
      Serial.println("\n'#' pressed. Use '*' to clear PIN.");
    } else if (isDigit(key)) {
      if (enteredPin.length() < pinLength) {
        enteredPin += key;
        Serial.print(key);
        if (enteredPin.length() == pinLength) {
          Serial.println();
          if (enteredPin == correctPin) {
            Serial.println("PIN Correct. Please scan your tag.");
            currentState = STATE_WAITING_FOR_TAG;
          } else {
            Serial.println("Incorrect PIN. Try again.");
            resetPinEntry();
            // Prompt for PIN again (Directly printed)
            Serial.println("\nPlease enter the 4-digit PIN using touch pads (0-9). Use '*' to clear:");
          }
        }
      } else {
        Serial.println("\nMaximum PIN length reached. Use '*' to clear.");
      }
    }
  }
}

void handleTagScan() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return; // No card or failed read, try again next loop
  }

  // Card detected and read
  Serial.print("Card detected. UID: ");
  printUID(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();

  // Compare UID
  bool uidMatch = true;
  if (mfrc522.uid.size != sizeof(authorizedUID)) {
    uidMatch = false;
  } else {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] != authorizedUID[i]) {
        uidMatch = false;
        break;
      }
    }
  }

  // Grant or deny access
  if (uidMatch) {
    Serial.println("-------------------");
    Serial.println("ACCESS GRANTED");
    Serial.println("-------------------");
    Serial.println("Unlocking...");

    // --- Unlock Action ---
    myServo.write(UNLOCKED_ANGLE); // Move servo to unlocked position
    delay(UNLOCK_DURATION); // Keep unlocked for specified duration
    Serial.println("Locking...");

    // --- Lock Action ---
    myServo.write(LOCKED_ANGLE); // Move servo back to locked position
    delay(500); // Short delay after locking

  } else {
    // --- Denied Action ---
    Serial.println("Access Denied - Incorrect Tag.");
    delay(1000); // Optional delay
  }

  // Halt PICC & reset state regardless of access result
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  resetPinEntry();
  currentState = STATE_WAITING_FOR_PIN;
  // Prompt for PIN again (Directly printed)
  Serial.println("\nPlease enter the 4-digit PIN using touch pads (0-9). Use '*' to clear:");
}

// --- Helper Functions ---

char readKeypad() {
  currentTouched = cap.touched();
  char pressedKey = 0;
  for (uint8_t i = 0; i < 12; i++) {
    // Check if the pad *is* touched now AND *was not* touched last time
    if ((currentTouched & _BV(i)) && !(lastTouched & _BV(i))) {
      pressedKey = keypadMap[i];
      // Optional: Add debounce logic here if you get multiple reads for one press
      // delay(10); // Simple debounce delay
      break; // Process only one key press per loop iteration
    }
  }
  lastTouched = currentTouched; // Update last touched state for next iteration
  return pressedKey;
}

void resetPinEntry() {
  enteredPin = "";
}

void printUID(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " "); // Add leading zero if needed
    Serial.print(buffer[i], HEX);
  }
}