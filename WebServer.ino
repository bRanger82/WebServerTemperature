

#include <SPI.h>
#include <Ethernet.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include "DHT.h"

#define DHTPIN 2     
#define DHTTYPE DHT11

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 16, 41, 58);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

DHT dht(DHTPIN, DHTTYPE);
SFE_BMP180 pressure;

float h = 0;    //Luftfeuchte auslesen
float t = 0;    //Temperatur auslesen
float r = 0;    //Heat-Index berechnen
double p0 = 0;  //Wert fuer Luftdruck
unsigned long lastButtonTime = millis();
unsigned long interval = 10000;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  dht.begin();
  pressure.begin(); 
}


bool getPressure(double * value)
{
  double T,P;
  char st;

  st = pressure.startTemperature();
  
  if (st != 0)
  {
    delay(st);
    st = pressure.getTemperature(T);
  }
  
  st = pressure.startPressure(3);

  if (st != 0)
  {
    delay(st);
    
    st = pressure.getPressure(P,T);
    
    if (st != 0)
    {
      *value = P;
      return true;
    }
  }
  return false;
}




void loop() 
{
  if(millis() -  lastButtonTime > interval)
  {
    h = dht.readHumidity();     //Luftfeuchte auslesen
    t = dht.readTemperature();  //Temperatur auslesen
    r = dht.computeHeatIndex(t, h, false); //Heat-Index berechnen
    p0 = 0; //Wert fuer Luftdruck
    bool pressAvailable = getPressure(&p0); //Luftdruck lesen 
  
    
    if (isnan(t) || isnan(h) || isnan(r) || !pressAvailable) //Fehler beim Lesen eines der Daten
    {
      h = -1;  //Luftfeuchte auslesen
      t = -1;  //Temperatur auslesen
      r = -1;  //Heat-Index berechnen
      p0 = 1;  //Wert fuer Luftdruck
    } 
    lastButtonTime = millis();
  }
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<title>Arduino Webserver Temperature</title>");
          client.println("<body>");
          client.println("<h3>Hallo</h3>");
          if (h == -1 && t == -1 && r == -1 && p0 == 1)
          {
            client.println("Error while reading sensor values!");
          } else if (h == 0 && t == 0 && r == 0 && p0 == 0)
          {
            client.println("Waiting for reading initial values!");
          } else
          {
            client.print("START");
            client.print("|"); // delim
            client.print("LEN:4"); //4 Werte werden uebertragen
            client.print("|"); // delim
            client.print(h); // Luftfeuchte
            client.print("|"); // delim
            client.print(t); // Temperatur
            client.print("|"); // delim
            client.print(r); // HeatIndex
            client.print("|");
            client.print(p0); //Luftdruck 
            client.print("|"); // delim
            client.println("EOF");  
          }
          client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

