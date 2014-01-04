#include <Stepper.h>
#include <IRremote.h>

Stepper stepper(100, 4,7,3,8); // ?? Reihenfolge

int RECV_PIN = 10;
IRrecv irrecv(RECV_PIN);
decode_results results;

int stepperValue;

void setup() {
  // initialize serial:
  Serial.begin(115200);

  pinMode(2, OUTPUT); // Lüfter

  pinMode(9, OUTPUT); // LED blue
  pinMode(6, OUTPUT); // LED green
  pinMode(5, OUTPUT); // LED red
  
    stepper.setSpeed(100);

 
 // stepper.step(-400);
 // stepperValue = 600;
  
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  int value;
  int a,b,c;    
  
  
  
  
  
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
  
  // if there's any serial available, read it:
  if (Serial.available() > 0) {

  int cmd = Serial.read();
  
  if (cmd == 'S')
  {
      int a = Serial.parseInt();
      int b = Serial.parseInt();
      int c = Serial.parseInt();
      int d = Serial.parseInt();
      
      Serial.print(a);
      Serial.print(b);
      Serial.print(c);
      Serial.print(d);
      
      Stepper s(100,a,b,c,d);
      s.setSpeed(30);
      s.step(40);
      s.step(-40);
      
      Serial.print("\n");
      return;
  }  

  
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
      
    case 's':
      value = Serial.parseInt();
      Serial.print("stepper ");      
      Serial.print(value); 
      stepper.step(value);
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
}


