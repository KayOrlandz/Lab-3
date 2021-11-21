// HDL Kweens
// 11/19/2021

/**********************************************************************
Exercise the motor using the L293D chip.
Display time, fan speed, and rotation direction on LCD.
Implement fan speed and rotation direction control via remote control.
***********************************************************************/

#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>
#include "IRremote.h"

#define ENABLE 5
#define DIRA 3
#define DIRB 4
#define BUTTON 2
#define RECEIVER 13

DS3231 clock;
RTCDateTime dt;

/*-----( Declare objects )-----*/
IRrecv irrecv(RECEIVER);                    // Create instance of 'irrecv'
decode_results results;                     // Create instance of 'decode_results'
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

int sec;
int rpm = 0;
int dir = 0;
 
void setup() {
  //---set pin direction
  pinMode(ENABLE,OUTPUT);
  pinMode(DIRA,OUTPUT);
  pinMode(DIRB,OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  
  attachInterrupt(0,buttonPressed,FALLING); // Establish pin interrupt for push button
  
  digitalWrite(DIRA,HIGH);                  // Set fan rotation direction to clockwise (C)
  digitalWrite(DIRB,LOW);
  
  Serial.begin(9600);                       // Inititalize serial communication
  clock.begin();                            // Initialize DS3231
  irrecv.enableIRIn();                      // Start the receiver
  clock.setDateTime(__DATE__, __TIME__);    // Retreive current date and time from RTC
  lcd.begin(16, 2);                         // Set up the LCD's number of columns and rows

  // this section of code will enable one arduino timer interrupt
  // timer1 will interrupt at 1Hz
  
  cli();                    // stop interrupts

  //set timer1 interrupt at 1Hz
  
  TCCR1A = 0;               // set entire TCCR1A register to 0
  TCCR1B = 0;               // same for TCCR1B
  TCNT1  = 0;               // initialize counter value to 0
  
  // set compare match register for 1hz increments
  
  OCR1A = 15624;            // = (16*10^6) / (1*1024) - 1 (must be <65536)
  
  // turn on CTC mode
  
  TCCR1B |= (1 << WGM12);
  
  // Set CS12 and CS10 bits for 1024 prescaler
  
  TCCR1B |= (1 << CS12) | (1 << CS10);
    
  // enable timer compare interrupt
  
  TIMSK1 |= (1 << OCIE1A);

  sei();                    // allow interrupts
 
}

ISR(TIMER1_COMPA_vect){         // timer1 interrupt evaluates conditions every second
  // Display current time on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clock:  ");
  lcd.print(dt.hour);
  lcd.print(":");
  lcd.print(dt.minute);
  lcd.print(":");
  lcd.print(dt.second);
  lcd.setCursor(0, 1);
  
  // Display current fan speed on LCD
  lcd.print("Speed: ");
  if (rpm == 255){
    lcd.print("Full");
  }
  else if (rpm == 192){
    lcd.print("3/4");
  }
  else if (rpm == 128){
    lcd.print("1/2");
  }
  else{
    lcd.print("0");
  }
  
  // Update fan rotation direction and display it on LCD
  if (dir == 1){
    lcd.print("  CC");
  }
  else{
    lcd.print("  C");
  }
} // END ISR

void buttonPressed(){ // Pin interrupt occurs whenever button is pushed
  // Change rotation direction if button is pushed
  if (dir == 0){
    dir = 1;
    digitalWrite(DIRA,LOW);     // CC
    digitalWrite(DIRB,HIGH);
  }
  else{
    dir = 0;
    digitalWrite(DIRA,HIGH);    // C
    digitalWrite(DIRB,LOW);
  }
} // END buttonPressed

void translateIR(){   // Function triggers whenever IR receiver receives something from the remote
  switch(results.value){

    case 0xFFA25D: Serial.println("POWER");         // Power button toggles between 0 speed and full speed
                   if (rpm == 0){
                    rpm = 255;
                   }
                   else{
                    rpm = 0;
                   }
                   break;
                   
    case 0xFF22DD: Serial.println("FAST BACK");     // Fast back button changes fan direction to counterclockwise
                   dir = 1;
                   break;

    case 0xFFC23D: Serial.println("FAST FORWARD");  // Fast forward button changes fan direction to clockwise
                   dir = 0;
                   break;
                   
    case 0xFFE01F: Serial.println("DOWN");          // Down button decreases the fan speed
                   if (rpm == 255){
                    rpm = 192;
                   }
                   else if (rpm == 192){
                    rpm = 128;
                   }
                   else{
                    rpm = 0;
                   }
                   break;

    case 0xFF906F: Serial.println("UP");            // Up button increases the fan speed
                   if (rpm == 0){
                    rpm = 128;
                   }
                   else if (rpm == 128){
                    rpm = 192;
                   }
                   else{
                    rpm = 255;
                   }
                   break;
   
    default: 
      Serial.println(" other button   ");
  } // End Case
  delay(500);                     // Do not get immediate repeat
} // END translateIR

void loop() {

  dt = clock.getDateTime();       // Retrieve current date and time from RTC
  sec = dt.second;                // Variable used to look at just the seconds of the RTC

  analogWrite(ENABLE,rpm);        // Controls the fan speed

  if (sec == 0){                  // Turns fan on at the beginning of a new minute
    rpm = 255;
  }
  if (sec == 10){
    rpm = 192;
  }
  if (sec == 20){
    rpm = 128;
  }
  if (sec == 30){                 // Turns fan off after 30 seconds
    rpm = 0;
  }

  if (irrecv.decode(&results)){   // Have we received an IR signal?
    translateIR(); 
    irrecv.resume();              // Receive the next value
  }  
} // END loop
   
