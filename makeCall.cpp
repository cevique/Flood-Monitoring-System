#include "globals.h"
#include "makeCall.h"
#include "checkCallStatus.h"

void makeCall(String number) {
  answered = false;   // reset flag before starting

  while (!answered && alarmState) {   // keep trying until answered or safe
    // Start call
    sim800.print("ATD");
    sim800.print(number);
    sim800.println(";");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Dialing...");

    unsigned long start = millis();

    // Check status for 20 seconds
    while (millis() - start < 20000 && !answered && alarmState) {
      checkCallStatus();
    }

    // Hang up after timeout or if answered
    sim800.println("ATH");

    if (answered) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Call answered!");
      delay(5000);
      lcd.clear();
      break;   // exit since picked
    } 
    else if (alarmState) {  // only redial if still unsafe
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("No answer");
      lcd.setCursor(0,1);
      lcd.print("Redialing...");
      delay(5000);  // wait before retry
      lcd.clear();
      // loop continues → will redial automatically
    }
  }

  // If alarm ended while calling
  if (!alarmState && !answered) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Safe now!");
    delay(3000);
    lcd.clear();
  }
}