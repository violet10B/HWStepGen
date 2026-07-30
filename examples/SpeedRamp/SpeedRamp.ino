// Smooth acceleration with hwsg::Ramp: accelerate to cruise, hold,
// decelerate to a standstill, reverse, repeat. Nothing blocks.
//
// setSpeed() changes the pulse rate instantly, and a motor commanded
// straight to a high speed stalls. The ramp walks the speed towards the
// target instead; direction changes happen at a standstill.

#include <HWStepGen.h>

const float kCruiseRpm = 240.0f;
const uint32_t kHoldMs = 3000;

hwsg::Stepper motor;
hwsg::Ramp ramp;

float nextCruiseRpm = -kCruiseRpm;
uint32_t settledAtMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  hwsg::StepperConfig config;
  config.stepPin = 25;
  config.dirPin = 26;
  config.stepsPerRev = 200;
  config.microsteps = 16;

  hwsg::Error error = motor.begin(config);
  if (error != hwsg::Error::Ok) {
    Serial.printf("motor.begin() failed: %s\n", hwsg::toString(error));
    while (true) {
      delay(1000);
    }
  }

  ramp.attach(motor);
  ramp.setAcceleration(180.0f);  // average rpm per second
  ramp.setEasing(hwsg::Easing::SmoothStep);
  ramp.setTargetSpeed(kCruiseRpm);
}

void loop() {
  if (ramp.update()) {
    return;  // still ramping
  }

  if (settledAtMs == 0) {
    settledAtMs = millis();
    Serial.printf("settled at %.1f rpm\n", motor.actualSpeedRpm());
    return;
  }
  if (millis() - settledAtMs < kHoldMs) {
    return;
  }
  settledAtMs = 0;

  // cruise -> 0 -> opposite cruise -> 0 -> ...
  if (ramp.targetSpeed() == 0.0f) {
    ramp.setTargetSpeed(nextCruiseRpm);
    nextCruiseRpm = -nextCruiseRpm;
  } else {
    ramp.stop();
  }
}
