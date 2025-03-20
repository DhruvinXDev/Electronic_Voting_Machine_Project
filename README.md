

# **ESP32 Electronic Voting Machine**

This project implements an electronic voting machine using an **ESP32** microcontroller. The system includes RFID-based voter authentication and a web interface to display real-time vote counts.

## **Hardware Components**
The system uses the following components:

- **ESP32 DOIT DEV V1 Kit**: Main controller board
- **6 LEDs**:
  - 4 Green LEDs (for vote confirmation of each party)
  - 1 Red LED (for RFID authentication failure)
  - 1 Green LED (for RFID authentication success)
- **2 Buzzers**:
  - Authentication Buzzer: 1 second for success, 5 seconds for failure
  - Vote Buzzer: 1-second confirmation when a vote is registered
- **4 Push Buttons**: One for each political party
- **RFID Reader**: For voter authentication
- **5 RFID Cards**: For unique voter identification

## **Circuit Connections**
### **RFID Reader (RC522) to ESP32**
| **RFID Pin**    | **ESP32 Pin**  |
|-----------------|----------------|
| SDA (SS)        | GPIO21         |
| SCK             | GPIO18         |
| MOSI            | GPIO23         |
| MISO            | GPIO19         |
| RST             | GPIO22         |
| GND             | GND            |
| 3.3V            | 3.3V           |

### **LEDs**
| **LED**                         | **ESP32 Pin** |
|----------------------------------|---------------|
| Authentication Success LED (Green) | GPIO12        |
| Authentication Failure LED (Red)  | GPIO14        |
| Party 1 LED (Green)             | GPIO27        |
| Party 2 LED (Green)             | GPIO26        |
| Party 3 LED (Green)             | GPIO25        |
| Party 4 LED (Green)             | GPIO33        |

### **Buzzers**
| **Buzzer**                 | **ESP32 Pin** |
|----------------------------|---------------|
| Authentication Buzzer      | GPIO32        |
| Vote Buzzer                | GPIO15        |

### **Buttons**
| **Button**                 | **ESP32 Pin** |
|----------------------------|---------------|
| Party 1 Button             | GPIO4         |
| Party 2 Button             | GPIO5         |
| Party 3 Button             | GPIO13        |
| Party 4 Button             | GPIO2         |

## **Code Overview**
The code implements the following functionality:

### **Vote Counter Variables:**
- `count_party_1`, `count_party_2`, `count_party_3`, `count_party_4`: Initialized to 0 for tracking votes for each party.

### **Security Variables:**
- `RFID_CARD_UID[5]`: Array of 5 different UIDs for authorized voters.
- `authentication_status`: 0 for false, 1 for true.
- `RFID_UID_STATUS[5]`: Array tracking which cards have voted (0 = not voted, 1 = voted).

### **Authentication Process:**
1. Scans the RFID card and checks if the UID matches any in the authorized list.
2. If the UID matches and hasn't voted (`RFID_UID_STATUS[i] == 0`), `authentication_status` is set to 1 (true).
   - Green LED blinks with a 1-second buzzer tone.
3. If the UID doesn't match or has already voted, a Red LED blinks with a 5-second buzzer tone.

### **Voting Process:**
- Voting is only allowed when `authentication_status == 1`.
- When a party button is pressed:
  - The respective vote counter is incremented.
  - The corresponding party's Green LED blinks.
  - A 1-second buzzer tone confirms the vote.
  - Updates `RFID_UID_STATUS[i]` to 1 (indicating the voter has voted).
  - Resets `authentication_status` to 0 (false).

### **Web Interface:**
- Displays real-time vote count for all parties.
- Automatically refreshes every 5 seconds.
- Uses different colors to represent votes for each party.
- Shows the total vote count.

## **To Use the System:**
1. Power on the ESP32.
2. Connect to the ESP32's WiFi network or ensure it's on your local network.
3. Access the web interface via the ESP32's IP address.
4. **To vote:**
   - Scan an authorized RFID card.
   - If authentication is successful, press one of the four party buttons.
   - The system will confirm with LED and buzzer feedback.
   - The vote count will be updated on the web interface.

### **Important:**
- Ensure to update the WiFi credentials in the code before uploading it to your ESP32.

