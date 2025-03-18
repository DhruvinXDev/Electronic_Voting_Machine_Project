Project Description: ESP32 Electronic Voting Machine
This project implements an electronic voting machine with the following components:

Hardware Components
ESP32 DOIT DEV V1 Kit: Main controller board
6 LEDs:
4 Green LEDs (for vote confirmation of each party)
1 Red LED (for RFID authentication failure)
1 Green LED (for RFID authentication success)
2 Buzzers:
Authentication Buzzer: 1 second for success, 5 seconds for failure
Vote Buzzer: 1 second confirmation when vote is registered
4 Push Buttons: One for each political party
RFID Reader: For voter authentication
5 RFID Cards: For unique voter identification
Circuit Connections
RFID Reader (RC522) to ESP32:
SDA (SS) → GPIO21
SCK → SCK (GPIO18)
MOSI → MOSI (GPIO23)
MISO → MISO (GPIO19)
RST → GPIO22
GND → GND
3.3V → 3.3V
LEDs:
Authentication Success LED (Green) → GPIO12
Authentication Failure LED (Red) → GPIO14
Party 1 LED (Green) → GPIO27
Party 2 LED (Green) → GPIO26
Party 3 LED (Green) → GPIO25
Party 4 LED (Green) → GPIO33
Buzzers:
Authentication Buzzer → GPIO32
Vote Buzzer → GPIO15
Buttons:
Party 1 Button → GPIO4
Party 2 Button → GPIO5
Party 3 Button → GPIO18
Party 4 Button → GPIO19
Code Overview
The code implements the following functionality:

Vote Counter Variables:
count_party_1, count_party_2, count_party_3, count_party_4 (all initialized to zero)
Security Variables:
RFID_CARD_UID[5]: Array of 5 different UIDs for authorized voters
authentication_status: 0 for false, 1 for true
RFID_UID_STATUS[5]: Array tracking which cards have voted (0 = not voted, 1 = voted)
Authentication Process:
Scans RFID card and checks if UID matches any in the authorized list
If UID matches and hasn't voted (RFID_UID_STATUS[i] == 0):
authentication_status set to 1 (true)
Green LED blinks with 1-second buzzer tone
If UID doesn't match or has already voted:
Red LED blinks with 5-second buzzer tone
Voting Process:
Only accepts votes when authentication_status == 1
When a party button is pressed:
Increments respective counter
Green LED for that party blinks
1-second buzzer tone for confirmation
Updates RFID_UID_STATUS[i] to 1 (voted)
Resets authentication_status to 0 (false)
Web Interface:
Shows real-time vote count for all parties
Automatically refreshes every 5 seconds
Visual display with different colors for each party
Shows total vote count
To Use the System:
Power on the ESP32
Connect to the ESP32's WiFi network or ensure it's on your local network
Access the web interface via the ESP32's IP address
To vote:
Scan an authorized RFID card
If authentication succeeds, press one of the four party buttons
System will confirm with LED and buzzer feedback
Vote count will be updated on the web interface
Remember to update the WiFi credentials in the code before uploading it to your ESP32.




