# ESP32 Connection Diagram for RFID Button System

## Component Overview
This connection diagram outlines how to wire the ESP32 with:
- MFRC522 RFID Reader
- 4 Push Buttons
- 5 LEDs
- 1 Buzzer

## Pin Assignments

### RFID Reader (MFRC522)
| MFRC522 Pin | ESP32 GPIO | Notes |
|-------------|------------|-------|
| SDA (SS)    | GPIO 21    | Chip select for SPI |
| SCK         | GPIO 18    | SPI clock |
| MOSI        | GPIO 23    | SPI master out, slave in |
| MISO        | GPIO 19    | SPI master in, slave out |
| RST         | GPIO 22    | Reset |
| 3.3V        | 3.3V       | Power supply |
| GND         | GND        | Ground |

### Push Buttons
| Button | ESP32 GPIO | Notes |
|--------|------------|-------|
| Button 1 | GPIO 4   | Connected with pull-up resistor |
| Button 2 | GPIO 17  | Connected with pull-up resistor |
| Button 3 | GPIO 13  | Connected with pull-up resistor |
| Button 4 | GPIO 14  | Connected with pull-up resistor |

### LEDs
| LED | ESP32 GPIO | Purpose |
|-----|------------|---------|
| LED 1 | GPIO 2   | Indicator for Button 1 (built-in LED on most ESP32 boards) |
| LED 2 | GPIO 15  | Indicator for Button 2 |
| LED 3 | GPIO 5   | Indicator for Button 3 |
| LED 4 | GPIO 16  | Indicator for Button 4 |
| LED 5 | GPIO 27  | RFID status indicator |

### Buzzer
| Component | ESP32 GPIO | Purpose |
|-----------|------------|---------|
| Buzzer    | GPIO 25    | Audio feedback |

## Wiring Instructions

### For each button:
1. Connect one terminal to the corresponding GPIO pin
2. Connect the other terminal to GND
3. No external pull-up resistors needed (using internal pull-ups)

### For each LED:
1. Connect the anode (longer leg) to the corresponding GPIO pin
2. Connect the cathode (shorter leg) to GND through a 220-330Ω resistor

### For the buzzer:
1. Connect the positive terminal to GPIO 25
2. Connect the negative terminal to GND

### For the RFID reader:
1. Connect the SDA pin to GPIO 21
2. Connect the SCK pin to GPIO 18
3. Connect the MOSI pin to GPIO 23
4. Connect the MISO pin to GPIO 19
5. Connect the RST pin to GPIO 22
6. Connect the 3.3V pin to the 3.3V output on the ESP32
7. Connect the GND pin to GND

## Notes
- All buttons are configured with internal pull-up resistors (INPUT_PULLUP)
- The system requires RFID authentication before buttons become active
- LED 5 indicates RFID status (blinking when waiting, solid when authorized)
- The authorized RFID card UID is: 0x91, 0x6C, 0xDA, 0x0B
