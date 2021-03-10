#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include <Keypad_I2C.h>
#include <Keypad.h>        // GDY120705
#include <Wire.h>

// keypad access control

char password[4];
char initial_password[4], new_password[4];
int i = 0;


#define I2CADDR 0x26 // keypad adress 

const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'0','4','A'},
  {'1','5','B'},
  {'2','6','C'},
  {'3','7','D'},
};
byte colPins[COLS] = {0, 1, 2};  //connect to the column pinouts of the keypad
byte rowPins[ROWS] ={4, 5, 6,7 }; //connect to the row pinouts of the keypad

//initialize an instance of class NewKeypad

Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

///////////////////////////////////////// ACCESS CONTROL DEFINE ///////////////////////////////////


#define LED_ON HIGH
#define LED_OFF LOW

#define led_a 7    // Set Led Pins
#define led_b 6
#define led_c 5
#define led_d 14



#define relay 15
#define gate  4    // Pin output signial to gate
#define btn_reset 3 // Write eemprom with "0123" this is the default password 
#define btn_inside 2 // Pin input call function Access granted

bool programMode = false;  // initialize programming mode to false
uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module

#define SS_PIN 10
#define RST_PIN 9
boolean toggle = false;
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.z


///////////////////////////////////////// Setup ///////////////////////////////////


void setup() {

  //Arduino Pin Configuration
  pinMode(led_a, OUTPUT);
  pinMode(led_b, OUTPUT);
  pinMode(led_c, OUTPUT);
  pinMode(led_d, OUTPUT);

  pinMode(relay, OUTPUT);
  pinMode(gate, OUTPUT);
  pinMode(btn_reset, INPUT);
  pinMode(btn_inside, INPUT);

  digitalWrite(gate, LOW);    // Make sure door is locked
  digitalWrite(led_a, LED_OFF);  // Make sure led is off
  digitalWrite(led_b, LED_OFF);  // Make sure led is off
  digitalWrite(led_c, LED_OFF); // Make sure led is off

  //Protocol Configuration

  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  customKeypad.begin( );  // keypad
  read_password();


  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);// Antenna Gain to Max it will increase reading distance

  Serial.println(F("Access control garage"));
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

}

void loop () {

  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    char key_pressed = customKeypad.getKey();


  if (key_pressed != NO_KEY) {
    Serial.println(key_pressed);

    
     // open with password
    if (key_pressed == 'A') {
      digitalWrite(led_a, LED_ON);  // Make sure led is off
      digitalWrite(led_b, LED_OFF);  // Make sure led is off
      digitalWrite(led_c, LED_OFF); // Make sure led is off
      digitalWrite(led_d, LED_OFF); // Make sure led is off
      access_with_keypad();
      
    }
    
    // change password
    
    if (key_pressed == 'B') {
      digitalWrite(led_a, LED_OFF);  // Make sure led is off
      digitalWrite(led_b, LED_ON);  // Make sure led is off
      digitalWrite(led_c, LED_OFF); // Make sure led is off
      digitalWrite(led_d, LED_OFF); // Make sure led is off
      change_password();
    }

    
    // programing mode
    if (key_pressed == 'C') {
        Serial.println("Key C pressed, option programing mode");
        digitalWrite(led_a, LED_OFF);  // Make sure led is off
        digitalWrite(led_b, LED_OFF);  // Make sure led is off
        digitalWrite(led_c, LED_ON); // Make sure led is off
        digitalWrite(led_d, LED_OFF); // Make sure led is off
        set_in_program_mode(); 
    }

    if ( key_pressed == 'D' ) {
      toggle_relay();
  }
    
  }

  if (digitalRead(btn_reset) == HIGH)
  {
    Serial.println("Wipe Memory");
    WipeMemoryTags();
  }

  if (digitalRead(btn_inside) == HIGH)
  {
    Serial.println("Call to granted function");
    granted();

  }

  }

  while (!successRead);   //the program will not go further while you are not getting a successful read
    

  if (programMode) {
    if ( findID(readCard) ) { // If scanned card is known delete it
      Serial.println(F("I know this PICC, removing..."));
      deleteID(readCard);
      Serial.println("-----------------------------");
      Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
    }
    else {                    // If scanned card is not known add it
      Serial.println(F("I do not know this PICC, adding..."));
      writeID(readCard);
      Serial.println(F("-----------------------------"));
      Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));

    }

  } else {
    Serial.println("Normal mode on");
    normalModeOn(); // Normal mode, blue power led is on, all others are off
    if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
      Serial.println(F("Welcome, You shall pass"));
      granted();         // Open the door lock for 300 ms

    }
    else {      // If not, show that the ID was not valid
      Serial.println(F("You shall not pass"));
      leds_off();
    }

  }
}
////////////////////////////////////////// TOGGLE RELAY ////////////////////////////////////////
void toggle_relay()
{
  Serial.println("Toggle_relay");
  toggle = !toggle;
  digitalWrite(relay,toggle);
  delay(500);
  
  }


/////////////////////////////////////////  PROGRAM MODE    /////////////////////////////////////
void set_in_program_mode() {
  Serial.println("Write Password");
   int j = 0;

  while (j < 4){
  if (digitalRead(btn_inside) == HIGH)
  {
    Serial.println("Call to granted function");
    granted();}
  if ( mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
         Serial.println("Tag is present, exit loop change password");
    leds_off();
    return;}
    char key = customKeypad.getKey();
   if (key)
    {
       if (key == 'C') {
        Serial.println("Come back to loop");
        programMode = false;
        leds_off();

        return ;
          }
       if (key == 'D') {
       j = 0;
       Serial.println("Clear imput memo");
       led_d_on();
       }
       else{
      new_password[j++] = key;
      Serial.println(key);
    }
    }
    key = 0;
  }
  delay(50);
  if ((strncmp(new_password, initial_password, 4)))
  {
    Serial.println("Wrong password, try again");
    delay(50);
  }
  else
  {
    programMode = true;
    Serial.println(F("Hello- entered program mode"));
    uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
    Serial.print(F("I have "));     // stores the number of ID's in EEPROM
    Serial.print(count);
    Serial.print(F(" record(s) on EEPROM"));
    Serial.println("");
    Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted () {
  digitalWrite(led_c, LED_ON);  
  digitalWrite(led_a, LED_ON);  
  digitalWrite(led_b, LED_ON);  
  digitalWrite(led_d, LED_ON); 
  digitalWrite(gate, HIGH);    
  delay(200);
  digitalWrite(gate, LOW);    
  digitalWrite(led_b, LED_OFF);  
  digitalWrite(led_c, LED_OFF);  
  digitalWrite(led_a, LED_OFF); 
  digitalWrite(led_d, LED_OFF); 
 
}

///////////////////////////////////////// Access leds_off  ///////////////////////////////////
void leds_off() {
  digitalWrite(led_d, LED_OFF);
  digitalWrite(led_b, LED_OFF); 
  digitalWrite(led_c, LED_OFF);   
  digitalWrite(led_a, LED_OFF); 
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 software version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    // Visualize system is halted
    digitalWrite(led_b, LED_OFF);  // Make sure green LED is off
    digitalWrite(led_c, LED_OFF);   // Make sure blue LED is off
    digitalWrite(led_a, LED_ON);   // Turn on red LED
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////


void led_d_on () {
  digitalWrite(led_d, LED_ON);
  delay(400);
  digitalWrite(led_d, LED_OFF);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(led_c, LED_OFF);  // Blue LED ON and ready to read card
  digitalWrite(led_a, LED_OFF);  // Make sure Red LED is off
  digitalWrite(led_b, LED_OFF);  // Make sure Green LED is off
  digitalWrite(led_d, LED_OFF);
  digitalWrite(gate, LOW);    // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
      return false;
    }
  }
  return true;
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
    }
    else {    // If not, return false
    }
  }
  return false;
}


///////////////////////////////////////// WIPE Start   ///////////////////////////////////

void WipeMemoryTags() {  // when button pressed pin should get low, button connected to ground

  digitalWrite(led_a, LED_ON); // Red Led stays on to inform user we are going to wipe
  digitalWrite(led_b, LED_OFF);  // Make sure led is off
  digitalWrite(led_c, LED_OFF); // Make sure led is off

  Serial.println(F("Wipe button pressed"));
  Serial.println(F("You have 5 seconds to cancel"));
  Serial.println(F("This will be remove all records and cannot be undone"));
  bool buttonWipe = monitorWipeButton(5000); // Give user enough time to cancel operation
  if (buttonWipe == true ) {    // If button still be pressed, wipe EEPROM
    Serial.println(F("Starting wiping EEPROM"));
    for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
      if (EEPROM.read(x) == 0) {              //If EEPROM address 0
        // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
      }
      else {
        EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
      }
    }
    
    Serial.println(F("EEPROM successfully wiped"));
    Serial.println(F("Setting default password"));

    set_default_pass();
   
  }
  else {
    Serial.println(F("Wiping Cancelled")); // Show some feedback that the wipe button did not pressed for 15 seconds
    digitalWrite(led_a, LED_OFF);
  }
}




bool monitorWipeButton(uint32_t interval) {
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval)  {
    // check on every half a second
    if (((uint32_t)millis() % 500) == 0) {
      if (digitalRead(btn_reset) == HIGH)
      {
        return false;
      }
    }
    return true;
  }
}


// KEY ACCESS FUNCTIONS

void change_password() {
  Serial.println("Key B pressed, option change password");
  Serial.println("Write Current Password");
  int j = 0;

  while (j < 4) {
  if (digitalRead(btn_inside) == HIGH)
  {
    Serial.println("Call to granted function");
    granted();
     return ;}
   if ( mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
         Serial.println("Tag is present, exit loop change password");
    leds_off();
    return;}
  
    char key = customKeypad.getKey();
    if (key)
    {
       if (key == 'B') {
        Serial.println("Come back to loop");
        leds_off();

        return ;
          }
       if (key == 'D') {
       j = 0;
       Serial.println("Clear imput memo");
       led_d_on();
       }
       else{
      new_password[j++] = key;
      Serial.println(key);
    }
    }
    key = 0;
  }
  delay(100);
 
  for(int j=0;j<4;j++)
   initial_password[j]=EEPROM.read(j+100);
  
    if ((strncmp(new_password, initial_password, 4)))
  {
    Serial.println("Wrong Password, exit loop change password");
    leds_off();
     return ; 
  }
  else
  {
    j = 0;
    Serial.println("New Password:");
    while (j < 4)
    {
    if (digitalRead(btn_inside) == HIGH)
  {
    Serial.println("Call to granted function");
    granted();
     return ;}
    if ( mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
       Serial.println("Tag is present, exit loop change password");
    leds_off();
    return;
  }
    
      char key = customKeypad.getKey();
      if (key)
      {
        initial_password[j] = key;
        Serial.println(key);
        EEPROM.write(j+100, key);
        j++;
      }
    }
    Serial.println("Pass Changed");
    leds_off();
    return ;

  }

}



void read_password() {
  for (int j = 0; j < 4; j++){
    initial_password[j] = EEPROM.read(j+100);
    Serial.println("Reading password from memory: ");
    Serial.println(EEPROM.read(j+100));

}
}

char n = '2'; 

void set_default_pass() {
  for(int j=0;j<4;j++)
  
   EEPROM.write(100+j, n);
}

void access_with_keypad()
{
  Serial.println("Key A pressed,option access with keypad key");
  Serial.println("Write Password");
  int j = 0;
  while (j < 4)
  {
    if (digitalRead(btn_inside) == HIGH)
  {
    Serial.println("Call to granted function");
    granted();
    return ;}

    if ( mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
             Serial.println("Tag is present, exit loop change password");
    leds_off();
    return;}
    char key = customKeypad.getKey();
    if (key)
    {
       if (key == 'A') {
        Serial.println("Come back to loop");
        leds_off();
        return ;
          }
       if (key == 'D') {
       j = 0;
       Serial.println("Clear imput memo");
       led_d_on();
       }
       else{
      new_password[j++] = key;
      Serial.println(key);
    }
    }
    key = 0;
  }
  delay(100);
  if ((strncmp(new_password, initial_password, 4)))
  {
    Serial.println("Wrong Password.");
    leds_off();
     return ;
  }
  else
  {
    Serial.println("Welcone, You shall pass");
    granted();
    leds_off();
     return ;
   
  }

}
