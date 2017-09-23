#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial esp8266Module(2, 3);               // RX, TX

String network = "Optics";                            // your access point SSID
String password = "F95JHNYNQP4M";                  // your wifi Access Point password
#define IP "184.106.153.149"                        // IP address of thingspeak.com
String GET = "GET /update?key=ZI1JYTQJUHNEZZ1W";    // replace with your channel key


void setup()
{
  Serial.begin(9600);                               // Setting hardware serial baud rate to 9600
  esp8266Module.begin(9600);                        // Setting softserial baud rate to 9600
  delay(20);
  //setupEsp8266();         // This badboy is for configuration so technically we only need to do this once ever.
}

void loop() 
{                               
  int reading = analogRead(A0);
  float voltage = reading * 5.0;
  voltage /= 1024.0; 
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                             //to degrees ((voltage - 500mV) times 100)
  updateTemp(String(temperatureC));
  delay(15000);
}


//-------------------------------------------------------------------
// Following function sends sensor data to thingspeak.com
//-------------------------------------------------------------------
void updateTemp(String voltage1)
{  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  esp8266Module.println(cmd);
  delay(1500);  // originally 5 seconds
  
  if(esp8266Module.find("Error")) // should get SEND OK if successful
  {
    Serial.println("ERROR while SENDING");
    return;
  }
  
  cmd = GET + "&field1=" + voltage1 + "\r\n";
  esp8266Module.print("AT+CIPSEND=");
  esp8266Module.println(cmd.length());
  delay(300); // originally 15 seconds
  
  if(esp8266Module.find(">"))
  {
    esp8266Module.print(cmd);
    Serial.print(voltage1);
    Serial.println(": Data sent");
  }
  else
  {
    checkConnection();
    esp8266Module.println("AT+CIPCLOSE");
    Serial.println("Connection closed");
  }
}


//-------------------------------------------------------------------
// Check an IP address is found
//-------------------------------------------------------------------
void checkConnection(void)
{
  esp8266Module.println("AT+CIFSR");
  if (esp8266Module.find("0.0.0.0"))
  {
    Serial.println("No IP address found");
  }
}


//-------------------------------------------------------------------
// Following function setup the esp8266, put it in station made and 
// connect to wifi access point.
//------------------------------------------------------------------
void setupEsp8266()                                   
{
    Serial.println("Reseting ESP8266 Wifi Module");
    esp8266Module.flush();
    esp8266Module.println(F("AT+RST"));
    delay(7000);  // was 7 seconds
    if (esp8266Module.find("OK"))
    {
      Serial.println("Found OK");
      Serial.println("Changing espmode");
      esp8266Module.flush();
      changingMode();
      delay(5000);
      esp8266Module.flush();
      connectToWiFi();
    }
    else
    {

        Serial.println("OK not found");

    }
}

//-------------------------------------------------------------------
// Following function sets esp8266 to station mode
//-------------------------------------------------------------------
bool changingMode()
{
    esp8266Module.println(F("AT+CWMODE=1"));
    if (esp8266Module.find("OK"))
    {
        Serial.println("Mode changed");

      return true;
    }
    else if(esp8266Module.find("NO CHANGE")){

        Serial.println("Already in mode 1");

      return true;
    }
    else
    {

        Serial.println("Error while changing mode");

      return false;
    }
}

//-------------------------------------------------------------------
// Following function connects esp8266 to wifi access point
//-------------------------------------------------------------------
bool connectToWiFi()
{

  Serial.print("Attempting to Connect to: ");
  Serial.println(network);

  String cmd = F("AT+CWJAP=\"");
  cmd += network;
  cmd += F("\",\"");
  cmd += password;
  cmd += F("\"");
  esp8266Module.println(cmd);
  delay(15000);
  
  if (esp8266Module.find("OK"))
  {

      Serial.println("Connected to Access Point");
 
    return true;
  }
  else
  {

    Serial.println("Could not connect to Access Point");

    return false;
  }
}


