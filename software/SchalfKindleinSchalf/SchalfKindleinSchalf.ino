#include <AccelStepper.h>
#include <IRremote.h>
#include <SimpleTimer.h>
#include <EEPROM.h>

//Stepper stepper(100, 4,7,3,8);
AccelStepper stepper(4,4,7,3,8); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

IRrecv irrecv(10);
decode_results results;

SimpleTimer timer;
int ledTimerId = -1;
int stepperTimerId = -1;

int colorMode; // 1,2,3 = rgb fixed an  - 4,5,6 = rgb pulsing - 7 - alle an - 8 - alle pulsing - 9 = rgb fading throuh colors
unsigned int color[3];
unsigned int decColor = 0;
unsigned int incColor = 1;
int colorValue = 0;
bool down = true;

bool off = false;

int ledSpeedLast = -100;
int hysterese = 50;

bool stepperUp;
int stepperValue;
#define STEPPER_WAY 200
int stepperSpeedLast = -100;
bool stepperOff = false;

bool fanOn = true;

void setup() {
  // initialize serial:
  Serial.begin(115200);
  
  pinMode(2, OUTPUT); // Lüfter

  pinMode(9, OUTPUT); // LED blue
  pinMode(6, OUTPUT); // LED green
  pinMode(5, OUTPUT); // LED red
  
  initStepper();
  stepper.moveTo(STEPPER_WAY / 2);
  
  irrecv.enableIRIn(); // Start the receiver

  int mode = EEPROM.read(0);
  if (mode < 0 || mode > 9)
    mode = 1;
  updateLedMode(mode);

  ledLoop();
}

void loop() {
   updateStepperSpeed();

   if (stepper.distanceToGo() == 0)
     stepper.moveTo(-stepper.currentPosition());
   stepper.run();

  timer.run();  
  
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    handleIr(results.value);
    irrecv.resume(); // Receive the next value
  }
  
   int temp = analogRead(A5);
   if ( temp > 680 ) // about 40°C
      fan(true);
   if ( temp < 650 )
      fan(false);    
      
   serialDebugInterface();
}

// ****************** debug *********************************
void serialDebugInterface() {
  // TODO add stepper
  
    // if there's any serial available, read it:
  if (Serial.available() == 0) 
     return;

  int cmd = Serial.read();
    int value;
    int a,b,c;    
  
    switch (cmd) {
      case 'r':    
        value = Serial.parseInt();
        Serial.print("red ");      
        Serial.print(value); 
        analogWrite(5, value);
        break;
      case 'g':    
        value = Serial.parseInt();
        Serial.print("green ");
        Serial.print(value); 
        analogWrite(6, value);
        break;
      case 'b':    
        value = Serial.parseInt();
        Serial.print("blue ");
        Serial.print(value); 
        analogWrite(9, value);
        break;
  
      case 'f':    
        value = Serial.parseInt();
        Serial.print("fan ");      
        Serial.print(value); 
        digitalWrite(2, value > 0 ? 1 : 0);
        break;  
        
       
      case 't':
         value = analogRead(A5);
         Serial.print("temp: ");
        Serial.print(value); 
         break; 
         
      case 'p':
         a = analogRead(A0);
         b = analogRead(A1);
         c = analogRead(A2);
         
         Serial.print(a);
         Serial.print("\t");
         Serial.print(b);
         Serial.print("\t");
         Serial.print(c);
         break;    
       
    }
    

  Serial.print('\n');

}

// ********************* Stepper ****************************
void fan(bool on)
{
  digitalWrite(2, on);
}

// ********************* Stepper ****************************
void initStepper() {
  stepper.setAcceleration(1000);
  stepper.setMaxSpeed(200);
  stepper.setSpeed(200);

  stepper.runToNewPosition(-STEPPER_WAY - 50);
  stepper.runToNewPosition(-STEPPER_WAY - 30);

  stepper.setCurrentPosition( -STEPPER_WAY / 2 );
  stepper.setAcceleration(1000);
  stepper.setMaxSpeed(200);
  stepper.setSpeed(200);
}

void updateStepperSpeed() {
  if (off)
  {
    SetStepperSpeed(0);
    stepperSpeedLast = -100;
    return;
  }
  
  
  int raw = analogRead(A0);
  int raw_min = stepperSpeedLast - hysterese;
  int raw_max = stepperSpeedLast + hysterese;
 
  if((raw != stepperSpeedLast))
  {
    if((raw > raw_max) || (raw<raw_min))
    {     
      SetStepperSpeed(raw / hysterese);
      
      stepperSpeedLast = raw;
      
      stepperOff = false;
    }
  }
}

void SetStepperSpeed(int value) {
   // value 0 ... 20

   stepper.setMaxSpeed(value * 15);
}

// ************************ IR ******************************
void handleIr(long code) {
  switch(code) {
    case 0xFF30CF: // "1"
      updateLedMode(1);
      break;
    case 0xFF18E7: // "2"
      updateLedMode(2);
      break;
    case 0xFF7A85: // "3"
      updateLedMode(3);
      break;
    case 0xFF10EF: // "4"
      updateLedMode(4);
      break;
    case 0xFF38C7: // "5"
      updateLedMode(5);
      break;
    case 0xFF5AA5: // "6"
      updateLedMode(6);
      break;
    case 0xFF42BD: // "7"
      updateLedMode(7);
      break;
    case 0xFF4AB5: // "8"
      updateLedMode(8);
      break;
    case 0xFF52AD: // "9"
      updateLedMode(9);
      break;
    case 0xFF22DD: // "play/pause"
      if (stepperOff)
        stepperSpeedLast = -100; 
      else
        SetStepperSpeed(0);
      
      stepperOff = !stepperOff;
      
      break;
    case 0xFFA25D: // "power"
      off = !off;
      
      if (!off) {
        // eingeschaltet
        updateLedMode(colorMode); // will enable ledTimer
        timer.enable(stepperTimerId);
      }
      else {
        // ausgeschaltet
        timer.disable(ledTimerId);
        timer.disable(stepperTimerId);
        writeRgb(0,0,0);  
      }
                
      break;
  }
}


// ************************ LED *****************************
void ledLoop() {
  
  // Farbe
  if ( colorMode <= 3 || colorMode == 7 ) {
    // kein Farbwechsel
    writeRgb(color[0], color[1], color[2]);  
  }
  else if ( (colorMode >= 4 && colorMode <= 6) || colorMode == 8  ) {
    nextColorPulse();
  }
  else
    nextColorRgb();
    
    
  // Farbwechselgeschwindigkeit
  int raw = analogRead(A1);
  int raw_min = ledSpeedLast - hysterese;
  int raw_max = ledSpeedLast + hysterese;
 
  if((raw != ledSpeedLast))
  {
    if((raw > raw_max) || (raw<raw_min))
    {
     
      SetColorSpeed( raw / hysterese); 
      ledSpeedLast = raw;
    }
  }
}

void SetColorSpeed(int value) {
   if ( ledTimerId >= 0)
     timer.deleteTimer(ledTimerId);   
   ledTimerId = timer.setInterval(value, ledLoop);
}

void updateLedMode(int mode) {
  timer.disable(ledTimerId);
    
   switch(mode) {
     case 1:
     case 4:
     case 9:
       // red
       color[0] = 255;
       color[1] = 0;
       color[2] = 0; 
       break;
     case 2:
     case 5:
       // green
       color[0] = 0;
       color[1] = 255;
       color[2] = 0; 
       break;
     case 3:
     case 6:
       // blue
       color[0] = 0;
       color[1] = 0;
       color[2] = 255; 
       break;       
     case 7:
     case 8:
       color[0] = 255;
       color[1] = 255;
       color[2] = 255; 
       break;
   }     
   
   switch(mode) {
     case 4:
     case 5:
     case 6:
     case 8:
       colorValue = 255;
       down = true;
       break;
     case 9:
       colorValue = 255;
       decColor = 0;
       incColor = 1;
       break;
   }   
   
   colorMode = mode;

   writeRgb(color[0], color[1], color[2]);  
   
   timer.enable(ledTimerId);
   
   EEPROM.write(0, mode);
}

void nextColorPulse() {
  
  colorValue += down ? -1 : 1;
  if (colorValue >= 255 || colorValue <= 1)   {
    down = !down;
  }
  
   switch(colorMode) {
     case 4:
       // red
       color[0] = colorValue;
       break;
     case 5:
       // green
       color[1] = colorValue;
       break;
     case 6:
       // blue
       color[2] = colorValue; 
       break;
     case 8:
       color[0] = colorValue; 
       color[1] = colorValue; 
       color[2] = colorValue; 
       break;
     }
   
   writeRgb(color[0], color[1], color[2]);  
}

// Fährt den Farbkanal decColor herunter und
// incColor hoch.
// Am ende Werden die Kanäe geändert
void nextColorRgb() {
  
  colorValue++;
  if (colorValue > 255 )   {
    colorValue = 0;
  }
   
  color[decColor] -= 1;
  color[incColor] += 1;

  if (color[decColor] == 0)
  {
      decColor++;
      incColor++;
      
      if ( decColor == 3) 
        decColor = 0;
      if ( incColor == 3) 
        incColor = 0;
  }
  
  writeRgb(color[0], color[1], color[2]);  
}

// Schaltet die LEDs wie angegeben und beachtet dabei den
// Dimmer an A2
void writeRgb(long red, long green, long blue) {
  long maxValue = analogRead(A2);

  if ( maxValue == 0)
  {
    analogWrite(5, 0);
    analogWrite(6, 0);
    analogWrite(9, 0); 
    return;
  }

  long r = red  * maxValue/ 1024L;
  if (red > 0 && r < 2) r=2;

  long g = green * maxValue/ 1024L;  
  if (green > 0 && g < 3) g=3;

  long b = blue* maxValue/ 1024L;
  if (blue > 0 && b < 6) b=6;
  
  analogWrite(5, r);
  analogWrite(6, g);
  analogWrite(9, b);
}
