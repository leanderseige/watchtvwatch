/*********

Watch-TV-Watch

This watch helps you and your family to determine who will decide what the family watches on TV tonight.

Copyright (C) 2023 Leander Seige

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

*********/

#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "<YOUR_SSID>";
const char* password = "<YOUR PASSPHRASE>";

WiFiServer server(80);

const long utcOffsetInSeconds = 3600;

int pin = 2;

// you probably want to adjust these values to the behaviour of your stepper motor
#define IN1 5
#define IN2 4
#define IN3 14
#define IN4 12
const int stepsPerRevolution = 51;  // change this to fit the number of steps per revolution

// replace ChildOne and ChildTwo with the names of you family members
const int PosMama = 0;
const int PosPapa = 72;
const int PosChildTwo = 48;
const int PosChildOne = 24;

int Positions[] = {PosMama,PosPapa,PosChildTwo,PosChildOne};
String PosNames[] = {"Mama","Papa","ChildTwo","ChildOne"};
int currentPos = 0; // Mama
unsigned long int day = 0;

bool powerSaving=false;

int Offset = -3;
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  // initialize GPIO 2 as an output.
 // pinMode(pin, OUTPUT);
 pinMode(13,INPUT);
 pinMode(15,OUTPUT);
// Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address on serial monitor
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");    //URL IP to be typed in mobile/desktop browser
  Serial.print(WiFi.localIP());
  Serial.println("/");

  timeClient.begin();
  timeClient.update();

  day = (timeClient.getEpochTime()/60/60/24)%4;
  currentPos = day;

  // set the speed and acceleration
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(100);

  int moveTo = Positions[currentPos]+Offset;
  Serial.print("Moving to ");
  Serial.println(moveTo);
  stepper.moveTo(moveTo);

}

void motorManager() {
  if (stepper.distanceToGo() != 0){
    digitalWrite(15,LOW);
    if (powerSaving==true) {
      powerSaving=false;
      stepper.enableOutputs();
      delay(200);
    }
    stepper.run();
  } else {
    digitalWrite(15,HIGH);
    if (powerSaving==false) {
      powerSaving=true;
      delay(200);
      stepper.disableOutputs();    
    }
  }
}

// the loop function runs over and over again forever
void loop() {

  int moveTo = 0;
  
  if(digitalRead(13)==HIGH && stepper.distanceToGo()==0) {
    currentPos=random(0, 4);
    Serial.println(currentPos);
    moveTo= Positions[currentPos]+Offset;
    Serial.print("Moving to ");
    Serial.println(moveTo);
    stepper.moveTo(moveTo);
    stepper.run();
  } 
  motorManager();

  WiFiClient client = server.accept();
  if (client) {
    Serial.println("\n[Client connected]");
    while (client.connected()) {
      motorManager();
      // read line by line what the client (web browser) is lineing
      if (client.available()) {
        String line = client.readStringUntil('\r');
    
        if (line.indexOf("/Command=forward") != -1)  { //Move 50 steps forward
          Serial.println("Moving Forward");
          Offset = Offset+1;
        } else if (line.indexOf("/Command=backward") != -1)  { //Move 50 steps backwards
          Serial.println("Moving Backward");
          Offset = Offset-1;
        } else if (line.indexOf("/Command=Reset") != -1)  { //Move 50 steps backwards
          Serial.println("Moving Reset");
          stepper.setCurrentPosition(0);
          Offset = 0;          
        } else if (line.indexOf("/Command=Mama") != -1)  { //Move 50 steps backwards
          Serial.println("Moving Mama");
          currentPos=0;
        } else if (line.indexOf("/Command=Papa") != -1)  { //Move 50 steps backwards
          Serial.println("Moving Papa");
          currentPos=1;
        } else if (line.indexOf("/Command=ChildTwo") != -1)  { //Move 50 steps backwards
          Serial.println("Moving ChildTwo");
          currentPos=2;
        } else if (line.indexOf("/Command=ChildOne") != -1)  { //Move 50 steps backwards
          Serial.println("Moving ChildOne");
          currentPos=3;
        } else {
          Serial.println("No Action");
        }

        moveTo = Positions[currentPos]+Offset;
        Serial.print("Moving to ");
        Serial.println(moveTo);
        stepper.moveTo(moveTo);
        stepper.run();
  
        // Return the response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println(""); //  do not forget this one
        client.println("<!DOCTYPE HTML>");
        client.println("<html>"); 
        client.println("<body>");
        client.println("<style>body {font-size:4vw;font-family:sans-serif;}</style>");    
        client.println("<style>a, button {display:inline-block;padding:4vw;margin:1vw;font-size:6vw;font-family:sans-serif;}</style>");  
        client.print("<p>");
        client.print("Status: ");
        client.print(PosNames[currentPos]);
        client.print("</p>");
        client.print("<p>");
        client.print("Offset: ");
        client.print(Offset);
        client.print("</p>");  
        client.print("<p>");
        client.print("Power Saving: ");
        client.print(powerSaving?"YES":"No");
        client.print("</p>");
        client.println("<a href=\"/Command=forward\"\"><button>Forward </button></a><br />");
        client.println("<a href=\"/Command=backward\"\"><button>Backward </button></a><br />");  
        client.println("<a href=\"/Command=Reset\"\"><button>Reset </button></a><br />");         
        client.println("<a href=\"/Command=Mama\"\"><button>Mama </button></a><br />");
        client.println("<a href=\"/Command=Papa\"\"><button>Papa </button></a><br />");  
        client.println("<a href=\"/Command=ChildTwo\"\"><button>ChildTwo </button></a><br />");
        client.println("<a href=\"/Command=ChildOne\"\"><button>ChildOne </button></a><br />");  
        client.println("</body>");
        client.println("</html>");
        break;
      }
    }  
  }

  while (client.available()) {
    motorManager();
    client.read();
  }

  client.flush();
  client.stop();
  // Serial.println("[Client disconnected]");
}
