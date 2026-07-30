// One stepper at constant speed, reversing every few seconds.
//
// Wiring (A4988 / DRV8825 / TMC2209 in step-dir mode):
//   GPIO 25 -> STEP, GPIO 26 -> DIR, GPIO 27 -> ENABLE (optional),
//   GND -> GND (common ground is required).
//
// Set the driver's current limit before powering the motor.

#include <HWStepGen.h>

hwsg::Stepper motor;

void setup() {
  Serial.begin(115200);
  delay(200);

  hwsg::StepperConfig config;
  config.stepPin = 25;
  config.dirPin = 26;
  config.enablePin = 27;
  config.stepsPerRev = 200;  // 1.8 degree motor
  config.microsteps = 16;    // match the driver's MS jumpers

  hwsg::Error error = motor.begin(config);
  if (error != hwsg::Error::Ok) {
    Serial.printf("motor.begin() failed: %s\n", hwsg::toString(error));
    while (true) {
      delay(1000);
    }
  }

  Serial.printf("Usable speed range: %.1f - %.0f rpm\n", motor.minRpm(),
                motor.maxRpm());

  motor.setSpeed(60.0f);
}

void loop() {
  delay(5000);

  // Negative speed reverses. The library halts pulses, flips DIR and waits
  // out the driver's setup time before resuming.
  motor.setSpeed(-motor.speedRpm());

  Serial.printf("now %.1f rpm (hardware: %.1f rpm)\n", motor.speedRpm(),
                motor.actualSpeedRpm());
}
