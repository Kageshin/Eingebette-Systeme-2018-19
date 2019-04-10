// Bluetooth Libary's
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enabled it
#endif
BluetoothSerial SerialBT;

// Display Libary's
#include <SPI.h> // Serial Peripheral Interface
#include <Wire.h> // IC2 Kommunikatoin (Display)
#include "SSD1306.h"
#define OLED_RESET 4
SSD1306 display(0x3c, 21, 22);

// EEPROM config
#include <EEPROM.h>
#define EEPROM_SIZE 1

// Other variables
int output_value = 0;
int targetValue = 50;
bool enabled;
unsigned long delayTimer = 0;

// Determin the Pins
int sensor_pin = 34;
int pump_pin = 23;

// Length of the Byte array for the Commands over Bluetouth
const int commandLength = 6;
byte command[commandLength];

void setup() {
  // EEPROM - Configure the memory and read the targetValue
  EEPROM.begin(EEPROM_SIZE);
  targetValue = EEPROM.read(1);
  enabled = false;
  
  // Configure Pins  
  pinMode(pump_pin, OUTPUT);
  digitalWrite(pump_pin, HIGH);

  // Display setup
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.drawString(64,20,"Testing");
  display.display();
  delay(2000);
  
  // Clear Display
  display.clear();

  // Setup Serial Interface and Bluetooth Connection
  Serial.begin(115200);
  SerialBT.begin("Gruppe 17 - Bewässerung");
  Serial.println("The device started, now you can pair it with bluetooth");

  // Debug Info to check if the the targetValue was Saved (Can be deleted)
  Serial.print("Read targetValue: ");
  Serial.println(targetValue);
}

void loop() {
  // From Chip to Phone, when you write in the Serial Monitor (Can be deleted) 
  if(Serial.available()){
    SerialBT.write(Serial.read());
  }

  // Read value of Sensor and Map map the Values to values between 0 and 100 for easier handling
  output_value = analogRead(sensor_pin);
  output_value = map(output_value, 3300, 1640, 0, 100);
    
  // Output current Value on Display Display
  display.clear();
  display.drawString(50, 20, String(output_value) + "%");
  display.display();
  delay(1000);

  // Watering System - Checks only if it was enabled
  if(enabled == true && delayTimer > 10){
  delayTimer = 0;
    // Check Sensor Value against Configured Value
    if(output_value < targetValue){
      Serial.println("PUMP ON - Automatic"); // DEBUG ------------------------------------------------------------ DEBUG
    
      digitalWrite(pump_pin, LOW);
      delay(10000);
      digitalWrite(pump_pin, HIGH);

      Serial.println("PUMP OFF - Automatic"); // DEBUG ------------------------------------------------------------ DEBUG
    }
  }
  
  // Bluetooth Connection from Phone to Chip to configure the Programm
  if(SerialBT.available()){
    // Read Command
    for(int i = 0; i < commandLength; i++){
      if(SerialBT.available()){
        command[i] = SerialBT.read();
      } else {
        command[i] = '-';
      }
    }

    // Output on Serial Monitor on PC (can be deleted) 
    for(int i = 0; i < commandLength; i++){
      Serial.write(command[i]);
    }
    Serial.println();
    
    // ||------------------------------------||
    // || Commands to configure the Programm ||
    // ||------------------------------------||

    // Enable / Disable Watering System
    bool prevStatus = enabled;
    if(command[0] == 'e'){
      enabled = true;
    } else if(command[0] == 'd'){
      enabled = false;
    }

    // Change the target value
    if(command[0] == 's'){      
      String inString = "";
      for(int i = 2; i < commandLength; i++){
        if(command[i] != '-'){
          inString += (char)command[i];  
        } else {
          break;
        }
      }
      // Override the current targetValue and save it
      targetValue = inString.toInt();
      EEPROM.write(1, targetValue);
      EEPROM.commit();
    }

    // Sends information to the connected device over Bluetooth
    if(command[0] == '?'){
      SerialBT.write('s');
      SerialBT.write('0');
      SerialBT.write('=');
      SerialBT.print(targetValue);
      
      if(enabled){
        SerialBT.write('e');
      } else {
        SerialBT.write('d');
      }
      SerialBT.println();
    }

    // Manually engable the Pump for 5 Sekonds
    if(command[0] == 'p'){
      Serial.println("PUMP ON - Manuell"); // DEBUG ------------------------------------------------------------ DEBUG

      digitalWrite(pump_pin, LOW);
      delay(5000);
      digitalWrite(pump_pin, HIGH);

      Serial.println("PUMP OFF - Manuell"); // DEBUG ------------------------------------------------------------ DEBUG

    }
  }
  delayTimer += 1;
}
