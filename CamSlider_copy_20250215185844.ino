// Sketch for CamSlider
// 06/04/2019 by RZtronics <raj.shinde004@gmail.com>
// Project homepage: http://RZtronics.com/
///////////////////////////////////////////////////////////////////////////////////////
//Terms of use
///////////////////////////////////////////////////////////////////////////////////////
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include "bitmap.h"

#define limitSwitch 11
#define PinSW 2
#define PinCLK 3  
#define PinDT 8
// I2C OLED display pins for ESP32 (GPIO 21 and GPIO 22)
#define OLED_RESET    -1  // Reset pin (or -1 if not used)
Adafruit_SSD1306 display(198, 64, &Wire, OLED_RESET);


AccelStepper stepper2(1, 7, 6); // (Type:driver, STEP, DIR)
AccelStepper stepper1(1, 5, 4);

MultiStepper StepperControl; 

long gotoposition[2]; 

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
volatile boolean rotationdirection;  


void Switch()  
{
 if(millis()-switch0>500)
 {
 flag=flag+1;
 }
 switch0=millis();
}

void Rotary()  
{
delay(75);
if (digitalRead(PinCLK))
rotationdirection= digitalRead(PinDT);
else
rotationdirection= !digitalRead(PinDT);
TurnDetected = true;
delay(75);
}

void setup() 
{
  Serial.begin(115200);
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  
  pinMode(limitSwitch, INPUT_PULLUP);
  pinMode(PinSW,INPUT_PULLUP); 
  pinMode(PinCLK,INPUT_PULLUP); 
  pinMode(PinDT,INPUT_PULLUP);  
  pinMode(12,INPUT_PULLUP);  

  // Set up I2C for ESP32
  Wire.begin(21, 22);  // SDA -> GPIO 21, SCL -> GPIO 22

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();
  
  // Create instances for MultiStepper - Adding the 2 steppers to the StepperControl instance for multi control
  StepperControl.addStepper(stepper1);
  StepperControl.addStepper(stepper2);

  attachInterrupt (digitalPinToInterrupt(2),Switch,RISING); // SW connected to D2
  attachInterrupt (digitalPinToInterrupt(3),Rotary,RISING); // CLK Connected to D3
   
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
  
              if(digitalRead(12)==0){
                display.clearDisplay();
                delay(2000);
                ulimit= stepper1.currentPosition();
              }
              
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
