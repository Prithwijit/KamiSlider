#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include "bitmap.h"  // Assuming this is your custom bitmap header file

// Define the OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define limitSwitch 12     // Use GPIO 12 for the limit switch
#define ulimitSwitch 27     // Use GPIO 12 for the limit switch
#define PinSW 13           // Use GPIO 13 for the rotary encoder switch
#define PinCLK 14          // Use GPIO 14 for the rotary encoder CLK
#define PinDT 15           // Use GPIO 15 for the rotary encoder DT
#define LMswitch 26

// Create an instance of the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  // -1 for default reset pin

// Define stepper motor connections and motor interface type
AccelStepper stepper2(1, 5, 18); // (Type: driver, STEP, DIR) - DIR moved to GPIO 18
AccelStepper stepper1(1, 19, 23); // (Type: driver, STEP, DIR) - DIR moved to GPIO 23

MultiStepper StepperControl; 

long gotoposition[2];  // Array to hold target positions for each stepper

volatile long XInPoint=0;
volatile long YInPoint=0;
volatile long XOutPoint=0;
volatile long YOutPoint=0;  
volatile long totaldistance=0;
volatile long ulimit=61000;
int flag=0; 
int temp=0;
int i,j;
unsigned long switch0=0;
unsigned long rotary0=0;
float setspeed=200;
float motorspeed;
float timeinsec;
float timeinmins;
volatile boolean TurnDetected;  
volatile boolean LMDetected= false;  
volatile boolean rotationdirection;  
unsigned long lastInterruptTime = 0; // Debounce timer
unsigned long debounceDelay = 500; 
volatile boolean move= false;

void Switch()  
{
 if(millis()-switch0>500)
 {
 flag=flag+1;
 }
 switch0=millis();
}
void UL()  
{
 if(millis()-switch0>500)
 {
 ulimit=stepper1.currentPosition();
 }
 switch0=millis();
}

void Lmove() {
  // Only react to switch press after debounce delay
  if (millis() - switch0 > debounceDelay) {
    LMDetected=!LMDetected;
    switch0 = millis();        // Update the debounce timer
  }
  if(LMDetected){
    move=true;
  }else{
    move = false;
  }
}
void Rotary() {
  unsigned long interruptTime = millis();  // Get the current time

  // Only handle the interrupt if enough time has passed since the last one
  if (interruptTime - lastInterruptTime > debounceDelay) {
    lastInterruptTime = interruptTime;  // Update the last interrupt time

    // Read the CLK and DT pins to determine the rotation direction
    if (digitalRead(PinCLK)) {
      rotationdirection = digitalRead(PinDT);
    } else {
      rotationdirection = !digitalRead(PinDT);
    }

    TurnDetected = true;  // Flag that a turn was detected
  }
}
void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);

  Serial.print("Begin setup");
  // Start communication with the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // I2C address: 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever if the display is not found
  }
  
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  
  pinMode(limitSwitch, INPUT_PULLUP);
  pinMode(PinSW,INPUT_PULLUP); 
  pinMode(PinCLK,INPUT_PULLUP); 
  pinMode(PinDT,INPUT_PULLUP);  
  pinMode(ulimitSwitch,INPUT_PULLUP);  
  pinMode(LMswitch,INPUT_PULLUP);  


  attachInterrupt (digitalPinToInterrupt(PinSW),Switch,RISING); // SW connected to D2
  attachInterrupt (digitalPinToInterrupt(PinCLK),Rotary,RISING); // CLK Connected to D3
  attachInterrupt (digitalPinToInterrupt(ulimitSwitch),UL,RISING); // CLK Connected to D3
  attachInterrupt (digitalPinToInterrupt(LMswitch),Lmove,CHANGE); // CLK Connected to D3
   
  // Clear the display
  display.clearDisplay();
  
  // Add the stepper motors to the MultiStepper control object
  StepperControl.addStepper(stepper1);
  StepperControl.addStepper(stepper2);
  display.setTextColor(SSD1306_WHITE);  // White text
 
  // display Boot logo
  display.drawBitmap(0, 0, Dheu, 128, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();  

  // display Boot logo
  display.drawBitmap(0, 0, CamSlider, 128, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();  

  display.setTextSize(1.5);
  display.setTextColor(WHITE);
  display.setCursor(25,0);
  display.print("Developed By:");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,8);
  display.print("Aakash, ");
  display.println("Prithwijit, ");
  display.println("Polo");
  display.display();
  delay(2000);
  display.clearDisplay();  
  
  Home(); // Move the slider to the initial position - homing
  
}

void Home()
{
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  if(digitalRead(limitSwitch)==1)
  {
    display.drawBitmap(0, 0, Homing, 128,64, 1);
    display.display();
  }
  
  while (digitalRead(limitSwitch)== 1) 
  {
    stepper1.setSpeed(-3000);
    stepper1.runSpeed();
    
 }
  delay(20);
  stepper1.setCurrentPosition(0);
    stepper1.moveTo(200);
    while(stepper1.distanceToGo() != 0)
    {
      stepper1.setSpeed(3000);
      stepper1.runSpeed();
    }
    stepper1.setCurrentPosition(0);
    display.clearDisplay();
}

void SetSpeed()
{
  display.clearDisplay();
  while( flag==6)
  {
  if (TurnDetected)  
  {
        TurnDetected = false;  // do NOT repeat IF loop until new rotation detected
        if (rotationdirection) 
        { 
         setspeed = setspeed + 30;
        }
        if (!rotationdirection) 
        { 
         setspeed = setspeed - 30;        
         if (setspeed < 0) 
        { 
         setspeed = 0;
        }
        }

          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.print("  Speed");
          //display.setCursor(64,0); changed
        //  display.print("|");
          motorspeed=setspeed/80;
          display.setTextSize(1);
          display.setCursor(10,16);
          display.print(" ");
          display.print(motorspeed);
          display.setCursor(10,24);
          display.print(" mm/s");   
          totaldistance=XOutPoint-XInPoint;
          if(totaldistance<0)
            {
              totaldistance=totaldistance*(-1);
            }
            else
            {
              
            }
          timeinsec=(totaldistance/setspeed);
          timeinmins=timeinsec/60;
          display.setTextSize(1);
          display.setCursor(64,0);
          display.print("  Time");
          display.setTextSize(1);
          display.setCursor(72,16);
          if(timeinmins>1)
          {
          display.print(" ");
          display.print(timeinmins);
          display.setCursor(72,24);
          display.print("  min");
          }
          else
          { 
          display.print(" ");
          display.print(timeinsec);
          display.setCursor(72,24);
          display.print("  sec");
          }
          display.display();
  }
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.print("  Speed");
        //  display.setCursor(64,0);
          //display.print("|");
          motorspeed=setspeed/80;
          
          display.setTextSize(1);
          display.setCursor(10,16);
          display.print(" ");
          display.print(motorspeed);
          display.setCursor(10,24);
          display.print(" mm/s");   
          totaldistance=XOutPoint-XInPoint;
           if(totaldistance<0)
            {
              totaldistance=totaldistance*(-1);
            }
            else
            {
              
            }
          timeinsec=(totaldistance/setspeed);
          timeinmins=timeinsec/60;
          display.setTextSize(1);
          display.setCursor(64,0);
          display.print("  Time");
          display.setTextSize(1);
          display.setCursor(72,16);
          if(timeinmins>1)
          {
          display.print(" ");
          display.print(timeinmins);
          display.setCursor(72,24);
          display.print("  min");
          }
          else
          { 
          display.print(" ");
          display.print(timeinsec);
          display.setCursor(72,24);
          display.print("  sec");
          }
          display.display();
  }
 
}

void stepperposition(int n,String msg)
{
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  if (TurnDetected)  
  {
        TurnDetected = false;  // do NOT repeat IF loop until new rotation detected
     if(n==1)
     {
        if (!rotationdirection) 
        { 
          if( stepper1.currentPosition()-500>0 )
          {
          stepper1.move(-500);
          while(stepper1.distanceToGo() != 0)
            {
              stepper1.setSpeed(-3000);
              stepper1.runSpeed();
            }
          }
            else
            {
                while (stepper1.currentPosition()!=0) 
                 {
                      stepper1.setSpeed(-3000);
                      stepper1.runSpeed();
                 }
            }
        }

        if (rotationdirection) 
        { 
          if( stepper1.currentPosition()+500<ulimit )
          {
          stepper1.move(500);
          while(stepper1.distanceToGo() != 0)
            {
              stepper1.setSpeed(3000);
              stepper1.runSpeed();
            }
          }
          else
          {
            while (stepper1.currentPosition()!= ulimit) 
             {
                  stepper1.setSpeed(3000);
                  stepper1.runSpeed();
    
             } 
          }
        }
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,16);
        display.println(stepper1.currentPosition());
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(45,0);
        display.println(msg);
        display.display(); 
     }
     if(n==2)
     {
       if (rotationdirection) 
       { 
         stepper2.move(-100);
         while(stepper2.distanceToGo() != 0)
           {
             stepper2.setSpeed(-3000);
             stepper2.runSpeed();
           }      
       }
        if (!rotationdirection) 
       { 
         stepper2.move(100);
         while(stepper2.distanceToGo() != 0)
           {
             stepper2.setSpeed(3000);
             stepper2.runSpeed();
           } 
       }
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,16);
        display.println(stepper2.currentPosition());
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(45,0);
        display.println(msg);
        display.display(); 
     }
     }
} 


void loop()
{ 
 if(move){
   while(LMDetected){
   stepper1.setSpeed(-3000);
   stepper1.runSpeed();
  }
}else{
  //Begin Setup
  if(flag==0)
  {
    display.clearDisplay();
    display.drawBitmap(0, 0, BeginSetup, 128, 64, 1);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(32,0);
    display.println("Begin Setup");
    display.display(); 
    delay(2000);
    display.clearDisplay();  

    

    setspeed=200;
  }
  
  //SetXin
  if(flag==1)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(45,0);
    display.println("Set X In");
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(stepper1.currentPosition());
    display.display(); 
    while(flag==1)
    {
    stepperposition(1,"Set X In"); 
    }
    XInPoint=stepper1.currentPosition();
  }
  //SetYin
  if(flag==2)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(45,0);
    display.println("Set Y In");
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(stepper2.currentPosition());
    display.display(); 
    while(flag==2)
    {
    stepperposition(2,"Set Y In");
    }
    //stepper2.setCurrentPosition(0);
    YInPoint=stepper2.currentPosition();
  }
  //SetXout
  if(flag==3)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(45,0);
    display.println("Set X Out");
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(stepper1.currentPosition());
    display.display(); 
    while(flag==3)
    {
    stepperposition(1,"Set X Out");
    Serial.println(stepper1.currentPosition());
    }
    XOutPoint=stepper1.currentPosition();
    
  }
  //SetYout
  if(flag==4)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(45,0);
    display.println("Set Y Out");
    display.setTextSize(2);
    display.setCursor(0,16);
    display.println(stepper2.currentPosition());
    display.display();
    while(flag==4)
    {
    stepperposition(2,"Set Y Out");
    }
    YOutPoint=stepper2.currentPosition();
    display.clearDisplay();
        
    // Go to IN position
    gotoposition[0]=XInPoint;
    gotoposition[1]=YInPoint;    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(24,8);
    display.println("Preview");
    display.display(); 
    stepper1.setMaxSpeed(3000);
    StepperControl.moveTo(gotoposition);
    StepperControl.runSpeedToPosition();
  }

  //Display Set Speed
  if(flag==5)
  {
    display.clearDisplay();
    display.setCursor(11,8);
    display.println("Set Speed");
    display.display();  
  }
  //Change Speed
  if(flag==6)
  {
    display.clearDisplay();
    SetSpeed();
  }
  //DisplayStart
  if(flag==7)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(35,8);
    display.println("Start");
    display.display();
  }
  //Start
  if(flag==8)
  { 
    display.clearDisplay();
    display.setCursor(24,8);
    display.println("Running");
    display.display();
    Serial.println(XInPoint);
    Serial.println(XOutPoint);
    Serial.println(YInPoint);
    Serial.println(YOutPoint);

    gotoposition[0]=XOutPoint;
    gotoposition[1]=YOutPoint;
    stepper1.setMaxSpeed(setspeed);
    StepperControl.moveTo(gotoposition);
    StepperControl.runSpeedToPosition();
    
    /*
    gotoposition[0]=XInPoint+1;
    gotoposition[1]=YInPoint+1;
    //while (gotoposition[0]<XOutPoint || gotoposition[1]<YOutPoint)
    while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0)  {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(24, 0);
      display.println("Running");

      // Calculate progress percentage
      int currentXPosition = stepper1.currentPosition();
      int currentYPosition = stepper2.currentPosition(); // Assuming stepper2 for Y axis
      int totalXDistance = XOutPoint - XInPoint;
      int totalYDistance = YOutPoint - YInPoint;
      int progressX = ((currentXPosition - XInPoint) * 100) / totalXDistance;
      int progressY = ((currentYPosition - YInPoint) * 100) / totalYDistance;
      int overallProgress = (progressX + progressY) / 2; // Average progress percentage

      // Display progress percentage
      display.setTextSize(1);
      display.setCursor(24, 16);
      //display.print("Progress: ");
      display.print(overallProgress+"%");
      //display.print("%");

      display.display();
      StepperControl.moveTo(gotoposition);
      StepperControl.runSpeedToPosition();
      gotoposition[0]=gotoposition[0]+1;
      gotoposition[1]=gotoposition[1]+1;
      }
      */
      /*
      // Initialize target positions once
      int stepXDistance = (XOutPoint - XInPoint) / 100;
      int stepYDistance = (YOutPoint - YInPoint) / 100;
      int targetXPosition = XInPoint + stepXDistance;
      int targetYPosition = YInPoint + stepYDistance;
      stepper1.setSpeed(setspeed);
      stepper2.setSpeed(setspeed); // Assuming stepper2 for Y axis
          // Move to the next target position
          gotoposition[0] = targetXPosition;
          gotoposition[1] = targetYPosition;
          StepperControl.moveTo(gotoposition);
        int xDistToMove=abs(XOutPoint - XInPoint);
        int yDistToMove=abs(YOutPoint - YInPoint);
      while ((stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0)&&(xDistToMove>0||yDistToMove>0)) {

          // Run the motors to the next target position
          StepperControl.runSpeedToPosition();
          xDistToMove=xDistToMove-abs(stepXDistance);
          yDistToMove=yDistToMove-abs(stepYDistance);
          // Calculate progress percentage
          int currentXPosition = stepper1.currentPosition();
          int currentYPosition = stepper2.currentPosition(); // Assuming stepper2 for Y axis
          int totalXDistance = XOutPoint - XInPoint;
          int totalYDistance = YOutPoint - YInPoint;
          int progressX = ((currentXPosition - XInPoint) * 100) / totalXDistance;
          int progressY = ((currentYPosition - YInPoint) * 100) / totalYDistance;
          int overallProgress = (progressX + progressY) / 2; // Average progress percentage

          // Display progress percentage
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(24, 0);
          display.println("Running");
          display.setTextSize(1);
          display.setCursor(24, 16);
          display.print("Progress: ");
          display.print(overallProgress);
          display.println("%");
          display.display();

          // Update target positions for the next step
          targetXPosition += stepXDistance;
          targetYPosition += stepYDistance;
          gotoposition[0] = targetXPosition;
          gotoposition[1] = targetYPosition;
          StepperControl.moveTo(gotoposition);
      }
    */
      
    flag=flag+1;
  }
  //Slide Finish
   if(flag==9)
   {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(30,8);
    display.println("Finish");
    display.display();
   }
  //Return to start
   if(flag==10)
   {
    display.clearDisplay();
    Home();
    flag=0;
   }  
}


}
