#include "globals.h"
#include "sendSMS.h"

void sendSMS(String number, String text) {
  // Set SMS mode to text
  sim800.println("AT+CMGF=1");    
  delay(1000);

  // Send SMS command
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(1000);

  // Send the message body
  sim800.print(text);
  delay(500);

  // End the message with Ctrl+Z (ASCII 26)
  sim800.write(26);

  // Show confirmation on LCD
  lcd.setCursor(0,0);
  lcd.print("SMS Sent!");
  delay(5000);
  lcd.clear();
}