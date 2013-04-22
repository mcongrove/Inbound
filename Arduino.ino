// Include necessary libraries
#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>

// Set constants
#define API_QUERY "GET /spaceapps/api.php HTTP/1.0"
#define API_QUERY_FREQUENCY 14400
#define API_DELIMITER "%"
#define API_SPECIAL "!"
#define LED_SIZE 8

// Ethernet configuration
EthernetClient  _Ethernet;
byte            _MacAddress[]       = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
int             _EthernetAttempts   = 4;
boolean         _EthernetConnected  = false;
int             _EthernetLastConnection;
boolean         _RequestFlag        = false;

// Define API information
IPAddress       _APIserverAddress(72,14,177,18);
String          _APIdata         = "";
boolean         _APIreadingData  = false;

// Define pin-outs
int             _LEDs[LED_SIZE]  = { 12, 11, 10, 9, 4, 5, 6, 7 };
int             _LEDearth        = 8;
int             _LEDsun          = 13;


/************************/
/*** GLOBAL FUNCTIONS ***/
/************************/


void setup() {
  // Initializes serial output
  Serial.begin(9600);
  
  EthernetConnect();
  ApiQuery();
  
  LedSetOutput();
  LedSetEarthSun();
}

void loop() {
  // Handle server data
  if(_Ethernet.available()) {
    if(_RequestFlag) {
      _RequestFlag = false;
      
      LedAllClear();
    }
    // Read the data from the client
    char _Data = _Ethernet.read();
    
    ApiHandleData(_Data);
  }
  
  // Handle server disconnect
  if(_EthernetConnected && !_Ethernet.connected()) {
    Serial.println("Disconnecting");
    
    // Kill the connection
    _Ethernet.stop();
    _EthernetConnected = false;
    
    reset();
  }
  
  if(!_EthernetLastConnection || (now() - _EthernetLastConnection) >= API_QUERY_FREQUENCY) {
    ApiQuery();
  }
}

void reset() {
  // Reset variables
  _APIreadingData = false;
  _APIdata    = "";
}

void killAll() {
  for(;;) {
    ;
  }
}


/****************/
/*** ETHERNET ***/
/****************/


void EthernetConnect() {
   // Await ethernet initialization
  delay(1000);
  
  // Initializes ethernet
  if(Ethernet.begin(_MacAddress) == 0) {
    Serial.println("Failed to configure DHCP");
    
    killAll();
  }
  
  Serial.print("Connecting to internet with IP of: ");
  Serial.println(Ethernet.localIP());
}


/****************/
/*** DATA API ***/
/****************/


void ApiQuery() {
  _EthernetLastConnection = now();
  
  // If connection is received, continue
  if(_Ethernet.connect(_APIserverAddress, 80)) {
    Serial.println("Connected");
    
    _EthernetConnected = true;
    _RequestFlag = true;
    
    // Perform an HTTP request
    _Ethernet.println(API_QUERY);
    _Ethernet.println();
  } else {
    // Connection failed
    Serial.println("Connection failed");
    
    // Retry
    if(_EthernetAttempts > 0) {
      _EthernetAttempts--;
      
      ApiQuery();
    }
  }
}

void ApiHandleData(char _Data) {
  // Detect start and end of valid data string
  if(!_APIreadingData && String(_Data) == API_DELIMITER) { _APIreadingData = true; return; }
  if(_APIreadingData && String(_Data) == API_DELIMITER) { _APIreadingData = false; return; }
  
  if(_APIreadingData) {
    // If we reach value delimiter, turn on the LED
    if(String(_Data) == ",") {
      LedSetOn(_APIdata.toInt());
      
      _APIdata = "";
    } else if(String(_APIdata) == "0") {
      // If we have a new CME, flash the Sun
      LedSunFlash();
    } else if(String(_Data) == API_SPECIAL) {
      // If a CME just hit earth, flash all the lights
      LedAllFlash();
    } else {
      // Save the values
      _APIdata = _APIdata + String(_Data);
    }
  }
}


/*******************/
/*** LED CONTROL ***/
/*******************/


void LedSetOutput() {
  int i;
  
  for (i = 0; i < LED_SIZE; i++) {
    pinMode(_LEDs[i], OUTPUT);
  }
}

void LedSetOn(int _Index) {
  digitalWrite(_LEDs[_Index], HIGH);
}

void LedSetEarthSun() {
  pinMode(_LEDearth, OUTPUT);
  pinMode(_LEDsun, OUTPUT);
  
  digitalWrite(_LEDearth, LOW);
  digitalWrite(_LEDsun, LOW);
}

void LedAllClear() {
  int i;
  
  for (i = 0; i < LED_SIZE; i++) {
    digitalWrite(_LEDs[i], LOW);
  }  
}

void LedAllFlash() {
  int i;
  
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], LOW); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], HIGH); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], LOW); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], HIGH); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], LOW); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], HIGH); }
  delay(250);
  for (i = 0; i < LED_SIZE; i++) { digitalWrite(_LEDs[i], LOW); }
}

void LedSunFlash() {
  digitalWrite(_LEDsun, HIGH);
  delay(250);
  digitalWrite(_LEDsun, LOW);
  delay(250);
  digitalWrite(_LEDsun, HIGH);
  delay(250);
  digitalWrite(_LEDsun, LOW);
  delay(250);
  digitalWrite(_LEDsun, HIGH);
  delay(250);
  digitalWrite(_LEDsun, LOW);
}
