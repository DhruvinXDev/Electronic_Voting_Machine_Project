// ESP32 Four Push Button Voting System with Buzzer, RFID, and Web Server
// Modified to allow one vote per RFID scan and display results on a web page

#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>

// Access Point configuration
const char* ssid = "ESP32_Voting_System";  // Name of the WiFi network
const char* password = "vote1234";         // Password for the WiFi network

// Web server on port 80
WebServer server(80);

// Define the GPIO pins for buttons, LEDs and buzzer
// Changed to avoid conflicts with RFID pins (18, 19, 21, 22, 23)
const int button1Pin = 4;    // GPIO pin for first button
const int button2Pin = 17;   // GPIO pin for second button
const int button3Pin = 13;   // GPIO pin for third button
const int button4Pin = 14;   // GPIO pin for fourth button

const int led1Pin = 2;       // GPIO pin for first LED (built-in on most ESP32 boards)
const int led2Pin = 15;      // GPIO pin for second LED
const int led3Pin = 5;       // Changed from 18 to 5 (to avoid conflict with SCK_PIN)
const int led4Pin = 16;      // Changed from 5 to 16
const int led5Pin = 27;      // Changed from 21 to 27 (to avoid conflict with SS_PIN)

const int buzzerPin = 25;    // Changed from 16 to 25 (to avoid conflict with led4Pin)

// RFID pins - KEEP THESE EXACTLY AS SPECIFIED
#define SS_PIN    21   // ESP32 pin GPIO21 for SDA (SS)
#define RST_PIN   22   // ESP32 pin GPIO22 for RST
#define SCK_PIN   18   // ESP32 pin GPIO18 for SCK
#define MOSI_PIN  23   // ESP32 pin GPIO23 for MOSI
#define MISO_PIN  19   // ESP32 pin GPIO19 for MISO

// Create MFRC522 instance
MFRC522 rfid(SS_PIN, RST_PIN);

// RFID card UID to check against (updated with the provided UID)
byte authorizedUID[4] = {0x91, 0x6C, 0xDA, 0x0B};

// RFID status
bool rfidAuthorized = false;
bool voteRegistered = false;  // New flag to track if a vote has been registered after authentication
unsigned long lastRFIDCheck = 0;
unsigned long lastDebugPrint = 0;

// Variables to handle button debouncing
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long lastDebounceTime3 = 0;
unsigned long lastDebounceTime4 = 0;
unsigned long debounceDelay = 50;  // Debounce time in milliseconds

// Button press counters - one for each button
int button1Count = 0;
int button2Count = 0;
int button3Count = 0;
int button4Count = 0;
int lastActiveButton = 0;  // 0 = none, 1-4 = button number

// Variable to track which LED is currently active (0 = none, 1-4 = LED number)
int activeLED = 0;

// Flag to track if we're in a counting session
bool countingSession = false;

// Names for the voting options
String optionNames[4] = {
  "Option 1",
  "Option 2",
  "Option 3",
  "Option 4"
};

void setup() {
  Serial.begin(115200);      // Initialize serial communication
  
  // Wait a bit for serial to be ready
  delay(1000);
  
  Serial.println("\n\n--- Starting ESP32 RFID Voting System with Web Server ---");
  
  // Set button pins as inputs with pull-up resistors
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  pinMode(button4Pin, INPUT_PULLUP);
  
  // Set LED pins as outputs
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(led4Pin, OUTPUT);
  pinMode(led5Pin, OUTPUT);
  
  // Set buzzer pin as output
  pinMode(buzzerPin, OUTPUT);
  
  // Ensure all LEDs start off
  turnOffAllLEDs();
  
  // Blink LED 5 to indicate program start
  for (int i = 0; i < 3; i++) {
    digitalWrite(led5Pin, HIGH);
    delay(100);
    digitalWrite(led5Pin, LOW);
    delay(100);
  }
  
  Serial.println("Initializing SPI bus for RFID...");
  
  // IMPORTANT: Set SS pin as OUTPUT before initializing SPI
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  
  // Initialize custom SPI for RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  delay(250); // Give SPI more time to initialize
  
  Serial.println("Initializing RFID reader...");
  // Initialize RFID
  rfid.PCD_Init();
  delay(250);
  
  // Reset the RFID reader
  rfid.PCD_Reset();
  delay(250);
  rfid.PCD_Init();
  
  // Print MFRC522 version and status to verify it's working
  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("MFRC522 Software Version: 0x");
  Serial.print(v, HEX);
  if (v == 0x91 || v == 0x92) {
    Serial.println(" = v1.0 or v2.0");
  } else if (v == 0x12) {
    Serial.println(" = counterfeit chip");
  } else {
    Serial.println(" (unknown or not detected properly)");
    Serial.println("WARNING: RFID reader may not be connected correctly!");
  }
  
  // Check SPI communication with RFID reader
  Serial.println("Testing RFID Reader Communication...");
  
  // Set antenna gain to max and verify
  Serial.println("Setting antenna gain to maximum...");
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  
  // Setup Access Point
  Serial.println("Setting up WiFi Access Point...");
  WiFi.softAP(ssid, password);
  
  Serial.print("Access Point IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Setup server routes
  server.on("/", handleRoot);
  server.on("/results", handleResults);
  server.on("/refresh", handleRefresh);
  server.on("/reset", handleReset);
  
  // Start the server
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.println("\nRFID Reader Initialization Complete");
  Serial.println("Please scan your card to vote");
  Serial.println("Authorized UID: 0x91 0x6C 0xDA 0x0B");
  Serial.println("Connect to WiFi SSID: " + String(ssid) + " with password: " + String(password));
  Serial.println("Then open a web browser and navigate to http://" + WiFi.softAPIP().toString());
  
  displayAllCounts();
}

void loop() {
  // Handle web server clients
  server.handleClient();
  
  // Check RFID more frequently to ensure we don't miss cards
  if (millis() - lastRFIDCheck >= 50) {  // Check every 50ms
    checkRFID();
    lastRFIDCheck = millis();
  }
  
  // Print debug message every 3 seconds
  if (millis() - lastDebugPrint >= 3000) {
    if (rfidAuthorized && !voteRegistered) {
      Serial.println("Please select your vote option");
    } else if (rfidAuthorized && voteRegistered) {
      Serial.println("Vote registered. Please scan again to vote again.");
    } else {
      Serial.println("Waiting for RFID card...");
    }
    lastDebugPrint = millis();
  }
  
  // Blink LED5 to show system is running when not authorized
  if (!rfidAuthorized) {
    if ((millis() / 500) % 2 == 0) {
      digitalWrite(led5Pin, HIGH);
    } else {
      digitalWrite(led5Pin, LOW);
    }
  }
  
  // Process button presses only if RFID is authorized and vote not yet registered
  if (rfidAuthorized && !voteRegistered) {
    // Keep LED5 solid on when authorized
    digitalWrite(led5Pin, HIGH);
    
    // Read current state of all buttons
    int reading1 = digitalRead(button1Pin);
    int reading2 = digitalRead(button2Pin);
    int reading3 = digitalRead(button3Pin);
    int reading4 = digitalRead(button4Pin);
    
    // Check if any button is pressed to control buzzer
    bool anyButtonPressed = false;
    
    // Handle Button 1
    if (reading1 == LOW) {  // Button pressed (LOW because of pull-up resistor)
      if ((millis() - lastDebounceTime1) > debounceDelay) {
        // Button 1 is pressed and debounced
        if (activeLED != 1) {  // Only update if this is a new press
          activeLED = 1;
          lastActiveButton = 1;
          updateLEDs();
          countingSession = true;  // Begin a counting session
        }
        anyButtonPressed = true;
      }
      lastDebounceTime1 = millis();
    }
    
    // Handle Button 2
    if (reading2 == LOW) {
      if ((millis() - lastDebounceTime2) > debounceDelay) {
        if (activeLED != 2) {
          activeLED = 2;
          lastActiveButton = 2;
          updateLEDs();
          countingSession = true;
        }
        anyButtonPressed = true;
      }
      lastDebounceTime2 = millis();
    }
    
    // Handle Button 3
    if (reading3 == LOW) {
      if ((millis() - lastDebounceTime3) > debounceDelay) {
        if (activeLED != 3) {
          activeLED = 3;
          lastActiveButton = 3;
          updateLEDs();
          countingSession = true;
        }
        anyButtonPressed = true;
      }
      lastDebounceTime3 = millis();
    }
    
    // Handle Button 4
    if (reading4 == LOW) {
      if ((millis() - lastDebounceTime4) > debounceDelay) {
        if (activeLED != 4) {
          activeLED = 4;
          lastActiveButton = 4;
          updateLEDs();
          countingSession = true;
        }
        anyButtonPressed = true;
      }
      lastDebounceTime4 = millis();
    }
    
    // Check if all buttons are released
    if (reading1 == HIGH && reading2 == HIGH && reading3 == HIGH && reading4 == HIGH) {
      // All buttons released
      if (activeLED != 0) {
        // Only count if we were in a counting session
        if (countingSession) {
          // Increment the counter for the last active button
          switch (lastActiveButton) {
            case 1:
              button1Count++;
              break;
            case 2:
              button2Count++;
              break;
            case 3:
              button3Count++;
              break;
            case 4:
              button4Count++;
              break;
          }
          
          // Display all button counts
          displayAllCounts();
          countingSession = false;  // End counting session until a new button press
          
          // Mark vote as registered and reset RFID authorization
          voteRegistered = true;
          
          // Signal successful vote with double beep
          signalSuccessfulVote();
          
          Serial.println("Vote registered successfully!");
          Serial.println("Please scan card again to vote again.");
        }
        
        // Turn off LED 1-4 only (keep LED 5 for RFID status)
        activeLED = 0;
        digitalWrite(led1Pin, LOW);
        digitalWrite(led2Pin, LOW);
        digitalWrite(led3Pin, LOW);
        digitalWrite(led4Pin, LOW);
      }
    }
    
    // Control buzzer based on any button press
    digitalWrite(buzzerPin, anyButtonPressed ? HIGH : LOW);
  } else if (rfidAuthorized && voteRegistered) {
    // Keep LED5 blinking in a different pattern when vote is registered
    // but still authenticated (waiting for RFID reset)
    if ((millis() / 200) % 2 == 0) {
      digitalWrite(led5Pin, HIGH);
    } else {
      digitalWrite(led5Pin, LOW);
    }
    
    // Wait for card to be removed
    resetRFIDIfCardRemoved();
  } else {
    // RFID not authorized, keep all LEDs off except LED 5 which will blink
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    digitalWrite(led4Pin, LOW);
    // LED5 controlled by the blinking code above
  }
  
  delay(10);  // Small delay to stabilize readings
}

// Handle root page - redirect to results
void handleRoot() {
  server.sendHeader("Location", "/results", true);
  server.send(302, "text/plain", "");
}

// Handle results page request
void handleResults() {
  String html = "<!DOCTYPE html>\n";
  html += "<html lang='en'>\n";
  html += "<head>\n";
  html += "  <meta charset='UTF-8'>\n";
  html += "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  html += "  <title>ESP32 Voting Results</title>\n";
  html += "  <style>\n";
  html += "    body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }\n";
  html += "    h1 { color: #333; }\n";
  html += "    .container { max-width: 600px; margin: 0 auto; }\n";
  html += "    .result-box { border: 1px solid #ddd; padding: 10px; margin-bottom: 10px; border-radius: 5px; }\n";
  html += "    .result-name { font-weight: bold; }\n";
  html += "    .bar-container { background-color: #f1f1f1; height: 25px; border-radius: 3px; margin-top: 5px; }\n";
  html += "    .bar { height: 25px; background-color: #4CAF50; border-radius: 3px; text-align: right; color: white; padding-right: 10px; line-height: 25px; }\n";
  html += "    .status { margin: 20px 0; padding: 10px; border-radius: 5px; }\n";
  html += "    .authorized { background-color: #d4edda; color: #155724; }\n";
  html += "    .unauthorized { background-color: #f8d7da; color: #721c24; }\n";
  html += "    .registered { background-color: #fff3cd; color: #856404; }\n";
  html += "    .button { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 5px; }\n";
  html += "    .reset { background-color: #f44336; }\n";
  html += "  </style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "  <div class='container'>\n";
  html += "    <h1>ESP32 Voting System Results</h1>\n";
  
  // Display system status
  html += "    <div class='status ";
  if (rfidAuthorized && !voteRegistered) {
    html += "authorized'><b>Status:</b> Card authorized, waiting for vote";
  } else if (rfidAuthorized && voteRegistered) {
    html += "registered'><b>Status:</b> Vote registered, please scan again to vote again";
  } else {
    html += "unauthorized'><b>Status:</b> Waiting for authorized RFID card";
  }
  html += "</div>\n";
  
  // Calculate total votes for percentage
  int totalVotes = button1Count + button2Count + button3Count + button4Count;
  
  // Display voting results with bars
  for (int i = 0; i < 4; i++) {
    int count = 0;
    switch (i) {
      case 0: count = button1Count; break;
      case 1: count = button2Count; break;
      case 2: count = button3Count; break;
      case 3: count = button4Count; break;
    }
    
    int percentage = totalVotes > 0 ? (count * 100) / totalVotes : 0;
    
    html += "    <div class='result-box'>\n";
    html += "      <div class='result-name'>" + optionNames[i] + ": " + String(count) + " votes</div>\n";
    html += "      <div class='bar-container'>\n";
    html += "        <div class='bar' style='width: " + String(percentage) + "%;'>" + String(percentage) + "%</div>\n";
    html += "      </div>\n";
    html += "    </div>\n";
  }
  
  html += "    <p><b>Total Votes: " + String(totalVotes) + "</b></p>\n";
  html += "    <a href='/refresh' class='button'>Refresh</a>\n";
  html += "    <a href='/reset' class='button reset'>Reset All Votes</a>\n";
  html += "    <p><small>ESP32 IP Address: " + WiFi.softAPIP().toString() + "</small></p>\n";
  html += "  </div>\n";
  
  // Add auto-refresh every 5 seconds
  html += "  <script>\n";
  html += "    setTimeout(function() { window.location.reload(); }, 5000);\n";
  html += "  </script>\n";
  html += "</body>\n";
  html += "</html>\n";
  
  server.send(200, "text/html", html);
}

// Handle refresh request
void handleRefresh() {
  server.sendHeader("Location", "/results", true);
  server.send(302, "text/plain", "");
}

// Handle reset request
void handleReset() {
  // Reset all vote counters
  button1Count = 0;
  button2Count = 0;
  button3Count = 0;
  button4Count = 0;
  
  Serial.println("All votes have been reset from web interface!");
  displayAllCounts();
  
  // Redirect back to results page
  server.sendHeader("Location", "/results", true);
  server.send(302, "text/plain", "");
}

// Enhanced RFID checking function with more debugging
void checkRFID() {
  // Try to detect if card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    // No card present, just return
    return;
  }
  
  // Card detected, output debug information
  Serial.println("\n----- CARD DETECTED -----");
  
  // Try to read the card
  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("ERROR: Could detect card presence but failed to read card serial!");
    return;
  }
  
  // Card has been successfully read! Show all card details
  Serial.println("SUCCESS: Card read successfully!");
  
  // Print UID in different formats for debugging
  Serial.print("Card UID (HEX): ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  Serial.print("Card UID (DEC): ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], DEC);
    Serial.print(" ");
  }
  Serial.println();
  
  Serial.print("Card UID Size: ");
  Serial.print(rfid.uid.size);
  Serial.println(" bytes");
  
  // Print card type
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("Card type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));
  
  // Compare UID with authorized UID
  bool authorized = true;
  for (byte i = 0; i < 4; i++) {  // Only check first 4 bytes
    if (i < rfid.uid.size && rfid.uid.uidByte[i] != authorizedUID[i]) {
      authorized = false;
      break;
    }
  }
  
  Serial.print("Authorized: ");
  Serial.println(authorized ? "YES" : "NO");
  
  // If already authorized and vote registered, this is a new scan,
  // so we need to reset the vote registered flag
  if (rfidAuthorized && voteRegistered) {
    rfidAuthorized = false;  // Reset first, then re-authorize
    voteRegistered = false;
    Serial.println("Previous vote session ended.");
  }
  
  if (authorized && !rfidAuthorized) {
    Serial.println("*** ACCESS GRANTED! ***");
    Serial.println("You may now cast your vote.");
    rfidAuthorized = true;
    voteRegistered = false;  // Reset vote status for new card
    digitalWrite(led5Pin, HIGH);  // Turn on LED 5 to indicate authorized access
    
    // Short beep to indicate successful authorization
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
  } else if (!authorized) {
    Serial.println("*** ACCESS DENIED! ***");
    rfidAuthorized = false;
    voteRegistered = false;
    digitalWrite(led5Pin, LOW);  // Turn off LED 5
    soundBuzzer5Times();  // Sound buzzer 5 times for unauthorized access
  }
  
  // Halt PICC and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Function to check if the card has been removed to reset RFID state
void resetRFIDIfCardRemoved() {
  // If card is no longer present for a certain time, reset the authorization
  static unsigned long cardRemovedTime = 0;
  
  if (!rfid.PICC_IsNewCardPresent()) {
    if (cardRemovedTime == 0) {
      cardRemovedTime = millis();
    } else if (millis() - cardRemovedTime > 1000) { // Wait 1 second to confirm card is truly gone
      rfidAuthorized = false;
      voteRegistered = false;
      cardRemovedTime = 0;
      Serial.println("Card removed. RFID authorization reset.");
      // Blink LED5 several times to indicate reset
      for (int i = 0; i < 3; i++) {
        digitalWrite(led5Pin, HIGH);
        delay(50);
        digitalWrite(led5Pin, LOW);
        delay(50);
      }
    }
  } else {
    cardRemovedTime = 0; // Reset timer if card is detected
  }
}

// Function to signal successful vote with double beep
void signalSuccessfulVote() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// Function to sound buzzer 5 times
void soundBuzzer5Times() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(200);
    digitalWrite(buzzerPin, LOW);
    delay(200);
  }
}

// Function to display all button counts
void displayAllCounts() {
  Serial.println("-------------------");
  Serial.println("Voting Results:");
  Serial.print("Option 1: ");
  Serial.println(button1Count);
  Serial.print("Option 2: ");
  Serial.println(button2Count);
  Serial.print("Option 3: ");
  Serial.println(button3Count);
  Serial.print("Option 4: ");
  Serial.println(button4Count);
  Serial.println("-------------------");
}

// Function to update LEDs based on active LED (preserving LED 5 status)
void updateLEDs() {
  // Turn off LEDs 1-4 only
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);
  digitalWrite(led4Pin, LOW);
  
  // Turn on only the active LED
  switch (activeLED) {
    case 1:
      digitalWrite(led1Pin, HIGH);
      break;
    case 2:
      digitalWrite(led2Pin, HIGH);
      break;
    case 3:
      digitalWrite(led3Pin, HIGH);
      break;
    case 4:
      digitalWrite(led4Pin, HIGH);
      break;
    default:
      // No LED active
      break;
  }
}

// Function to turn off all LEDs including LED 5
void turnOffAllLEDs() {
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);
  digitalWrite(led4Pin, LOW);
  digitalWrite(led5Pin, LOW);
}
