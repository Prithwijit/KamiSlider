/*
CameraSlider - Motor Control
Description: This file contains all the functions we use to initialize, configure and control motors
*/
#include "Point.h"
#include "config_cameraslider.h"

// Internal state variables
sliderState_t sliderState = SLIDER_HOMING;
bool bmotorState = true;
bool bhomingComplete = false;
bool analogInput = false;
bool initialized = false;
bool percentageFlag = false;
bool displayPercentage=true;
bool softDelay=false;
int lastReading = LOW;

float fSliderPos = 0.0;
float fRotationPos = 0.0;

// Start and stop positions
float fStartPos_Slider = 0.0;
float fStartPos_Rotation = 0.0;

float fEndPos_Slider = 0.0;
float fEndPos_Rotation = 0.0;

float fSlidingSpeed = 0.0;
float fRotatingSpeed = 0.0;

uint32_t slideDurationSec = 1;

const int centerMin = 1860; // Minimum center value when the joystick is at rest
const int centerMax = 1920; // Maximum center value when the joystick is at rest
const int maxValue = 4095;  // Assuming your joystick goes up to 4095 (12-bit ADC?)
const int outputMin = -270; // Minimum output range
const int outputMax = 250;  // Maximum output range

unsigned long lastDisplayTime = 0;
unsigned long startTime=0;

String dispTime(unsigned long durationInSeconds) {
    unsigned long hours = durationInSeconds / 3600;
    unsigned long minutes = (durationInSeconds % 3600) / 60;
    unsigned long seconds = durationInSeconds % 60;

    String timeString = "";

    if (hours > 0) {
        timeString += String(hours) + "h ";
    }
    if (minutes > 0 || hours > 0) {
        timeString += String(minutes) + "m ";
    }
    timeString += String(seconds) + "s";

    return timeString;
}

void updateDisplay()
{
    if(flag!=1 && flag!=2 && flag!=3 && flag!=4 && flag!=8){
        display.fillRect(0, 56, 128, 8, SSD1306_BLACK);
        display.setTextSize(1);
        display.setCursor(0, 56);
        display.print(stepper_slide.getCurrentPositionInMillimeters());
        display.setCursor(64, 56);
        display.print( SliderConfig.Config.rotate_direction * (stepper_pan.getCurrentPositionInSteps() / SliderConfig.Config.pan_steps_per_degree));
        display.display();
    }
    if((sliderState==SLIDER_MOVING_TO_END || sliderState==SLIDER_WORKING)&& flag!=6 && displayPercentage){
        display.fillRect(64, 44, 64, 8, SSD1306_BLACK);
        display.setTextSize(1);
        display.setCursor(70, 44);
        float total=fStartPos_Slider-fEndPos_Slider;
        //Serial.println("total");
        //Serial.println(total);
        float moved=stepper_slide.getCurrentPositionInMillimeters()-fStartPos_Slider;
        //Serial.println("moved");
        //Serial.println(moved);
        float prog=(moved/total)*100;
        if(prog<0){
            prog=prog*-1;
        }
        if(prog>99){
            percentageFlag=true;
        }else if(prog<1){
            percentageFlag=false;
        }
        //Serial.println("prog");
        //Serial.println(prog);
        if(percentageFlag){
            prog=100-prog;
        }
        display.print(prog);
        display.print(" %");
        display.display();
    }
   
}
// Main motor control function.
// We call this function from within our main loop to
// execute different commands based on current state
// of the CameraSlider state machine. Values/states of
// this state machine can be updated asynchronously (HTTP)
// or trough interrupts.
void CameraSlider_tick()
{
    // Serial.println("lastADButtonState");
    // Serial.println(lastADButtonState);
    // SwitchAD();
    if(updateULflag){
        //Serial.println("updating");
        updateULflag=false;
        CameraSlider_UpdateRailLength(getSliderPos());
    }
    int reading = digitalRead(PIN_A_D);
    // Check if PIN_A_D is HIGH and it wasn't HIGH in the previous loop iteration
    if (reading == HIGH && !initialized)
    {
        stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
        stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);

        stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm);
        stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed * SliderConfig.Config.slide_steps_per_mm);
        stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.slide_steps_per_mm);

        // Configure pan motor
        // -- Revolution in steps
        stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.pan_steps_per_degree);
        stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
        initialized = true; // Mark as initialized
    }

    // Check if PIN_A_D is LOW to reset the initialized flag
    if (reading == LOW && lastReading == HIGH)
    {
        initialized = false; // Reset the flag when PIN_A_D goes LOW
    }

    lastReading = reading; // Store the current reading for the next iteration
    if (reading == HIGH && (sliderState!=SLIDER_MOVING_TO_START&&sliderState!=SLIDER_MOVING_TO_END&&sliderState!=SLIDER_WORKING))
    {

        int joystickXValue = analogRead(PIN_JOY_X);
        // Serial.print("a");
        // Serial.println(joystickXValue);
        //  Calculate the deviation from the center (taking into account the range of the joystick)
        // int centerValue = (centerMin + centerMax) / 2;  // Averaging the min and max center values
        // int deviation = joystickXValue - centerValue;  // Deviation from the center
        //  Map the deviation to the range of -3000 to 3000
        int mappedValue = map(joystickXValue, 0, maxValue, outputMin, outputMax);
        mappedValue = mappedValue + 30;
        
        
        //  Serial.println(mappedValue);
        if (joystickXValue < 1840 && mappedValue < -10 &&  (flag<5) && (sliderState!=SLIDER_MOVING_TO_START&&sliderState!=SLIDER_MOVING_TO_END&&sliderState!=SLIDER_WORKING))
        {
            if(digitalRead(PIN_JOY_SW)==HIGH){
                mappedValue=mappedValue/10;
                   //Serial.println(mappedValue);
               }
            // if (sliderState == SLIDER_IDLE || sliderState==SLIDER_READY)
            {
                /*//Serial.println("moving L");
                stepper1.setSpeed(mappedValue);
                stepper1.runSpeed();
               */
                // stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm); // Set this value based on your stepper's specifications
                // stepper_slide.setSpeedInStepsPerSecond(mappedValue);
                // stepper_slide.processMovement();
                /*stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
                stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);
        */
                stepper_slide.moveRelativeInSteps(mappedValue);
                // stepper_slide.processMovement();
            }
        }
        else if (joystickXValue > 1940 && mappedValue > 10  &&  (flag<5) && (sliderState!=SLIDER_MOVING_TO_START&&sliderState!=SLIDER_MOVING_TO_END&&sliderState!=SLIDER_WORKING))
        {
            if(digitalRead(PIN_JOY_SW)==HIGH){
                mappedValue=mappedValue/10;
                  // Serial.println(mappedValue);
               }
            // if (sliderState == SLIDER_IDLE || sliderState==SLIDER_READY)
            {

                /*//Serial.println("moving R");
                stepper1.setSpeed(mappedValue);
                stepper1.runSpeed();
                */
                // stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm); // Set this value based on your stepper's specifications
                // stepper_slide.setSpeedInStepsPerSecond(mappedValue);
                // stepper_slide.processMovement();
              /*  stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
                stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);*/
        
                stepper_slide.moveRelativeInSteps(mappedValue);
                // stepper_slide.processMovement();
            }
        }
        else
        {

            // Begin Setup
            if (flag == 0)
            {
                display.fillRect(0, 8, 128, 48, SSD1306_BLACK);
                display.drawBitmap(0, 8, BeginSetup, 128, 48, 1);
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(0, 0);
                display.print(F("Connect to Kamislider     192.168.4.1"));
                display.display();
                // delay(2000);
                //display.clearDisplay();
            }

            // SetXin
            if (flag == 1)
            {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(45, 0);
                display.println("Set X In");
                display.setTextSize(2);
                display.setCursor(0, 16);
                display.println(stepper_slide.getCurrentPositionInMillimeters());
                display.display();
                while (flag == 1 && digitalRead(PIN_A_D) == HIGH)
                {
                    stepperposition(1, "Set X In");
                }
                fStartPos_Slider = stepper_slide.getCurrentPositionInMillimeters();
            }
            // SetYin
            if (flag == 2)
            {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(45, 0);
                display.println("Set Y In");
                display.setTextSize(2);
                display.setCursor(0, 16);
                display.println(stepper_pan.getCurrentPositionInMillimeters());
                display.display();
                while (flag == 2 && digitalRead(PIN_A_D) == HIGH)
                {
                    stepperposition(2, "Set Y In");
                }
                // stepper2.setCurrentPosition(0);
                fStartPos_Rotation = stepper_pan.getCurrentPositionInMillimeters();
            }
            // SetXout
            if (flag == 3)
            {
                stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
                stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);
        
                stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm);
                stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed * SliderConfig.Config.slide_steps_per_mm);
                stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.slide_steps_per_mm);
        
                // Configure pan motor
                // -- Revolution in steps
                stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.pan_steps_per_degree);
                stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(45, 0);
                display.println("Set X Out");
                display.setTextSize(2);
                display.setCursor(0, 16);
                display.println(stepper_slide.getCurrentPositionInMillimeters());
                display.display();
                while (flag == 3 && digitalRead(PIN_A_D) == HIGH)
                {
                    stepperposition(1, "Set X Out");
                    //Serial.println(stepper_slide.getCurrentPositionInMillimeters());
                }
                fEndPos_Slider = stepper_slide.getCurrentPositionInMillimeters();
            }
            // SetYout
            if (flag == 4)
            {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(45, 0);
                display.println("Set Y Out");
                display.setTextSize(2);
                display.setCursor(0, 16);
                display.println(stepper_pan.getCurrentPositionInMillimeters());
                display.display();
                while (flag == 4 && digitalRead(PIN_A_D) == HIGH)
                {
                    stepperposition(2, "Set Y Out");
                }
                fEndPos_Rotation = stepper_pan.getCurrentPositionInMillimeters();
                display.clearDisplay();
                //sliderState = SLIDER_MOVING_TO_START;
                /*
               // Go to IN position
               gotoposition[0]=fStartPos_Slider;
               gotoposition[1]=fStartPos_Rotation;
               display.clearDisplay();
               display.setTextSize(2);
               display.setCursor(24,8);
               display.println("Preview");
               display.display();
               stepper1.setMaxSpeed(3000);
               StepperControl.moveTo(gotoposition);
               StepperControl.runSpeedToPosition();*/
            }
            if(flag==5){
                CameraSlider_MoveToStart(SliderConfig.Config.default_slider_speed,SliderConfig.Config.default_slider_accel,SliderConfig.Config.default_rotate_speed,SliderConfig.Config.default_rotate_accel);
                flag++;
            }
            // Display Set Speed
            if (flag == 6)
            {
                //display.clearDisplay();
                
                display.fillRect(0, 0, 128, 56, SSD1306_BLACK);
                display.setTextSize(2);
                display.setCursor(24, 15);
                display.println("Preview");
                display.setTextSize(1);
                display.setCursor(10, 35);
                display.println("Returning to Start");
                display.display();
            }
            // Display Set Speed
            if (flag == 7)
            {
                //display.clearDisplay();
                
                display.fillRect(0, 0, 128, 56, SSD1306_BLACK);
                display.setTextSize(2);
                display.setCursor(11, 24);
                display.println("Set Speed");
                display.display();
            }
            // Change Speed
            if (flag == 8)
            {
               // display.clearDisplay();
                SetSpeed();
            }
            // DisplayStart
            if (flag == 9)
            {
                display.fillRect(0, 0, 128, 56, SSD1306_BLACK);
                display.setTextSize(1);
                display.setCursor(40, 0);
                display.println("Start");
                display.setCursor(0,16);      
                display.print(fStartPos_Slider);  
                display.setCursor(64,16);      
                display.print(fEndPos_Slider);  
                display.setCursor(0,28);
                display.print(fStartPos_Rotation); 
                display.setCursor(64,28);
                display.print(fEndPos_Rotation); 
                display.setCursor(0,44);
                display.print(dispTime(slideDurationSec)); 
                display.display();
            }
            if(flag==10){
                stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
                stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);
                stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm);
                stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed * SliderConfig.Config.slide_steps_per_mm);
                stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.slide_steps_per_mm);

                // Configure pan motor
                // -- Revolution in steps
                stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.pan_steps_per_degree);
                stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
                
                CameraSlider_StartMotion();
                flag++;
            }            
        }

    } // else{

    // Serial.print("d");
    // Serial.println(sliderState);
    /*   if ((millis() - lastDisplayTime) > 500)
       {
           display.fillRect(0, 48, 128, 8, SSD1306_BLACK);
           display.setCursor(0, 48);
           display.print(stepper_slide.getCurrentPositionInMillimeters());
           display.setCursor(64, 48);
           display.print(stepper_pan.getCurrentPositionInMillimeters());
           display.display();
           lastDisplayTime = millis();
       }
*/
    switch (sliderState)
    {
    case SLIDER_MOTORS_OFF:
        break;

    case SLIDER_IDLE:
        break;

    case SLIDER_MOVING_TO_START:
        // Serial.println("SLIDER_MOVING_TO_START");
        //  Moving to start
        if ((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
        {
            stepper_slide.processMovement();
            stepper_pan.processMovement();
        }
        // Move complete
        else
        {
            startTime= millis();
            if (slideDurationSec <= 0)
            {
                slideDurationSec = 1;
            }

            // Calculate speed
            if (fEndPos_Slider >= fStartPos_Slider)
            {
                fSlidingSpeed = (fEndPos_Slider - fStartPos_Slider) / slideDurationSec;
            }
            else
            {
                fSlidingSpeed = (fStartPos_Slider - fEndPos_Slider) / slideDurationSec;
            }

            if (fEndPos_Rotation >= fStartPos_Rotation)
            {
                fRotatingSpeed = (fEndPos_Rotation - fStartPos_Rotation) / (slideDurationSec);
            }
            else
            {
                fRotatingSpeed = (fStartPos_Rotation - fEndPos_Rotation) / (slideDurationSec);
            }
            softDelay=true;
            Serial.println("fSlidingSpeed");
            Serial.println(fSlidingSpeed);
            
            // Configure slider
            stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed);
            stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);
            stepper_slide.setTargetPositionInMillimeters(fEndPos_Slider);

            // Configure pan
            stepper_pan.setSpeedInStepsPerSecond(fRotatingSpeed);
            stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
            stepper_pan.setTargetPositionInSteps(fEndPos_Rotation);

            CameraSlider_SetState(SLIDER_MOVING_TO_END);
        }
        break;

    case SLIDER_MOVING_TO_END:
        // Moving to end position
        // Serial.println("SLIDER_MOVING_TO_END");
        if ((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
        {
            if(softDelay){
                float total=fStartPos_Slider-fEndPos_Slider;
                //Serial.println("total");
                //Serial.println(total);
                float moved=stepper_slide.getCurrentPositionInMillimeters()-fStartPos_Slider;
                //Serial.println("moved");
                //Serial.println(moved);
                float prog=(moved/total)*100;
                if(prog<0){
                    prog=prog*-1;
                }
                // Assume totalDuration is the total time for the operation in milliseconds
                float totalDuration = slideDurationSec * 1000; // Convert seconds to milliseconds
    
                // Calculate time passed since the operation started
                float timeProg = millis() - startTime;
    
                // Calculate progress as a percentage
                float timePercentageProgress = (timeProg / totalDuration) * 100;
                float progDiff=prog-timePercentageProgress;
                //Serial.println(progDiff);
                if(progDiff>0.001){

                    // Calculate the exact time difference in ms
                    float expectedTime = (prog / 100) * totalDuration;
                    float timeDiff=expectedTime-timeProg;
                    if(timeDiff<0){
                        timeDiff=timeDiff*-1;
                    }
                   // Serial.println(timeDiff);
                    delay(timeDiff);
                }
                if(progDiff<0.1 && SliderConfig.Config.exact_time==-1){
                    //Serial.println(progDiff);
                    stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed+(progDiff*-1*10));
                 }else{
                    stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed);
                 }
            }
            
            stepper_slide.processMovement();
            stepper_pan.processMovement();
        }
        // Move completed
        else
        {
            softDelay=false;
            CameraSlider_SetState(SLIDER_READY);
            flag=0;
        }
        break;

    case SLIDER_HOMING:
        CameraSlider_HomeSlidingRail();
        break;

    case SLIDER_READY:
        if (inMultiMotion)
        {
            multiStep = multiStep + 1;
            CameraSlider_StartMulti();
        }
        break;

    case SLIDER_WORKING:
        // Serial.println("SLIDER_WORKING");
        if ((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
        {
            if(softDelay){
                float total=fStartPos_Slider-fEndPos_Slider;
                //Serial.println("total");
                //Serial.println(total);
                float moved=stepper_slide.getCurrentPositionInMillimeters()-fStartPos_Slider;
                //Serial.println("moved");
                //Serial.println(moved);
                float prog=(moved/total)*100;
                if(prog<0){
                    prog=prog*-1;
                }
                // Assume totalDuration is the total time for the operation in milliseconds
                float totalDuration = slideDurationSec * 1000; // Convert seconds to milliseconds
    
                // Calculate time passed since the operation started
                float timeProg = millis() - startTime;
    
                // Calculate progress as a percentage
                float timePercentageProgress = (timeProg / totalDuration) * 100;
                float progDiff=prog-timePercentageProgress;
                //Serial.println(progDiff);
                if(progDiff>0.001){

                    // Calculate the exact time difference in ms
                    float expectedTime = (prog / 100) * totalDuration;
                    float timeDiff=expectedTime-timeProg;
                    if(timeDiff<0){
                        timeDiff=timeDiff*-1;
                    }
                  //  Serial.println(timeDiff);
                    delay(timeDiff);
                }
                if(progDiff<0.1 && SliderConfig.Config.exact_time==-1){
                    //Serial.println(progDiff);
                    stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed+(progDiff*-1*10));
                 }else{
                    stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed);
                 }
            }
            
           stepper_slide.processMovement();
            stepper_pan.processMovement();
        }else if(digitalRead(PIN_A_D)==HIGH){
            displayPercentage=true;
            CameraSlider_SetState(SLIDER_READY);
            softDelay=false;
        }else{
            softDelay=false;
        }
        break;

    default:
        return;
    }

    //}
}
void setupMotors()
{
    stepper1.setMaxSpeed(5000);
    stepper1.setSpeed(200);
    stepper2.setMaxSpeed(5000);
    stepper2.setSpeed(200);
    // Add the stepper motors to the MultiStepper control object
    StepperControl.addStepper(stepper1);
    StepperControl.addStepper(stepper2);
    // Connect to motors
    stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
    stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);

    // Configure sliding motor
    // Serial.println("Slider motor");
    //Serial.print("-- Steps per mm: ");
    // Serial.println(SliderConfig.Config.slide_steps_per_mm, DEC);

    stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm);
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed * SliderConfig.Config.slide_steps_per_mm);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.slide_steps_per_mm);

    // Configure pan motor
    // -- Revolution in steps
    stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
}

void SetSpeed()
{
    int timeinmins = 0;
    float setspeed = 0;
    float motorspeed = 0;
    float totaldistance = 0;
    // int reading = digitalRead(PIN_A_D);
    display.clearDisplay();
    while (flag == 8 && digitalRead(PIN_A_D) == HIGH) {
        if (TurnDetected) {
            TurnDetected = false; // do NOT repeat IF loop until new rotation detected
            if (rotationdirection) {
                slideDurationSec += 1; // Increase slide duration by 5 seconds (or other desired increment)
            }
            if (!rotationdirection) {
                slideDurationSec -= 1; // Decrease slide duration by 5 seconds (or other desired decrement)
                if (slideDurationSec < 1) { // Ensure slide duration doesn't go below 1 second
                    slideDurationSec = 1;
                }
            }
    
            totaldistance = fEndPos_Slider - fStartPos_Slider;
            if (totaldistance < 0) {
                totaldistance = totaldistance * (-1);
            }
    
            setspeed = totaldistance / slideDurationSec; // Calculate setspeed based on slideDurationSec
    
            display.clearDisplay();
            //display.setTextSize(2);
            display.setTextColor(WHITE);
            display.setCursor(0, 0);
            //display.print("  Speed");
            motorspeed = setspeed / 80; // Adjust this calculation as needed
            display.setTextSize(1);
            display.setCursor(10, 16);
            display.print(" ");
            display.print(motorspeed);
            display.setCursor(10, 24);
            display.print(" mm/s");
    
            display.setTextSize(1);
            display.setCursor(64, 0);
            display.print("  Time");
            display.setTextSize(1);
            display.setCursor(72, 16);
            timeinmins = slideDurationSec / 60;
            if (timeinmins > 0) {
                
                int recalculatedSecs=slideDurationSec-(timeinmins*60);
                display.print(" ");
                display.print(timeinmins);
                display.setCursor(72, 24);
                display.print("  min");
                display.setCursor(72, 32);
                display.print(recalculatedSecs);
                display.print("  sec");
            } else {
                display.print(" ");
                display.print(slideDurationSec);
                display.setCursor(72, 24);
                display.print("  sec");
            }
            display.display();
        }
    
        // Ensure the display is updated with the latest values
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("  Speed");
        motorspeed = setspeed / 80;
    
        display.setTextSize(1);
        display.setCursor(10, 16);
        display.print(" ");
        display.print(motorspeed);
        display.setCursor(10, 24);
        display.print(" mm/s");
    
        totaldistance = fEndPos_Slider - fStartPos_Slider;
        if (totaldistance < 0) {
            totaldistance = totaldistance * (-1);
        }
    
        setspeed = totaldistance / slideDurationSec; // Calculate setspeed based on slideDurationSec
    
        display.setTextSize(1);
        display.setCursor(64, 0);
        display.print("  Time");
        display.setTextSize(1);
        display.setCursor(72, 16);
        timeinmins = slideDurationSec / 60;
        
        if (timeinmins > 0) {
                
            int recalculatedSecs=slideDurationSec-(timeinmins*60);
            display.print(" ");
            display.print(timeinmins);
            display.setCursor(72, 24);
            display.print("  min");
            display.setCursor(72, 32);
            display.print(recalculatedSecs);
            display.print("  sec");
        }  else {
            display.print(" ");
            display.print(slideDurationSec);
            display.setCursor(72, 24);
            display.print("  sec");
        }
        display.display();
    }
    
}

void stepperposition(int n, String msg)
{
    /* stepper1.setMaxSpeed(3000);
     stepper1.setSpeed(200);
     stepper2.setMaxSpeed(3000);
     stepper2.setSpeed(200);*/
    int joystickXValue = analogRead(PIN_JOY_X);
    // Serial.print("a");
    // Serial.println(joystickXValue);
    //  Calculate the deviation from the center (taking into account the range of the joystick)
    // int centerValue = (centerMin + centerMax) / 2;  // Averaging the min and max center values
    // int deviation = joystickXValue - centerValue;  // Deviation from the center
    //  Map the deviation to the range of -3000 to 3000
    int mappedValue = map(joystickXValue, 0, maxValue, outputMin, outputMax);
    mappedValue = mappedValue + 30;
    
    //  Serial.println(mappedValue);
    if (joystickXValue < 1840 && mappedValue < -10 && n == 1)
    {
        if(digitalRead(PIN_JOY_SW)==HIGH){
            mappedValue=mappedValue/10;
              // Serial.println(mappedValue);
           }
        // if (sliderState == SLIDER_IDLE || sliderState==SLIDER_READY)
        {
            /*//Serial.println("moving L");
            stepper1.setSpeed(mappedValue);
            stepper1.runSpeed();
           */
            // stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm); // Set this value based on your stepper's specifications
            // stepper_slide.setSpeedInStepsPerSecond(mappedValue);
            // stepper_slide.processMovement();
            stepper_slide.moveRelativeInSteps(mappedValue);
            // stepper_slide.processMovement();
        }
    }
    else if (joystickXValue > 1940 && mappedValue > 10 && n == 1)
    {
        if(digitalRead(PIN_JOY_SW)==HIGH){
            mappedValue=mappedValue/10;
              // Serial.println(mappedValue);
           }
        // if (sliderState == SLIDER_IDLE || sliderState==SLIDER_READY)
        {

            /*//Serial.println("moving R");
            stepper1.setSpeed(mappedValue);
            stepper1.runSpeed();
            */
            // stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm); // Set this value based on your stepper's specifications
            // stepper_slide.setSpeedInStepsPerSecond(mappedValue);
            // stepper_slide.processMovement();
            stepper_slide.moveRelativeInSteps(mappedValue);
            // stepper_slide.processMovement();
        }
    }
    if (n == 1)
    {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 16);
        display.println(stepper_slide.getCurrentPositionInMillimeters());
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(45, 0);
        display.println(msg);
        display.display();
    }
    else if (n == 2)
    {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 16);
        display.println(stepper_pan.getCurrentPositionInMillimeters());
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(45, 0);
        display.println(msg);
        display.display();
    }
    if (TurnDetected)
    {
        TurnDetected = false; // do NOT repeat IF loop until new rotation detected
        if (n == 1)
        {
            if (!rotationdirection)
            {
                // if( stepper_slide.getCurrentPositionInMillimeters()-500>0 )
                //{
                stepper_slide.moveRelativeInSteps(-100);
                /*while(stepper1.distanceToGo() != 0)
                  {
                    stepper1.setSpeed(-3000);
                    stepper1.runSpeed();
                  }*/
                //}
            }

            if (rotationdirection)
            {
                // if( stepper_slide.getCurrentPositionInMillimeters()+500<SliderConfig.Config.rail_length )
                //{
                stepper_slide.moveRelativeInSteps(100);
                /*while(stepper1.distanceToGo() != 0)
                  {
                    stepper1.setSpeed(3000);
                    stepper1.runSpeed();
                  }*/
                //}
            }
            /* display.clearDisplay();
             display.setTextSize(2);
             display.setCursor(0,16);
             display.println(stepper_slide.getCurrentPositionInMillimeters());
             display.setTextSize(1);
             display.setTextColor(WHITE);
             display.setCursor(45,0);
             display.println(msg);
             display.display(); */
        }
        if (n == 2)
        {
            if (rotationdirection)
            {
                stepper_pan.moveRelativeInSteps(-100);
            }
            if (!rotationdirection)
            {
                stepper_pan.moveRelativeInSteps(100);
            }
            /*display.clearDisplay();
            display.setTextSize(2);
            display.setCursor(0,16);
            display.println(stepper_pan.getCurrentPositionInMillimeters());
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(45,0);
            display.println(msg);
            display.display(); */
        }
    }
}

void CameraSlider_MoveToPositionRelative(float xPos, float xSpeed, float xAccel, float rAngle, float rSpeed, float rAccel)
{
    displayPercentage=false;
    // Invert slider or pan motor if necessary
    xPos = SliderConfig.Config.slider_direction * xPos;
    rAngle = SliderConfig.Config.rotate_direction * rAngle;

    // Setup slider
    stepper_slide.setTargetPositionInMillimeters(xPos);
    stepper_slide.setSpeedInMillimetersPerSecond(xSpeed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(xAccel);

    // Setup pan
    stepper_pan.setTargetPositionRelativeInSteps(rAngle * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setSpeedInStepsPerSecond(rSpeed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(rAccel * SliderConfig.Config.pan_steps_per_degree);
    // Clear the display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Move To Position (R)"));
    display.setCursor(0, 16);
    display.print(stepper_slide.getCurrentPositionInMillimeters());
    display.setCursor(0, 28);
    display.print(stepper_pan.getCurrentPositionInSteps()/SliderConfig.Config.pan_steps_per_degree);
    display.setCursor(64, 16);
    display.print(xPos);
    display.setCursor(64, 28);
    display.print(rAngle);

    display.display(); // Refresh the display to show the message
    // Updatestate machine
    sliderState = SLIDER_WORKING;
}

void CameraSlider_MoveToPositionAbsolute(float xPos, float xSpeed, float xAccel, float rSteps, float rSpeed, float rAccel)
{
    displayPercentage=false;
    // Invert slider or pan motor if necessary
    xPos = SliderConfig.Config.slider_direction * xPos;
    rSteps = SliderConfig.Config.rotate_direction * rSteps;

    // Setup slider
    stepper_slide.setTargetPositionInMillimeters(xPos);
    stepper_slide.setSpeedInMillimetersPerSecond(xSpeed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(xAccel);

    // Setup pan
    stepper_pan.setTargetPositionInSteps(rSteps);
    stepper_pan.setSpeedInStepsPerSecond(rSpeed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(rAccel * SliderConfig.Config.pan_steps_per_degree);
    // Clear the display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Move To Position (A)"));
    display.setCursor(0, 16);
    display.print(stepper_slide.getCurrentPositionInMillimeters());
    display.setCursor(0, 28);
    display.print( stepper_pan.getCurrentPositionInSteps());
    display.setCursor(64, 16);
    display.print(xPos);
    display.setCursor(64, 28);
    display.print(rSteps);

    display.display(); // Refresh the display to show the message
    // Updatestate machine
    sliderState = SLIDER_WORKING;
}

void CameraSlider_MoveToStart(float xSpeed, float xAccel, float rSpeed, float rAccel)
{
    // Clear the display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Move To Start"));
    display.setCursor(64, 16);
    display.print(fStartPos_Slider);
    display.setCursor(64, 28);
    display.print(fStartPos_Rotation);

    display.display(); // Refresh the display to show the message
    CameraSlider_MoveToPositionAbsolute(fStartPos_Slider, xSpeed, xAccel, fStartPos_Rotation, rSpeed, rAccel);
}

void CameraSlider_MoveToEnd(float xSpeed, float xAccel, float rSpeed, float rAccel)
{
    // Clear the display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Move To End"));
    display.setCursor(64, 16);
    display.print(fEndPos_Slider);
    display.setCursor(64, 28);
    display.print(fEndPos_Rotation);

    display.display(); // Refresh the display to show the message
    CameraSlider_MoveToPositionAbsolute(fEndPos_Slider, xSpeed, xAccel, fEndPos_Rotation, rSpeed, rAccel);
}

void CameraSlider_HomeSlidingRail(void)
{
    // Clear the display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Connect to Kamislider     192.168.4.1"));
    display.drawBitmap(0, 0, Homing, 128, 64, 1);
    display.display();
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);

    // Serial.println("Homing linear rail");
    //Serial.print("Dir: ");
    // Serial.println(SliderConfig.Config.homing_direction, DEC);
    //Serial.print("EndSW: ");
    // Serial.println(PIN_END_SWICH_X, DEC);
    if (stepper_slide.moveToHomeInMillimeters(SliderConfig.Config.homing_direction, SliderConfig.Config.homing_speed_slide, SliderConfig.Config.rail_length, PIN_END_SWICH_X) != true)
    {
        //
        // this code is executed only if homing fails because it has moved farther
        // than maxHomingDistanceInMM and never finds the limit switch, blink the
        // LED fast forever indicating a problem
        //
        // Serial.println("Failed homing!!!");
        while (true)
        {
            digitalWrite(PIN_LED, HIGH);
            delay(200);
            digitalWrite(PIN_LED, LOW);
            delay(200);
        }
    }
    sliderState = SLIDER_READY;
    bhomingComplete = true;

    // Not necessary as it should already be done by above function
    stepper_slide.setCurrentPositionInMillimeters(0);
    // Serial.println("done.");
    display.fillRect(0, 8, 128, 48, SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Connect to Kamislider     192.168.4.1"));
    display.drawBitmap(0, 8, BeginSetup, 128, 48, 1);
    display.display();
}

bool CameraSlider_getMotorState()
{
    return bmotorState;
}

bool CameraSlider_SetState(sliderState_t newState)
{
    if (bmotorState == false)
    {
        return false;
    }

    if (newState <= SLIDER_FIRST)
    {
        // Serial.println("Invalid state requested! (state <= SLIDER_FIRST)");
        return false;
    }
    else if (newState >= SLIDER_LAST)
    {
        // Serial.println("Invalid state requested! (state >= SLIDER_LAST)");
        return false;
    }
    else
    {
        sliderState = newState;
        return true;
    }

    return false;
}

void CameraSlider_EnableMotors(bool enable)
{
    if (enable == true)
    {
        digitalWrite(PIN_MTR_nEN, LOW);
        bmotorState = true;
        CameraSlider_SetState(SLIDER_IDLE);
    }
    else
    {
        digitalWrite(PIN_MTR_nEN, HIGH);
        bmotorState = false;
        CameraSlider_SetState(SLIDER_MOTORS_OFF);
        bhomingComplete = false;
    }
}

bool CameraSlider_FormatJSON_CameraSliderStatus(char *buff, int size)
{
    int len;
    len = snprintf(buff, size, "{\"homed\":%d,\"motors\":%d,\"state\":%d,\"posX\":%f,\"posZ\":%f,\"spX\":%f,\"spZ\":%f,\"epX\":%f,\"epZ\":%f}",
                   bhomingComplete,
                   bmotorState,
                   sliderState,
                   getSliderPos(),
                   getRotationPos(true),
                   fStartPos_Slider,
                   SliderConfig.Config.rotate_direction * (fStartPos_Rotation / SliderConfig.Config.pan_steps_per_degree),
                   fEndPos_Slider,
                   SliderConfig.Config.rotate_direction * (fEndPos_Rotation / SliderConfig.Config.pan_steps_per_degree)

    );

    if (len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CameraSlider_FormatJSON_MultiSliderStatus(char *buff, int size, Point *globalArray, int currentArraySize)
{
    int len = 0;
    len += snprintf(buff + len, size - len, "[");

    for (int i = 0; i < currentArraySize; i++)
    {
        len += snprintf(buff + len, size - len, "{\"x\":%d,\"y\":%d,\"z\":%d}",
                        globalArray[i].x,
                        globalArray[i].y,
                        globalArray[i].z);
        if (i < currentArraySize - 1)
        {
            len += snprintf(buff + len, size - len, ",");
        }
    }

    len += snprintf(buff + len, size - len, "]");

    return (len > 0 && len < size);
}

bool CameraSlider_FormatJSON_CameraConfig(char *buff, int size)
{
    int len;
    len = snprintf(buff, size, "{\"rail_length\":%d,\"dir_homing\":%d,\"dir_slider\":%d,\"dir_rotation\":%d,\"exact_time\":%d,\"slider_steps_per_mm\":%d,\"rotation_steps_per_deg\":%d,\"homing_speed_slider\":%d,\"homing_speed_rotation\":%d}",
                   SliderConfig.Config.rail_length,
                   SliderConfig.Config.homing_direction,
                   SliderConfig.Config.slider_direction,
                   SliderConfig.Config.rotate_direction,
                   SliderConfig.Config.exact_time,
                   SliderConfig.Config.slide_steps_per_mm,
                   SliderConfig.Config.pan_steps_per_degree,
                   SliderConfig.Config.homing_speed_slide,
                   SliderConfig.Config.homing_speed_pan);

    if (len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CameraSlider_getHomingState(void)
{
    return bhomingComplete;
}

bool CameraSlider_StartMotion(void)
{
    if(!inMultiMotion){
    display.clearDisplay();
    }
    displayPercentage=true;
    // Configure slider
    stepper_slide.setTargetPositionInMillimeters(fStartPos_Slider);
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);

    // Configure pan
    stepper_pan.setTargetPositionInSteps(fStartPos_Rotation);
    stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.default_rotate_speed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_rotate_accel * SliderConfig.Config.pan_steps_per_degree);

    display.setCursor(0,44);
    display.print(dispTime(slideDurationSec)); 
    display.display();
    CameraSlider_SetState(SLIDER_MOVING_TO_START);

    return true;
}

bool CameraSlider_SetDuration(uint32_t durationSec)
{
    slideDurationSec = durationSec;
    return true;
}

bool CameraSlider_SetStartPosition(float slideStartPos, float rotStartPos)
{
    fStartPos_Slider = slideStartPos;
    fStartPos_Rotation = rotStartPos;

    return true;
}

bool CameraSlider_SetEndPosition(float slideEndPos, float rotEndPos)
{
    fEndPos_Slider = slideEndPos;
    fEndPos_Rotation = rotEndPos;

    return true;
}

bool CameraSlider_StoreAsStartPosition(void)
{
    fStartPos_Slider = getSliderPos();
    fStartPos_Rotation = getRotationPos(false);

    return true;
}

bool CameraSlider_StoreAsEndPosition(void)
{
    fEndPos_Slider = getSliderPos();
    fEndPos_Rotation = getRotationPos(false);

    return true;
}

bool CameraSlider_StoreAsRotationHome(void)
{
    stepper_pan.setTargetPositionInSteps(0);
    stepper_pan.setCurrentPositionInMillimeters(0);
    stepper_pan.setCurrentPositionInRevolutions(0);
    stepper_pan.setCurrentPositionInSteps(0);

    return true;
}

float getSliderPos()
{
    return stepper_slide.getCurrentPositionInMillimeters();
}

float getRotationPos(bool calculateDegrees)
{
#if 0
    return stepper_pan.getCurrentPositionInRevolutions();
#else
    if (calculateDegrees)
    {
        return SliderConfig.Config.rotate_direction * (stepper_pan.getCurrentPositionInSteps() / SliderConfig.Config.pan_steps_per_degree);
    }
    else
    {
        return SliderConfig.Config.rotate_direction * stepper_pan.getCurrentPositionInSteps();
    }
#endif
}
void setOuterHome()
{
    CameraSlider_UpdateRailLength(getSliderPos());
}
void CameraSlider_UpdateRailLength(uint32_t rail_length)
{
    if(rail_length<100){
        rail_length=100;
    }
    SliderConfig.Config.rail_length = rail_length;
    SliderConfig.Write();
}

void SwitchAD()
{
    // Serial.println("TRIGGERED");
    int reading = digitalRead(PIN_A_D); // Read digital value from GPIO26
                                        /*
                                           Serial.println("reading");
                                           Serial.println(reading);
                                           // If the switch changed, due to noise or pressing:
                                           if (reading != lastADButtonState) {
                                             // reset the debouncing timer
                                             lastDebounceTime = millis();
                                           }
                                         */
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        if (reading != lastADButtonState)
        {
            lastADButtonState = reading;

            // Only print the state change if it's different
            //Serial.println(reading);
        }
    }
    lastDebounceTime = millis();
    // analogInput=!analogInput;
}
