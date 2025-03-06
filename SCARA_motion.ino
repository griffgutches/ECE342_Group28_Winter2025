#include <AccelStepper.h>

// Define stepper motor connections and interface type
#define X_MOTOR_STEP_PIN 3
#define X_MOTOR_DIR_PIN 4
#define Y_MOTOR_STEP_PIN 5
#define Y_MOTOR_DIR_PIN 6

#define MaxSpeed 1000
#define defaultSpeed 150

#define X_steps_per_mm 100 // needs to be changed
#define Y_steps_per_mm 100 // needs to be changed
#define mm_per_in 0.0394

bool absoluteMode = false; // true: absolute positioning, false: relative positioning 
bool inchMode = false;    // true: unit is inches, false: unit is mm

AccelStepper Xmotor(AccelStepper::DRIVER, X_MOTOR_STEP_PIN, X_MOTOR_DIR_PIN);
AccelStepper Ymotor(AccelStepper::DRIVER, Y_MOTOR_STEP_PIN, Y_MOTOR_DIR_PIN);

struct GCodeCommand 
{
    String commandType; // G0, G1, M2, etc.
    float x = 0.0;      // X-coordinate
    float y = 0.0;      // Y-coordinate
    float feedrate = 0.0; // Feedrate (optional, default 0)
};


void setup() 
{
    Serial.begin(115200);

    Xmotor.setMaxSpeed(MaxSpeed);         // Set max speed (adjust as needed)
    Ymotor.setMaxSpeed(MaxSpeed);

    Xmotor.setCurrentPosition(0.0);     // Define the origin at the current position 
    Ymotor.setCurrentPosition(0.0);

}


void loop() 
{
  if (Serial.available() > 0) 
  {
    // Read a line of incoming data 
    String command = Serial.readStringUntil('\n'); 
    command.trim(); // Remove any extra whitespace

    Serial.println(command);  // Double Check

    GCodeCommand gcode = parseCommand(command);

    unitConversion(gcode);

    printGCode(gcode);

    if(validateGCode(gcode))
    {
      processGCode(gcode);
    }
    else
    {
      Serial.println("Invalid G-Code. Possition and/or feedrate out of range.");
    }
  }
}


GCodeCommand  parseCommand(String command)
{
  GCodeCommand gcode;
  gcode.commandType = command.substring(0, 2); // Extract command type (e.g., "G0", "G1")

  int xIndex = command.indexOf('X');
  int yIndex = command.indexOf('Y');
  int fIndex = command.indexOf('F'); // Feedrate

  if (xIndex != -1) 
  {
    gcode.x = command.substring(xIndex + 1).toFloat();
  }

  if (yIndex != -1) 
  {
    gcode.y = command.substring(yIndex + 1).toFloat();
  }

  if (fIndex != -1) 
  {
    gcode.feedrate = command.substring(fIndex + 1).toFloat();
  }

  return gcode;
}


void unitConversion(GCodeCommand &gcode)
{
  if(inchMode)
  {
    gcode.x *= (mm_per_in * X_steps_per_mm);
    gcode.y *= (mm_per_in * Y_steps_per_mm);
    gcode.feedrate *= ((mm_per_in * X_steps_per_mm) / 60);
  }
  else
  {
    gcode.x *= (X_steps_per_mm);
    gcode.y *= (Y_steps_per_mm);
    gcode.feedrate *= ((X_steps_per_mm) / 60);
  }
}


bool validateGCode(GCodeCommand gcode)
{
  if(gcode.x < 0 || gcode.x > 800)
  {
    // out of range
    return false;
  }
  else if(gcode.y < 0 || gcode.y > 400) // update later
  {
    // out of range
    return false;
  }
  else if(gcode.feedrate < 0 || gcode.feedrate > MaxSpeed)
  {
    // out of range
    return false;
  }
  else
  {
    return true;
  }
}


void printGCode(GCodeCommand gcode)
{
  Serial.print("X: "); Serial.println(gcode.x);
  Serial.print("Y: "); Serial.println(gcode.y);
  Serial.print("Feedrate: "); Serial.println(gcode.feedrate);
}


void processGCode(GCodeCommand gcode) 
{
  if (gcode.commandType == "G0") 
  {
    Serial.println("Rapid Move Command Received");

    Xmotor.setSpeed(MaxSpeed);    // Set speed to max 
    Ymotor.setSpeed(MaxSpeed);
    
    if(absoluteMode)
    {
      Xmotor.moveTo(gcode.x);    // Move to location
      Ymotor.moveTo(gcode.y); 
    }
    else
    {
      Xmotor.move(gcode.x);    // Move a distance
      Ymotor.move(gcode.y); 
    } 
  } 
  else if (gcode.commandType == "G1") 
  {
    Serial.println("Linear Move Command Received");

    Xmotor.setSpeed(gcode.feedrate);    // Set speed to feedrate, (feedrate (mm/min) must be converted into steps per second. I need to know steps/mm for the conversion)
    Ymotor.setSpeed(gcode.feedrate);

    if(absoluteMode)
    {
      Xmotor.moveTo(gcode.x);    // Move to location
      Ymotor.moveTo(gcode.y); 
    }
    else
    {
      Xmotor.move(gcode.x);    // Move a distance
      Ymotor.move(gcode.y); 
    } 
  } 
  else if (gcode.commandType == "G90") 
  {
    Serial.println("Absolute Positioning Mode Activated");
    
    absoluteMode = true; 

  } 
  else if (gcode.commandType == "G91") 
  {
    Serial.println("Relative Positioning Mode Activated");
        
    absoluteMode = false; 

  } 
  else if (gcode.commandType == "G20") 
  {
    Serial.println("Units Set to Inches");
    
    inchMode = true;
  } 
  else if (gcode.commandType == "G21") 
  {
    Serial.println("Units Set to Millimeters");
        
    inchMode = false;
  } 
  else if (gcode.commandType == "M2" || gcode.commandType == "M6") 
  {
    Serial.println("Program End / Tool Change Command Received");
    
    Xmotor.setSpeed(defaultSpeed);    
    Ymotor.setSpeed(defaultSpeed);

    Xmotor.moveTo(0.0);    // Move to origin
    Ymotor.moveTo(0.0); 
  } 
  else 
  {
    Serial.println("Unknown Command");
  }
}





