// ALL INCLUDES AND VARIABLES FOR MOTOR CONTROL
#include <cmath>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <vector>
#include <string>
#include <L298N.h>


#define STEPS 200
#define LIMIT_SWITCH_PIN_VERT 2 // vertical axis limit switch
#define LIMIT_SWITCH_PIN_HORI 3 //horizontal axis limit switch

L298N stepperExtr(4, 5, 6);
AccelStepper stepperBase(1, 13, 12);
AccelStepper stepperHori(1, 11, 10);
AccelStepper stepperVert(1, 9, 8);


// Create MultiStepper for synchronized motion
MultiStepper steppers;

// Predefined characteristics
const int MAX_HEIGHT = 163;
const int MAX_RADIUS = 94;
const float PIP_WIDTH = 0.35; // cm
const float PIP_SPEED = 70;  // mm/s
const float EXTRUSION_SPEED = 90; // 1-255

// Conversion factors
const int TOP_STEPS = 21000;
const float MM_PER_STEP_FROST = 0; ////////////////set after getting frosting extruder set up
const float MM_PER_STEP_VERT = 0.0077; // 0.007762
const float MM_PER_STEP_HORI = 0.183984;
const float DEG_PER_STEP = 0.956126;
const float R_STEPS_PER_REV = 376.519544; //after converions
const float STEPS_PER_REV = 200.0; //og
const float SPIRAL_ADJUSTMENT = 0.75;

// Completion Boolean
bool COMPLETE = false;

// Booleans to say text once
bool cake_msg = 0;
bool start_msg = 0;

// State machine
enum State {
  GET_DIMENSIONS,
  HORI_LIMIT,
  VERT_LIMIT,
  MOVE_TO_HEIGHT,
  MOVE_VERT,
  MOVE_TO_START,
  MOVE_HORI,
  FROSTING_TOP,
  TOP_COMPLETE,
  RESET_TOP
};

State state = GET_DIMENSIONS; // frost top after getting dimensions

int cake_h = 0;
int cake_r = 0;
float current_r = 0;
int add_h_skip = 0;


long positions[2];
//new variables for smooth top frost
int total_r_steps;
int total_h_steps;
int r_steps;


unsigned long t_prev = 0;
const unsigned long T_INTERVAL = 20; // Update every


int coord_index = 0;

int i = 0;

void setup() {
  //set up limit switches
  pinMode(LIMIT_SWITCH_PIN_VERT, INPUT);
  pinMode(LIMIT_SWITCH_PIN_HORI, INPUT);


  // Set maximum speeds and acceleration

  stepperExtr.setSpeed(EXTRUSION_SPEED);

  stepperBase.setMaxSpeed(300);
  stepperBase.setAcceleration(500);
  stepperBase.setCurrentPosition(0);
 
  stepperHori.setMaxSpeed(300);
  stepperHori.setAcceleration(500);
  stepperHori.setCurrentPosition(0);
 
  stepperVert.setMaxSpeed(1500);
  stepperVert.setAcceleration(500);
  stepperVert.setCurrentPosition(0);
 
  // Add motors to MultiStepper
  steppers.addStepper(stepperBase);  // Index 0
  steppers.addStepper(stepperHori);  // Index 1


  Serial.begin(115200);
  Serial.println("-----STARTING-----");
  Serial.println("Send cake diameter and height as: diameter,height");
}

void loop() {
  uint32_t t = millis();

  stepperBase.run();
  stepperVert.run();
  stepperHori.run();

  if (state == GET_DIMENSIONS) {
    get_dimensions();
  }
  if (state == HORI_LIMIT || state == VERT_LIMIT) {
    setLimits(); // set limit switch location to 0
  } 
  if (state == RESET_TOP) {
    resetTop();
  }
  if (state == MOVE_TO_HEIGHT || state == MOVE_TO_START) {
    move_to_start();
  }


  if (t - t_prev >= T_INTERVAL) {
    t_prev = t;

    switch (state) {    

        case MOVE_HORI:
          moveHorizontal();
          break;

        case MOVE_VERT:
          moveVertical();
          break;

        case FROSTING_TOP:
          executeFrostingStep();
          break;
         
        case TOP_COMPLETE:
          break;
         
        default:
          break;
    }
  }
}

void setLimits() {
  if (state == HORI_LIMIT){
    stepperHori.setSpeed(100);
    if (digitalRead(LIMIT_SWITCH_PIN_HORI) == HIGH) {
      stepperHori.stop();
      stepperHori.setCurrentPosition(round(MAX_RADIUS / MM_PER_STEP_HORI)); // set to max position in steps
      stepperHori.moveTo(0);
      state = VERT_LIMIT;
    }
  } else if (state == VERT_LIMIT){
    stepperVert.move(-100000);
    if (digitalRead(LIMIT_SWITCH_PIN_VERT) == HIGH){
      stepperVert.stop();
      stepperVert.setCurrentPosition(0); //set to min position
      stepperVert.moveTo(TOP_STEPS);
      state = RESET_TOP;
    }
  }
}

void resetTop() {
  if (stepperVert.distanceToGo() == 0 && !COMPLETE) {
    state = MOVE_TO_HEIGHT;  // FIXED: was STARTING_POSITION which doesn't exist
  }
  else if (stepperVert.distanceToGo() == 0) {
    state = TOP_COMPLETE;
  }
}

void move_to_start() {  // FIXED: added void return type
  if (state == MOVE_TO_HEIGHT) {  // FIXED: was = instead of ==
    float move_down = MAX_HEIGHT - (cake_h + PIP_WIDTH); //calc mm distance
    long v_steps = round(move_down / MM_PER_STEP_VERT); //convert distance to steps
    stepperVert.moveTo(stepperVert.currentPosition() - v_steps); // move down to cake height
    state = MOVE_VERT;
  } else if (state == MOVE_TO_START) {
    stepperHori.moveTo(1);
    state = MOVE_HORI;
  }
}

void get_dimensions() {
  if (!Serial.available()) return;

  if (Serial.available() > 0) {
    // Read the values sequentially: diameter,height
    cake_r = Serial.parseInt() / 2;  // FIXED: use global variable, divide diameter by 2 to get radius
    cake_h = Serial.parseInt();      // FIXED: use global variable
    
    Serial.print("Received - Diameter: ");
    Serial.print(cake_r * 2);
    Serial.print("mm, Height: ");
    Serial.print(cake_h);
    Serial.println("mm");
    Serial.print("Calculated Radius: ");
    Serial.print(cake_r);
    Serial.println("mm");
    
    if (cake_r > 0 && cake_h >= 0) {
      state = HORI_LIMIT;
      Serial.println("Starting limit calibration...");
    } else {
      Serial.println("Invalid dimensions, please resend");
    }
  }
}

void moveVertical() {
  if (stepperVert.distanceToGo() == 0) {
    Serial.println("vertical position reached");
    state = MOVE_TO_START;
  }
}

void moveHorizontal() {
  if (stepperHori.distanceToGo() == 0) {
    Serial.println("starting position reached");
    stepperExtr.forward();
    delay(1500); // to get frosting out at the start
    state = FROSTING_TOP;
  }
}

void executeFrostingStep() {
  r_steps = stepperBase.currentPosition();

  if (((stepperHori.currentPosition() * MM_PER_STEP_HORI)) < (cake_r-1)) {
    current_r = ((stepperHori.currentPosition() * MM_PER_STEP_HORI));

    double omega = PIP_SPEED / sqrt(PIP_WIDTH*PIP_WIDTH + current_r*current_r);
    double radial_mm_per_s = PIP_WIDTH * omega;

    double base_steps_per_sec = omega * R_STEPS_PER_REV / (2 * M_PI);
    double hori_steps_per_sec = radial_mm_per_s / MM_PER_STEP_HORI;

    stepperBase.setSpeed(base_steps_per_sec);
    stepperHori.setSpeed(hori_steps_per_sec);
    if (!stepperExtr.isMoving()) {
      stepperExtr.forward();
    }
  } else {
    stepperBase.stop();
    stepperHori.stop();

    stepperExtr.backward();
    delay(4000);
    stepperExtr.stop();

    state = RESET_TOP;
    stepperVert.moveTo(20500);
    COMPLETE = true;
    return;
  }
}