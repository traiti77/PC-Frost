
#include <L298N.h>
L298N stepperExtr(4, 5, 6);

const float EXTRUSION_SPEED = 200; // 1-255

void setup() {
  // put your setup code here, to run once:
  stepperExtr.setSpeed(EXTRUSION_SPEED);

}

void loop() {
  // put your main code here, to run repeatedly:
  stepperExtr.forward();
}
