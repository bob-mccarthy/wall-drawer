
#include <Arduino.h>
#include <WiFi.h>
#include "indexHtml.h"
#include "FastAccelStepper.h"

const char *ssid = "MAKERSPACE";
const char *password = "12345678";

NetworkServer server(80);

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *leftStepper = NULL;
FastAccelStepper *rightStepper = NULL;

FastAccelStepper *slowerStepper = NULL;
FastAccelStepper *fasterStepper = NULL;

const int leftStepPin= D4;
const int leftDirPin = D5;

const int rightStepPin = D9;
const int rightDirPin = D10;


void setup() {
  Serial.begin(115200);
  
  pinMode(leftStepPin, OUTPUT);
  pinMode(rightStepPin, OUTPUT);
  pinMode(leftDirPin, OUTPUT);
  pinMode(rightDirPin, OUTPUT);
  engine.init();
  leftStepper = engine.stepperConnectToPin(leftStepPin);
  rightStepper = engine.stepperConnectToPin(rightStepPin);
  leftStepper->setDirectionPin(leftDirPin);
  rightStepper->setDirectionPin(rightDirPin);
  leftStepper->setSpeedInHz(1000);
  rightStepper->setSpeedInHz(1000);
  leftStepper->setAcceleration(1000);
  rightStepper->setAcceleration(1000);


  // We start by connecting to a WiFi network

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
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        if (c == '\n') {            // if the byte is a newline character
          int postIndex = currentLine.indexOf("POST /moveSync");
          if (postIndex != -1){
            Serial.println(currentLine);
            Serial.println(postIndex);
            Serial.println(currentLine[postIndex + 11]);
            Serial.println("beginning moveSync handling");
            long params[2] = {0,0};
            uint8_t currParam = 1;
            long digitPlace = 1; //determines the tens place of the current digit we are looking at
            for(int i = currentLine.length() - 10; i > postIndex + 10; i--){
              if(currentLine[i] == '/'){
                currParam -= 1;
                digitPlace = 1;
                continue;
              }
              if(currentLine[i] == '-'){
                params[currParam] *= -1;
                continue;
              }
              Serial.println(currentLine[i]);
              params[currParam] +=  (long)(currentLine[i] - '0') * digitPlace;
              digitPlace *= 10;
            }

            float speedToDistanceRatio;
            long shorterDist;
            long longerDist;
            if (abs(params[0] - leftStepper->getCurrentPosition()) < abs(params[1] - rightStepper->getCurrentPosition())){
              slowerStepper = leftStepper;
              fasterStepper = rightStepper;
              speedToDistanceRatio = 1000 / abs(rightStepper->getCurrentPosition() - params[1]);
              shorterDist = params[0];
              longerDist = params[1];
            }
            else{
              slowerStepper = rightStepper;
              fasterStepper = leftStepper;
              speedToDistanceRatio = 1000 / abs(rightStepper->getCurrentPosition() - params[0]);
              shorterDist = params[1];
              longerDist = params[0];
            }

            
            fasterStepper->setAcceleration(1000);
            fasterStepper->setSpeedInHz(1000);

            slowerStepper->setAcceleration((int)(speedToDistanceRatio * shorterDist));
            slowerStepper->setSpeedInHz((int)(speedToDistanceRatio * shorterDist));

            slowerStepper->moveTo(shorterDist);
            fasterStepper->moveTo(longerDist);
            
            // leftStepper->moveTo(params[0]);
            // rightStepper->moveTo(params[1]);
            Serial.printf("leftStepperPos: %lu, rightStepperPos: %ld", params[0], params[1]);
            
          }
          else {
            postIndex = currentLine.indexOf("POST /move");
            if (postIndex != -1){
              Serial.println(currentLine);
              Serial.println(postIndex);
              Serial.println(currentLine[postIndex + 11]);
              Serial.println("beginning post request handling");
              long params[2] = {0,0};
              uint8_t currParam = 1;
              long digitPlace = 1; //determines the tens place of the current digit we are looking at
              for(int i = currentLine.length() - 10; i > postIndex + 10; i--){
                if(currentLine[i] == '/'){
                  currParam -= 1;
                  digitPlace = 1;
                  continue;
                }
                if(currentLine[i] == '-'){
                  params[currParam] *= -1;
                  continue;
                }
                Serial.println(currentLine[i]);
                params[currParam] +=  (long)(currentLine[i] - '0') * digitPlace;
                digitPlace *= 10;
              }
              if(params[0] == 0){
                leftStepper->move(params[1]);
              }
              else{
                rightStepper->move(params[1]);
              }
              Serial.printf("motorID: %lu, numSteps: %ld", params[0], params[1]);
            }
          }
          

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print(indexHtml);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
 
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
