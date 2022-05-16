#include "TofImager.h"

TofImager tofImager;

void setup() {
  tofImager.Init();
}

void loop() {
  tofImager.Update();
}
