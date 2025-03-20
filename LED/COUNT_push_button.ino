// ESP32 Four Push Button Example with Buzzer
// Button presses are only counted if the last active LED was from that button

// Define the GPIO pins for buttons, LEDs and buzzer
const int button1Pin = 4;    // GPIO pin for first button
const int button2Pin = 5;    // GPIO pin for second button
const int button3Pin = 13;   // GPIO pin for third button
const int button4Pin = 14;   // GPIO pin for fourth button

const int led1Pin = 2;       // GPIO pin for first LED (built-in on most ESP32 boards)
const int led2Pin = 15;      // GPIO pin for second LED
const int led3Pin = 18;      // GPIO pin for third LED
const int led4Pin = 19;      // GPIO pin for fourth LED

const int buzzerPin = 16;    // GPIO pin for buzzer

// Variables to handle button debouncing
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long lastDebounceTime3 = 0;
unsigned long lastDebounceTime4 = 0;
unsigned long debounceDelay = 50;  // Debounce time in milliseconds

// Button press counters
int button1Count = 0;
int button2Count = 0;
int button3Count = 0;
int button4Count = 0;

// Variable to track which LED is currently active (0 = none, 1-4 = LED number)
int activeLED = 0;

// Variable to keep track of the last active LED
int lastActiveLED = 0;

// Flag to track if a button was just pressed
bool buttonJustPressed = false;

void setup() {
  Serial.begin(115200);      // Initialize serial communication
  
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
  
  // Set buzzer pin as output
  pinMode(buzzerPin, OUTPUT);
  
  // Ensure all LEDs start off
  turnOffAllLEDs();
  
  Serial.println("ESP32 Four Button Example with Last LED Button Count Feature");
}

void loop() {
  // Read current state of all buttons
  int reading1 = digitalRead(button1Pin);
  int reading2 = digitalRead(button2Pin);
  int reading3 = digitalRead(button3Pin);
  int reading4 = digitalRead(button4Pin);
  
  // Check if any button is pressed to control buzzer
  bool anyButtonPressed = false;
  buttonJustPressed = false;
  
  // Handle Button 1
  if (reading1 == LOW) {  // Button pressed (LOW because of pull-up resistor)
    if ((millis() - lastDebounceTime1) > debounceDelay) {
      // Button 1 is pressed and debounced
      if (activeLED != 1) {  // Only if this is a new press
        // Only increment counter if the last active LED was LED 1
        if (lastActiveLED == 1) {
          button1Count++;
          Serial.println("Button 1 counted! Count: " + String(button1Count));
        }
        buttonJustPressed = true;
        Serial.println("Button 1 pressed!");
      }
      activeLED = 1;
      updateLEDs();
      anyButtonPressed = true;
    }
    lastDebounceTime1 = millis();
  }
  
  // Handle Button 2
  if (reading2 == LOW) {
    if ((millis() - lastDebounceTime2) > debounceDelay) {
      if (activeLED != 2) {  // Only if this is a new press
        // Only increment counter if the last active LED was LED 2
        if (lastActiveLED == 2) {
          button2Count++;
          Serial.println("Button 2 counted! Count: " + String(button2Count));
        }
        buttonJustPressed = true;
        Serial.println("Button 2 pressed!");
      }
      activeLED = 2;
      updateLEDs();
      anyButtonPressed = true;
    }
    lastDebounceTime2 = millis();
  }
  
  // Handle Button 3
  if (reading3 == LOW) {
    if ((millis() - lastDebounceTime3) > debounceDelay) {
      if (activeLED != 3) {  // Only if this is a new press
        // Only increment counter if the last active LED was LED 3
        if (lastActiveLED == 3) {
          button3Count++;
          Serial.println("Button 3 counted! Count: " + String(button3Count));
        }
        buttonJustPressed = true;
        Serial.println("Button 3 pressed!");
      }
      activeLED = 3;
      updateLEDs();
      anyButtonPressed = true;
    }
    lastDebounceTime3 = millis();
  }
  
  // Handle Button 4
  if (reading4 == LOW) {
    if ((millis() - lastDebounceTime4) > debounceDelay) {
      if (activeLED != 4) {  // Only if this is a new press
        // Only increment counter if the last active LED was LED 4
        if (lastActiveLED == 4) {
          button4Count++;
          Serial.println("Button 4 counted! Count: " + String(button4Count));
        }
        buttonJustPressed = true;
        Serial.println("Button 4 pressed!");
      }
      activeLED = 4;
      updateLEDs();
      anyButtonPressed = true;
    }
    lastDebounceTime4 = millis();
  }
  
  // Check if all buttons are released
  if (reading1 == HIGH && reading2 == HIGH && reading3 == HIGH && reading4 == HIGH) {
    // All buttons released, record the last active LED before turning off
    if (activeLED != 0) {
      lastActiveLED = activeLED;
      
      // Print the last active LED and button press counts
      Serial.print("Last active LED was: ");
      Serial.println(lastActiveLED);
      Serial.println("Button press counts:");
      Serial.print("Button 1: ");
      Serial.println(button1Count);
      Serial.print("Button 2: ");
      Serial.println(button2Count);
      Serial.print("Button 3: ");
      Serial.println(button3Count);
      Serial.print("Button 4: ");
      Serial.println(button4Count);
      Serial.println("-------------------");
      
      // Turn off all LEDs
      activeLED = 0;
      turnOffAllLEDs();
    }
  }
  
  // Control buzzer based on any button press
  digitalWrite(buzzerPin, anyButtonPressed ? HIGH : LOW);
  
  delay(10);  // Small delay to stabilize readings
}

// Function to update LEDs based on active LED
void updateLEDs() {
  turnOffAllLEDs();
  
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

// Function to turn off all LEDs
void turnOffAllLEDs() {
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);
  digitalWrite(led4Pin, LOW);
}
