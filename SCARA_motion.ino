#include <AccelStepper.h>

// Define stepper motor connections 
#define X_MOTOR_STEP_PIN 3
#define X_MOTOR_DIR_PIN 4
#define Y_MOTOR_STEP_PIN 5
#define Y_MOTOR_DIR_PIN 6

// Define speed parameters
#define MaxSpeed 6000
#define defaultSpeed 2500

// Define conversion factors
#define steps_per_mm .9 
#define mm_per_in 25.4

// Flags
bool absoluteMode = false; // true: absolute positioning, false: relative positioning 
bool inchMode = false;    // true: unit is inches, false: unit is mm

// Motors
AccelStepper Xmotor(AccelStepper::DRIVER, X_MOTOR_STEP_PIN, X_MOTOR_DIR_PIN);
AccelStepper Ymotor(AccelStepper::DRIVER, Y_MOTOR_STEP_PIN, Y_MOTOR_DIR_PIN);

// GCodeCommand definition 
struct GCodeCommand 
{
    String commandType; // G0, G91, M2, etc.
    float x = 0.0;      // X-coordinate
    float y = 0.0;      // Y-coordinate
    float feedrate = 0.0; // Feedrate 
};

//-------------------------------------------------------------------------------

void setup() 
{
    Serial.begin(115200);

    // Set max speed 
    Xmotor.setMaxSpeed(MaxSpeed);         
    Ymotor.setMaxSpeed(MaxSpeed);

    // Set Acceleration
    Xmotor.setAcceleration(1000);
    Ymotor.setAcceleration(1000);

    // Define the origin at the current position 
    Xmotor.setCurrentPosition(0.0);     
    Ymotor.setCurrentPosition(0.0);

}


void loop() 
{
  if (Serial.available() > 0) 
  {
    // Read a line of incoming data 
    String command = Serial.readStringUntil('\n'); 
    command.trim(); // Remove any extra whitespace

    Serial.print("G-code command recieved: "); Serial.println(command);  // Used for debugging

    GCodeCommand gcode = parseCommand(command);

    unitConversion(gcode); 

    printGCode(gcode);   // Used for debugging

    // Run Command
    processGCode(gcode);
  }
}

//-------------------------------------------------------------------------------
// Helper Functions
//-------------------------------------------------------------------------------

void setOrigin()
{
    // Define the origin at the current position 
    Xmotor.setCurrentPosition(0.0);     
    Ymotor.setCurrentPosition(0.0);

    Serial.println("Origin defined at the current position. \n");
}

// Parses the g-code command sent from the Python GUI into a GCodeCommand struct
GCodeCommand  parseCommand(String command)
{
  GCodeCommand gcode;

  gcode.commandType = command.substring(0, 3); // Extract command type (e.g., "G0", "G91")

  // Check if the third character is a space
  if (command.charAt(2) == ' ') 
  {
    gcode.commandType = command.substring(0, 2); // Drop the space and keep only the first two characters
  }

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


// Converts the g-code units steps 
void unitConversion(GCodeCommand &gcode)
{
  if(inchMode)
  {
    gcode.x *= (mm_per_in * steps_per_mm);
    gcode.y *= (mm_per_in * steps_per_mm);
    gcode.feedrate *= ((mm_per_in * steps_per_mm) / 60.0f);
  }
  else
  {
    gcode.x *= (steps_per_mm);
    gcode.y *= (steps_per_mm);
    gcode.feedrate *= ((steps_per_mm) / 60.0f);
  }
}


// Prints the contents of a GCodeCommand struct 
void printGCode(GCodeCommand gcode)
{
  Serial.println("Parsed g-code (converted to steps):");
  Serial.print("Command: "); Serial.println(gcode.commandType);
  Serial.print("X: "); Serial.println(gcode.x);
  Serial.print("Y: "); Serial.println(gcode.y);
  Serial.print("Feedrate: "); Serial.println(gcode.feedrate);
}


// Moves the stepper motors to their target location
void go()
{
  while(Xmotor.currentPosition() != Xmotor.targetPosition() || Ymotor.currentPosition() != Ymotor.targetPosition()  )
  {
    Xmotor.run();
    Ymotor.run();
  }
}

//-------------------------------------------------------------------------------
// Primary Function 
//-------------------------------------------------------------------------------

// This function operates the stepper motors based on the g-code command
void processGCode(GCodeCommand gcode) 
{
  if (gcode.commandType == "G0") 
  {
    Serial.println("Rapid Move Command Received\n");

    Xmotor.setMaxSpeed(MaxSpeed);    // Set speed to max 
    Ymotor.setMaxSpeed(MaxSpeed);
    
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

    go();
  } 
  else if (gcode.commandType == "G1") 
  {
    Serial.println("Linear Move Command Received\n");

    Xmotor.setMaxSpeed(gcode.feedrate);    // Set speed to feedrate, (feedrate (mm/min) must be converted into steps per second. I need to know steps/mm for the conversion)
    Ymotor.setMaxSpeed(gcode.feedrate);

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

    go();
  } 
  else if (gcode.commandType == "G90") 
  {
    Serial.println("Absolute Positioning Mode Activated\n");
    
    absoluteMode = true; 

  } 
  else if (gcode.commandType == "G91") 
  {
    Serial.println("Relative Positioning Mode Activated\n");
        
    absoluteMode = false; 

  } 
  else if (gcode.commandType == "G20") 
  {
    Serial.println("Units Set to Inches\n");
    
    inchMode = true;
  } 
  else if (gcode.commandType == "G21") 
  {
    Serial.println("Units Set to Millimeters\n");
        
    inchMode = false;
  } 
  else if (gcode.commandType == "M2" || gcode.commandType == "M6") 
  {
    Serial.println("Program End / Tool Change Command Received\n");
    
    Xmotor.setMaxSpeed(defaultSpeed);    
    Ymotor.setMaxSpeed(defaultSpeed);

    Xmotor.moveTo(0.0);    // Move to origin
    Ymotor.moveTo(0.0); 

    go();
  } 
  else 
  {
    Serial.println("Unknown Command");

    // Check for pre-programed design 
    checkSpecialInput(gcode);
  }
}

//-------------------------------------------------------------------------------
// Fun Functions 
//-------------------------------------------------------------------------------


// Function to check for pre-programed design 
void checkSpecialInput(GCodeCommand gcode)
{
  Serial.println("Checking for special input...");

  if(gcode.commandType == "Cro")  // Cross
  {
    drawCross(gcode.x);
  }
  else if(gcode.commandType == "Box") // Box
  {
    drawBox(gcode.x);
  }
  else if(gcode.commandType == "Che") // Check
  {
    drawCheck(gcode.x);
  }
  else if(gcode.commandType == "Zig") // Zig-Zag
  {
    drawZigZag(gcode.x, gcode.y);
  }
  else if(gcode.commandType == "00")  // Re-set origin at current position
  {
    setOrigin();
  }
  else if(gcode.commandType == "SL")  // Straight Line
  {
    straightLine();
  }
  else if(gcode.commandType == "28")  // Draw number 28
  {
    groupNumber(gcode.x);
  }
  else if(gcode.commandType == "C1") // Cool Design 1 (3 Crosses)
  {
    cool1();
  }
  else if(gcode.commandType == "C2")  // Cool Design 2  (Boxes in Boxes)
  {
    cool2();
  }
  else if(gcode.commandType == "C3")  // Cool Design 3  (Extended Zig-Zag Pattern)
  {
    cool3(gcode.feedrate);
  }
  else if(gcode.commandType == "C4")  // Cool Design 4 (3D Cross)
  {
    cool4();
  }
  else
  {
    Serial.println("No special input found. \n");
  }
}


// Draws a cross (input scales)
void drawCross(float s)
{
  Serial.println("Drawing Cross\n");

  Xmotor.setMaxSpeed(MaxSpeed);    // Set speed to max 
  Ymotor.setMaxSpeed(MaxSpeed);

  setOrigin();

  Xmotor.moveTo(0.0);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(0.0); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo((-5.0)*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo((-5.0)*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0); 
  go();
  Xmotor.moveTo(0.0);    
  Ymotor.moveTo(0.0); 
  go();
}


// Draws a box (input scales)
void drawBox(float s)
{
  Serial.println("Drawing Box\n");
  
  Xmotor.setMaxSpeed(MaxSpeed);    // Set speed to max 
  Ymotor.setMaxSpeed(MaxSpeed);

  // square one
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(0.0);    
  Ymotor.moveTo(0.0); 
  go();

  // edge 1
  Xmotor.moveTo(5.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();

  // square 2
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(15.0*s); 
  go();
  Xmotor.moveTo(5.0*s);    
  Ymotor.moveTo(15.0*s); 
  go();
  Xmotor.moveTo(5.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();


  // edge 2
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(5.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();

  // edge 3
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(15.0*s);    
  Ymotor.moveTo(15.0*s); 
  go();

  // edge 4
  Xmotor.moveTo(5.0*s);    
  Ymotor.moveTo(15.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();

  Xmotor.moveTo(0.0);    
  Ymotor.moveTo(0.0); 
  go();
}


// Draws a check (input scales) needs work!
void drawCheck(float s)
{
  Serial.println("Drawing Check\n");

  Xmotor.setMaxSpeed(MaxSpeed);    // Set speed to max 
  Ymotor.setMaxSpeed(MaxSpeed);
  
  Xmotor.moveTo(-10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(-10.0*s);    
  Ymotor.moveTo(9.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(24.0*s); 
  go();
}


// Draws a zig-zag pattern (inputs dictate the number of zigzags and the direction of pattern)
void drawZigZag(float c, float dir)
{
  float direction; 

  if(dir == 0.0)
  {
    direction = 1.0;
  }
  else
  {
    direction = -1.0;
  }
  
  Serial.println("Drawing Zig-Zag Pattern\n");

  int count = ceil(c); // convert input to an integer

  Xmotor.setMaxSpeed(MaxSpeed);    // Set speed to max 
  Ymotor.setMaxSpeed(MaxSpeed);
  
  // as many zigzags as specified by the user
  for(int i = 0; i < count; i++)  
  {
    // Zig-Zag
    Xmotor.move(20.0);    
    Ymotor.move(0.0); 
    go();
    Xmotor.move(0.0);    
    Ymotor.move(5.0*direction); 
    go();
    Xmotor.move(-20.0);    
    Ymotor.move(0.0); 
    go();
    Xmotor.move(0.0);    
    Ymotor.move(5.0*direction); 
    go();
  }
}

void straightLine()
{
  Serial.println("Drawing a 10in straight line. \n");

  Xmotor.move(107);    
  Ymotor.move(10.0); 
  go();
  Xmotor.move(107);    
  Ymotor.move(10.0); 
  go();
}


void cool1()
{
  delay(5000);

  drawCross(3);
  Xmotor.move(0.0);    
  Ymotor.move(60.0); 
  go();
  setOrigin();

  drawCross(2);
  Xmotor.move(0.0);    
  Ymotor.move(-120.0); 
  go();
  setOrigin();

  drawCross(2);
  setOrigin();

  Xmotor.move(-30.0);    
  Ymotor.move(-30.0); 
  go();
}

void cool2()
{
  setOrigin();

  delay(5000);

  drawBox(5);

  Xmotor.move(-30.0);    
  Ymotor.move(-30.0); 
  go();

  delay(2000);

  Xmotor.moveTo(0.0);    
  Ymotor.moveTo(0.0); 
  go();

  drawBox(4);
  drawBox(3);
  drawBox(2);
  drawBox(1);
  drawBox(0.5);

  Xmotor.move(-30.0);    
  Ymotor.move(-30.0); 
  go();
}

void cool3(float feedrate)
{
  Xmotor.setMaxSpeed(feedrate);    // Set speed to feedrate, (feedrate (mm/min) must be converted into steps per second. I need to know steps/mm for the conversion)
  Ymotor.setMaxSpeed(feedrate);

  delay(5000);

  drawZigZag(5, 0);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();

  drawZigZag(5, 1);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();

  drawZigZag(5, 0);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();

  drawZigZag(5, 1);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();

  drawZigZag(5, 0);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();

  drawZigZag(5, 1);
  Xmotor.move(20.0);    
  Ymotor.move(0.0); 
  go();
}

void cool4()
{
  delay(5000);

  drawCross(3);
  Xmotor.move(5.0);    
  Ymotor.move(-5.0); 
  go();
  setOrigin();

  drawCross(3);
  setOrigin();

  Xmotor.move(-30.0);    
  Ymotor.move(-30.0); 
  go();
}

void groupNumber(float s)
{
  delay(5000);

  setOrigin();

  // two
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();

  // transition
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(20.0*s); 
  go();
  setOrigin();

  // eight
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(20.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
  Xmotor.moveTo(10.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(10.0*s); 
  go();
  Xmotor.moveTo(0.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();

  // transition
  Xmotor.moveTo(-20.0*s);    
  Ymotor.moveTo(0.0*s); 
  go();
}






