// *********************************************************************************************
// pwrSwtch
// Created on 2015 dec. 13
// author: Nandor Toth
//  
//
// SC5262S serial controller.
// Controls up to 8 CONRAD/RENKFORCE/whatever 433MHz remote controlled devices.
// Only 2 Channels are used from the max available 4. 4 devices per channel.
// 
// Pinout: Arduino -> SC
//          D12 ->  A0
//          D11 ->  A1
//          D10 ->  A4
//          D9  ->  A5
//          D8  ->  A6
//          D7  ->  A7
//          D6  ->  ~TE
//          D5  ->  D0
//
// Protocol:
//            only ASCII chars used: 0 = '0', A = 'A'
//            command items:
//                  channel id = SC5262 channel select. Only channel A, and B are used.
//                                <A> = channel 'A'
//                                <B> = channel 'B'
//                  device id = SC5362 device select. All 4 devices are supported
//                                <1> = dev 1
//                                <2> = dev 2
//                                <3> = dev 3
//                                <4> = dev 4
//                  on/off = As it seems.
//                                <0> = dev off
//                                <1> = dev on
//            command: {<channel id><device id><on/off>}
//                      {<A|B><0|1|2|3|4><0|1>}
//            response: (<channed id><device id><on/off> - OK)
//                      (<A|B><0|1|2|3|4><0|1> - OK)
//            examples:
//                      -> {A01} // turn on channel 'A', dev 1 
//                      <- (A01 - OK)
//                      -> {A00} // turn off channel 'A', dev 1 
//                      <- (A00 - OK)
//
// *********************************************************************************************

int RelayPins[] = {12, 11, 10, 9, 8, 7, 6, 5};

String rs232Cmd = "";         // received rs232 command 
boolean newCmd = false;       // new command received flag
boolean waitingCmd = false;   // '{' received waiting for the rest of the command

long previousMillis = 0;              // will store last time LED was updated
long portResetTimeout = 400;          // interval [ms] after port should reseted to the default state
boolean portStateChanged = false;     // port State changed, start portResetTimeout 

byte data = 0x0;
byte mask = 0x0;
int i = 0;

//#define DEBUG 1

void initIoPins(void);

void initIoPins(){
    for (int i = 0; i < 8; i++)
    {
      digitalWrite(RelayPins[i], LOW);
      pinMode(RelayPins[i], OUTPUT);
    }  
  }

void setup() {
  Serial.begin(57600);
  initIoPins();
}

void loop(){
  String rs232Resp = "";
  char outSt = '_';

  unsigned long currentMillis = millis();

  if (portStateChanged && (currentMillis - previousMillis > portResetTimeout)){
    portStateChanged = false;
    initIoPins();
  }
  
  while (Serial.available() > 0) 
  {
    char ch = Serial.read();
    if (ch == '{')
    {
      rs232Cmd = "";
      waitingCmd = true;
    }
    else if (ch == '}')
    {
      waitingCmd = false;
      newCmd = true;
    }
    else if (waitingCmd)
      rs232Cmd += ch;
   
    if (newCmd)
    {
      rs232Cmd.toUpperCase();
      
      if (rs232Cmd.length() == 3 &&
          (rs232Cmd.charAt(0) == 'A' || rs232Cmd.charAt(0) == 'B') &&
          (rs232Cmd.charAt(1) >= '1' && rs232Cmd.charAt(1) <= '4') &&
          (rs232Cmd.charAt(2) == '0' || rs232Cmd.charAt(2) == '1')){
            
        data = 0x1 << (0x1 & rs232Cmd.charAt(0)-'A');           // channel id
        data |= 0x1 << (0x2 + (0x3 & rs232Cmd.charAt(1)-'1'));  // device id
        data |= 0x40;                                           // transmit enable(~TE)
        data |=  rs232Cmd.charAt(2)-'0' == 0 ? 0x80: 0x0;       // on/off 

        rs232Resp = "";
        for (mask=0x1, i=0; mask>0; mask<<=1, i++){
          if (data & mask){
            digitalWrite(RelayPins[i], HIGH);
          }
          else{
            digitalWrite(RelayPins[i], LOW);
          }
          rs232Resp += digitalRead(RelayPins[i]);
          portStateChanged = true;
          previousMillis = currentMillis;
        }
        #ifdef DEBUG
          Serial.print("Real Arduino GPIO states: ");
          Serial.println(rs232Resp);
        #endif
        Serial.println("(" + rs232Cmd + " - OK)");
      }
      else{
        Serial.println("(" + rs232Cmd + " - INVALID)");
      }
      rs232Cmd = "";
      newCmd = false;      
    }
  } 
}
