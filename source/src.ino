#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>

// Pin definitions
#define SS_PIN 21       // ESP32 pin for SPI SS to RC522
#define RST_PIN 22      // ESP32 pin for SPI RST to RC522

// LED pins
#define AUTH_SUCCESS_LED 12    // Green LED for RFID authentication success
#define AUTH_FAIL_LED 14       // Red LED for RFID authentication failure
#define PARTY1_LED 27          // Green LED for Party 1 vote
#define PARTY2_LED 26          // Green LED for Party 2 vote
#define PARTY3_LED 25          // Green LED for Party 3 vote
#define PARTY4_LED 33          // Green LED for Party 4 vote

// Buzzer pins
#define AUTH_BUZZER 32         // Buzzer for authentication feedback
#define VOTE_BUZZER 15         // Buzzer for vote confirmation

// Button pins
#define PARTY1_BTN 4           // Button for Party 1
#define PARTY2_BTN 5           // Button for Party 2
#define PARTY3_BTN 13          // Button for Party 3
#define PARTY4_BTN 2          // Button for Party 4

// Access Point Settings
const char* ap_ssid = "VotingMachine_AP";
const char* ap_password = "votingmachine123";

// RFID setup
MFRC522 rfid(SS_PIN, RST_PIN);

// Create webserver object
WebServer server(80);

// Vote counters (initially zero)
int count_party_1 = 0;
int count_party_2 = 0;
int count_party_3 = 0;
int count_party_4 = 0;

// RFID UIDs (5 different cards) - stored as byte arrays
byte RFID_CARD_UID[5][4] = {
  {0x91, 0x6C, 0xDA, 0x0B},  // Card 1
  {0xA2, 0x7D, 0xEB, 0x1C},  // Card 2
  {0xB3, 0x8E, 0xFC, 0x2D},  // Card 3
  {0xC4, 0x9F, 0x0D, 0x3E},  // Card 4
  {0xD5, 0xA0, 0x1E, 0x4F}   // Card 5
};

// Authentication status (0 = false, 1 = true)
int authentication_status = 0;

// RFID UID status (0 = not voted, 1 = voted)
int RFID_UID_STATUS[5] = {0, 0, 0, 0, 0};

// Current authenticated card index
int current_card_index = -1;

void setup() {
  Serial.begin(115200);
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Initialize pins
  pinMode(AUTH_SUCCESS_LED, OUTPUT);
  pinMode(AUTH_FAIL_LED, OUTPUT);
  pinMode(PARTY1_LED, OUTPUT);
  pinMode(PARTY2_LED, OUTPUT);
  pinMode(PARTY3_LED, OUTPUT);
  pinMode(PARTY4_LED, OUTPUT);
  
  pinMode(AUTH_BUZZER, OUTPUT);
  pinMode(VOTE_BUZZER, OUTPUT);
  
  pinMode(PARTY1_BTN, INPUT_PULLUP);
  pinMode(PARTY2_BTN, INPUT_PULLUP);
  pinMode(PARTY3_BTN, INPUT_PULLUP);
  pinMode(PARTY4_BTN, INPUT_PULLUP);
  
  // Set initial states
  digitalWrite(AUTH_SUCCESS_LED, LOW);
  digitalWrite(AUTH_FAIL_LED, LOW);
  digitalWrite(PARTY1_LED, LOW);
  digitalWrite(PARTY2_LED, LOW);
  digitalWrite(PARTY3_LED, LOW);
  digitalWrite(PARTY4_LED, LOW);
  digitalWrite(AUTH_BUZZER, LOW);
  digitalWrite(VOTE_BUZZER, LOW);
  
  // Setup WiFi Access Point
  Serial.println("Setting up Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("Access Point Created");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  
  // Initial startup indication
  for (int i = 0; i < 3; i++) {
    digitalWrite(AUTH_SUCCESS_LED, HIGH);
    delay(200);
    digitalWrite(AUTH_SUCCESS_LED, LOW);
    delay(200);
  }
  
  Serial.println("Voting Machine Ready. Scan RFID card to begin.");
}

void loop() {
  // Handle web server clients
  server.handleClient();
  
  // Check if there's a new card present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Print card UID for debugging
    Serial.print("Card detected: ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    
    // Check if card is authorized
    int cardIndex = checkCardAuthorization(rfid.uid.uidByte, rfid.uid.size);
    
    if (cardIndex != -1 && RFID_UID_STATUS[cardIndex] == 0) {
      // Authentication successful
      authentication_status = 1;
      current_card_index = cardIndex;
      Serial.println("Authentication successful. Please vote.");
      
      // Turn on success LED
      digitalWrite(AUTH_SUCCESS_LED, HIGH);
      
      // Success beep - 1 second
      digitalWrite(AUTH_BUZZER, HIGH);
      delay(1000);
      digitalWrite(AUTH_BUZZER, LOW);
      digitalWrite(AUTH_SUCCESS_LED, LOW);
      
    } else {
      // Authentication failed or already voted
      authentication_status = 0;
      current_card_index = -1;
      
      if (cardIndex == -1) {
        Serial.println("Authentication failed. Invalid card.");
      } else {
        Serial.println("Authentication failed. Card already used for voting.");
      }
      
      // Turn on fail LED
      digitalWrite(AUTH_FAIL_LED, HIGH);
      
      // Failure beep - 5 seconds
      digitalWrite(AUTH_BUZZER, HIGH);
      delay(5000);
      digitalWrite(AUTH_BUZZER, LOW);
      digitalWrite(AUTH_FAIL_LED, LOW);
    }
    
    // Halt PICC
    rfid.PICC_HaltA();
    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();
  }
  
  // Check for votes if authentication is successful
  if (authentication_status == 1) {
    // Check party buttons
    if (digitalRead(PARTY1_BTN) == LOW) {
      processVote(0, PARTY1_LED, &count_party_1);
    }
    else if (digitalRead(PARTY2_BTN) == LOW) {
      processVote(1, PARTY2_LED, &count_party_2);
    }
    else if (digitalRead(PARTY3_BTN) == LOW) {
      processVote(2, PARTY3_LED, &count_party_3);
    }
    else if (digitalRead(PARTY4_BTN) == LOW) {
      processVote(3, PARTY4_LED, &count_party_4);
    }
  }
}

// Function to compare UID bytes and check authorization
int checkCardAuthorization(byte* cardUID, byte uidSize) {
  for (int i = 0; i < 5; i++) {
    bool match = true;
    
    // Compare each byte of the UID
    for (byte j = 0; j < 4; j++) {
      if (j < uidSize && cardUID[j] != RFID_CARD_UID[i][j]) {
        match = false;
        break;
      }
    }
    
    if (match) {
      return i; // Return the card index
    }
  }
  
  return -1; // Not found
}

// Function to print UID in hex format
void printHex(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}

void processVote(int partyIndex, int ledPin, int* counterVar) {
  if (current_card_index != -1 && authentication_status == 1) {
    // Increment vote counter
    (*counterVar)++;
    
    // Turn on party LED
    digitalWrite(ledPin, HIGH);
    
    // Vote confirmation beep - 1 second
    digitalWrite(VOTE_BUZZER, HIGH);
    delay(1000);
    digitalWrite(VOTE_BUZZER, LOW);
    digitalWrite(ledPin, LOW);
    
    // Mark card as used
    RFID_UID_STATUS[current_card_index] = 1;
    
    // Reset authentication status
    authentication_status = 0;
    current_card_index = -1;
    
    Serial.print("Vote recorded for Party ");
    Serial.println(partyIndex + 1);
    
    // Print current vote count
    printVoteCounts();
  }
}

void printVoteCounts() {
  Serial.println("Current Vote Count:");
  Serial.print("Party 1: ");
  Serial.println(count_party_1);
  Serial.print("Party 2: ");
  Serial.println(count_party_2);
  Serial.print("Party 3: ");
  Serial.println(count_party_3);
  Serial.print("Party 4: ");
  Serial.println(count_party_4);
}

void handleRoot() {
  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "<title>Voting Machine Results</title>\n";
  html += "<style>\n";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 20px; }\n";
  html += ".party-box { border: 1px solid #ddd; margin: 10px; padding: 15px; border-radius: 5px; }\n";
  html += ".party1 { background-color: #c8e6c9; }\n";
  html += ".party2 { background-color: #bbdefb; }\n";
  html += ".party3 { background-color: #ffecb3; }\n";
  html += ".party4 { background-color: #f8bbd0; }\n";
  html += ".votes { font-size: 24px; font-weight: bold; }\n";
  html += "h1 { color: #333; }\n";
  html += ".refresh-btn { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; }\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<h1>Electronic Voting Machine Results</h1>\n";
  
  html += "<div class=\"party-box party1\">\n";
  html += "<h2>Party 1</h2>\n";
  html += "<div class=\"votes\">" + String(count_party_1) + "</div>\n";
  html += "</div>\n";
  
  html += "<div class=\"party-box party2\">\n";
  html += "<h2>Party 2</h2>\n";
  html += "<div class=\"votes\">" + String(count_party_2) + "</div>\n";
  html += "</div>\n";
  
  html += "<div class=\"party-box party3\">\n";
  html += "<h2>Party 3</h2>\n";
  html += "<div class=\"votes\">" + String(count_party_3) + "</div>\n";
  html += "</div>\n";
  
  html += "<div class=\"party-box party4\">\n";
  html += "<h2>Party 4</h2>\n";
  html += "<div class=\"votes\">" + String(count_party_4) + "</div>\n";
  html += "</div>\n";
  
  html += "<p>Total Votes: " + String(count_party_1 + count_party_2 + count_party_3 + count_party_4) + "</p>\n";
  html += "<button class=\"refresh-btn\" onclick=\"location.reload()\">Refresh Results</button>\n";
  html += "<script>\n";
  html += "setTimeout(function(){ location.reload(); }, 5000);\n";
  html += "</script>\n";
  html += "</body>\n";
  html += "</html>\n";
  
  server.send(200, "text/html", html);
}
