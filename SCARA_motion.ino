#include <AccelStepper.h>

// Define stepper motor connections and interface type
#define MOTOR1_STEP_PIN 1
#define MOTOR1_DIR_PIN 2
#define MOTOR2_STEP_PIN 3
#define MOTOR2_DIR_PIN 4

AccelStepper motor1(AccelStepper::DRIVER, MOTOR1_STEP_PIN, MOTOR1_DIR_PIN);
AccelStepper motor2(AccelStepper::DRIVER, MOTOR2_STEP_PIN, MOTOR2_DIR_PIN);

void setup() 
{
    Serial.begin(115200);

    // Set default acceleration for the motors
    motor1.setAcceleration(2000); // Set acceleration (adjust as needed)
    motor2.setAcceleration(2000); // Set acceleration (adjust as needed)
}

void loop() 
{
  if (Serial.available() > 0) 
  {

    // Read a line of incoming data 
    String command = Serial.readStringUntil('\n'); 

    // Expecting a string in the format: "M1:(motor1_position) M2:(motor2_position) S:(speed)"
    if (command.startsWith("M1:") && command.indexOf("M2:") != -1 && command.indexOf("S:") != -1)
    {
      // Parse the motor 1, motor 2 positions and speed (in steps per second)
      int motor1_pos = command.substring(3, command.indexOf("M2:")).toInt();
      int motor2_pos = command.substring(command.indexOf("M2:") + 3, command.indexOf("S:")).toInt();
      float speed = command.substring(command.indexOf("S:") + 2).toFloat();
    
      // Set the speed (in steps per second) for both motors based on the received speed
      motor1.setMaxSpeed(speed);
      motor2.setMaxSpeed(speed);
    
      // Move the motors to the received positions
      motor1.moveTo(motor1_pos);
      motor2.moveTo(motor2_pos);
    }
  }
}
