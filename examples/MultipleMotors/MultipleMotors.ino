// Three axes at independent speeds, stopped together through a StepperBank.
//
// Each stepper takes an LEDC timer of its own, so the motor count is capped
// per chip: 8 on the ESP32, 4 on the S2/S3/P4, 3 on the C3/C6/H2.

#include <HWStepGen.h>

hwsg::Stepper axisX;
hwsg::Stepper axisY;
hwsg::Stepper axisZ;
hwsg::StepperBank axes;

bool startAxis(hwsg::Stepper &stepper, int8_t stepPin, int8_t dirPin,
               const char *name) {
  hwsg::StepperConfig config;
  config.stepPin = stepPin;
  config.dirPin = dirPin;
  config.stepsPerRev = 200;
  config.microsteps = 8;

  hwsg::Error error = stepper.begin(config);
  if (error != hwsg::Error::Ok) {
    Serial.printf("%s failed to start: %s\n", name, hwsg::toString(error));
    return false;
  }
  Serial.printf("%s on LEDC channel %d\n", name, stepper.channel());
  axes.add(stepper);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.printf("free stepper slots: %u\n",
                hwsg::Stepper::availableChannels());

  startAxis(axisX, 25, 26, "X");
  startAxis(axisY, 32, 33, "Y");
  startAxis(axisZ, 18, 19, "Z");

  axisX.setSpeed(30.0f);
  axisY.setSpeed(60.0f);
  axisZ.setSpeed(-90.0f);
}

void loop() {
  delay(4000);
  axes.stopAll();

  delay(1000);
  for (hwsg::Stepper *motor : axes) {
    motor->setSpeed(45.0f);
  }
}
