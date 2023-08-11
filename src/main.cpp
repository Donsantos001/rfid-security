#include <Arduino.h>
#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define motionSensor 4
#define buzzer 6
#define alarm 5
#define redLed 8
#define greenLed 7

long timeDifference(long);
boolean readID();

Servo myservo;
byte readCard[4];
String tag_UID = "778A6634";
String tagID = "";
MFRC522 mfrc522(SS_PIN, RST_PIN);
int pos = 0;
int flag = 0;

long redTime, alarmTime, buzzerTime, servoTime, closeTime;

int motionState = LOW;
int alarmState = LOW;
int servoState = 0;
int buzzerState = 0;
int redLedState = 0;

int closeDegree = 10;
int servoDegree = 10;
int openDegree = 150;
int alarmCount = 0, beepCount = 2;
int buzzerCount = 0;

void setup()
{
  pinMode(motionSensor, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(alarm, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  myservo.write(servoDegree);
  myservo.attach(3);

  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  redTime = millis();
  closeTime = -3000; // you can leave at 0
}
void loop()
{
  if (flag == 0)
  {
    // controls the redLed for unauthorized
    if (redLedState == 1)
    {
      if (timeDifference(redTime) < 2000)
      {
        digitalWrite(redLed, HIGH);
      }
      else
      {
        digitalWrite(redLed, LOW);
        redTime = millis();
        redLedState = 0;
      }
    }

    // controls the buzzer for unauthorized
    if (buzzerState == 2)
    {
      if (timeDifference(buzzerTime) < 2000)
      {
        digitalWrite(buzzer, HIGH);
      }
      else
      {
        digitalWrite(buzzer, LOW);
        buzzerTime = millis();
        buzzerState = 0;
      }
    }

    // if motion is detected and the door is closed
    if (digitalRead(motionSensor) == HIGH && alarmState == LOW && timeDifference(closeTime) >= 5000)
    {
      alarmState = HIGH;
      alarmCount = 0;
      alarmTime = millis();
    }

    // controls alarm based when motion was detecteds
    if (alarmState == HIGH)
    {
      if (timeDifference(alarmTime) < 1000)
      {
        digitalWrite(alarm, HIGH);
      }
      else if (timeDifference(alarmTime) < 1500)
      {
        digitalWrite(alarm, LOW);
      }
      else
      {
        alarmCount += 1;
        alarmTime = millis();
      }

      if (alarmCount >= beepCount)
      {
        alarmCount = 0;
        alarmState = LOW;
        closeTime = millis();
      }
    }

    // if user scans his/her card
    if (readID())
    {
      if (tagID == tag_UID)
      {
        servoState = 1;
        buzzerState = 1;
        flag = 1;
        redLedState = 0;
        alarmState = LOW;
        buzzerTime = millis();
        digitalWrite(alarm, LOW);
        digitalWrite(greenLed, HIGH);
      }
      else
      {
        buzzerState = 2;
        redLedState = 1;
        redTime = millis();
        buzzerTime = millis();
      }
    }
  }

  if (flag == 1)
  {
    if (digitalRead(motionSensor) == HIGH && servoDegree >= openDegree)
    {
      servoTime = millis();
    }
  }

  // controls the led blinking state
  if (redLedState == 0)
  {
    if (timeDifference(redTime) < 2000)
    {
      digitalWrite(redLed, LOW);
    }
    else if (timeDifference(redTime) < 2100)
    {
      digitalWrite(redLed, HIGH);
    }
    else
    {
      redTime = millis();
    }
  }

  // controls servo based on state
  if (servoState == 1)
  {
    servoDegree += 2;
    myservo.write(servoDegree);
    if (servoDegree >= openDegree)
    {
      servoState = 2;
      servoTime = millis();
    }
  }
  if (servoState == 2 && timeDifference(servoTime) >= 4000)
  {
    servoDegree -= 2;
    myservo.write(servoDegree);
    if (servoDegree <= closeDegree)
    {
      servoState = 0;
      digitalWrite(greenLed, LOW);
      flag = 0;
      closeTime = millis();
    }
  }

  // controls the buzzer state
  if (buzzerState == 1)
  {
    if (timeDifference(buzzerTime) < 200)
    {
      digitalWrite(buzzer, HIGH);
    }
    else if (timeDifference(buzzerTime) < 400)
    {
      digitalWrite(buzzer, LOW);
    }
    else
    {
      buzzerCount += 1;
      buzzerTime = millis();
    }

    if (buzzerCount >= beepCount)
    {
      buzzerCount = 0;
      buzzerState = 0;
    }
  }

  delay(20);
}

// Checks the time elapsed from a reference time
long timeDifference(long prevTime)
{
  return millis() - prevTime;
}

// Read new tag if available
boolean readID()
{
  // Check if a new tag is detected or not. If not return.
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return false;
  }
  // Check if a new tag is readable or not. If not return.
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return false;
  }
  tagID = "";
  // Read the 4 byte UID
  for (uint8_t i = 0; i < 4; i++)
  {
    // readCard[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Convert the UID to a single String
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Stop reading
  return true;
}