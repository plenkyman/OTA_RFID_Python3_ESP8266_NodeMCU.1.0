/*
 * RFID Door Access System, works on ESP8266, NodeMCU 1.0
 * Remixed by plenkyman, V 1.00, January 2017
 * works with a python3 script running on a nother machine logging entries.
 *  -> define keys(UID) and adjust their number
 *  -> adjust all relevant data for Wifi, OTA and machcine running the python3 script
 * ----------------------------------------------------------------------------
 * Based on MFRC522 library; see https://github.com/miguelbalboa/rfid
 * pin layout used:
 *--------------------------------
 * MFRC522      ESP8266    Relay  
 * Reader/PCD   NodeMCU            
 * Pin          Pin       
 * -------------------------------
 * RST          D2          
 * SDA(SS)      D4         
 * MOSI         D7 
 * MISO         D6 
 * SCK          D5
 * GND          GND
 * 3.3V         3.3V
 * ---------------------------------
 *              GND       GND
 *              Vin       VCC
 *              D8        In1
 * ---------------------------------                     
 * LEDS:  Green  D3, RED  D1
 */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>
//#*#*#*#*#*#*#*#*#*#*#*#*#*#      Adjust all data below     #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*
const char* ssid = "SomeSSID";
const char* password = "SomeWifiPassword";
// port and ip for python server listening and logging
const uint16_t listenerport = 9999;
const char * listenerhost = "192.168.1.15"; // ip
const char* OTAhostname = "RFID_NodeMCU_ESP8266";
const char* OTApassword = "SomePassword";
int opendoortime = 2000;


//Array with keys
String AllKeys[] = {
"D348762A", // 1. John Doe
"D347762A", // 2. Mary Jane
"D346762A", // 3. Tom Ford
"D345762A", // 4. Donald
"D344762A", // 5. Victor
"D343762A"  // 6. Frank
    };
int AllKeysLenght = 6; // the number of keys registered in AllKeys
String RestartKey = "34FB795A";
//#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*

#define RST_PIN         D2
#define SS_PIN          D4
#define RELAY1          D8
#define GREENLED        D3
#define REDLED          D1

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

void setup() {  
digitalWrite(LED_BUILTIN, LOW);   // start setup, internal LED ON
Serial.begin(115200); // start com
WiFi.mode(WIFI_STA); // define Wifi mode
WiFi.begin(ssid, password); // connect wifi
while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();}
// Over The Air setup  
ArduinoOTA.setHostname(OTAhostname);
ArduinoOTA.setPassword(OTApassword);
ArduinoOTA.begin();
//setup LED and Relay
pinMode(LED_BUILTIN, OUTPUT);
pinMode(GREENLED, OUTPUT);
pinMode(REDLED, OUTPUT);
pinMode(RELAY1, OUTPUT);
// start SPI and MFRC522
SPI.begin();        // Init SPI bus
mfrc522.PCD_Init(); // Init MFRC522 card
// internal LED OFF, Setup complete
digitalWrite(LED_BUILTIN, HIGH);   // Show setup on LED_BUILTIN
} // setup complete

// the main program
void loop() {
//prepare OTA
ArduinoOTA.handle();
// do the door stuff
digitalWrite(GREENLED, HIGH);
delay(6);
digitalWrite(GREENLED, LOW);
delay(70);
digitalWrite(REDLED, HIGH);
delay(6);
digitalWrite(REDLED, LOW);
delay(500);
int  AuthResponse = 0; // Set AuthResponse to 0
// Look for new cards
if ( ! mfrc522.PICC_IsNewCardPresent())
return;
// Select one of the cards
if ( ! mfrc522.PICC_ReadCardSerial())
return;
String comparerkey = ""; // declare comparerkey
for (byte i = 0; i < mfrc522.uid.size; i++) {    // get key from reader
    comparerkey += String(mfrc522.uid.uidByte[i], HEX);}
comparerkey.toUpperCase(); // make it uppercase   
// check if key in AllKeys
for (int thisKey = 0; thisKey < AllKeysLenght; thisKey++) {  // cycle thru array AllKeys
    if (AllKeys[thisKey].equalsIgnoreCase(comparerkey)){  // checks if UID is in array AllKeys 
          AuthResponse = 1;}}
String tosendmessage = "";
if (AuthResponse == 1){
    tosendmessage = "OK " + comparerkey;
    keyok(tosendmessage);}
else{
    // check for program card
    if (RestartKey.equalsIgnoreCase(comparerkey)){  RestartMCU();}   
    else{
        // log unknown card
        tosendmessage = "UNKNOWN " + comparerkey;
        keynotok(tosendmessage);}}
// reset response
AuthResponse = 0; 
// Halt PICC
mfrc522.PICC_HaltA();
 // Stop encryption on PCD
mfrc522.PCD_StopCrypto1();
} // main program complete

//open door and led
void keyok(String passit){
  digitalWrite(GREENLED, HIGH);
  digitalWrite(RELAY1, HIGH);
  sendmessage(passit);
  delay(opendoortime);
  digitalWrite(GREENLED, LOW);
  digitalWrite(RELAY1, LOW);
}
//key not ok
void keynotok(String passit){
    digitalWrite(REDLED, HIGH);
    sendmessage(passit);     
    digitalWrite(REDLED, LOW);
}
//send a tcp message
void sendmessage(String getkey) {
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    if (!client.connect(listenerhost, listenerport)) {
        delay(2000);
        return;}
    // This will send the request to the server
    client.print(getkey);
    //read back one line from server
    String line = client.readString();
    Serial.println(line);
    if (line == "OK"){
        blinkit(LED_BUILTIN,20,30);}
    // RestartMCU with card
    if (line == "NetworkRestartCMD"){
        RestartMCU();}
    client.stop();} 
     
// blink internal LED when message received
void blinkit(int LED, int count, int wait){ 
  for (int i = 0; i < count; i++) { 
         digitalWrite(LED, LOW);
         delay(wait);
         digitalWrite(LED, HIGH);
         delay(wait);}}
// Restart
void RestartMCU(){
  digitalWrite(REDLED, HIGH);
  digitalWrite(GREENLED, HIGH);
  sendmessage("Restarting NodeMCU_1.0");
  blinkit(REDLED,50,30);
  blinkit(GREENLED,50,30); 
  delay(1000);                   
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  ESP.restart();}


