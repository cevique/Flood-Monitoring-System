#include "globals.h"
#include "checkCallStatus.h"

void checkCallStatus() {
  while (sim800.available()) {
    String response = sim800.readStringUntil('\n');
    response.trim();
    if (response.length() == 0) continue;  // skip empty lines

    Serial.println("URC: " + response);

    if (response.indexOf("VOICE CALL: BEGIN") != -1) {
      answered = true;
    }
    else if (response.indexOf("VOICE CALL: END") != -1) {
      answered = false;
    }
    else if (response.indexOf("NO CARRIER") != -1 || response.indexOf("BUSY") != -1) {
      answered = false;
    }
  }
}