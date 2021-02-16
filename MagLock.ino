
#include "rdm630.h"


#define TX_PIN 2
#define LOCK_PIN A2
#define SENSOR_TRIG_PIN 6
#define SENSOR_ECH_PIN 5
#define SENSOR_PWR_PIN 7 

#define TRIGGER_MAX_THRESHOLD 10
#define TRIGGER_MIN_THRESHOLD 2
#define SENSOR_CHECK 10
#define OPEN_TIME 7000
#define COOLDOWN_TIME 500
#define REST_TIME 5000

#define TIME_ELLAPSED (unsigned long)(millis() - currentStatus.sensorTime)

rdm630 rfid(TX_PIN, 0);

enum states
{
  idle,
  read,
  triggered,
  accepted,
  denied,
  error
};

struct status
{
  states state;
  int cardNum;
  unsigned long sensorTime, cardTime, card, eepromAdd, whiteList[100];
  bool locked;

} currentStatus;


void(* resetFunc) (void) = 0;

unsigned long readCard()
{
  unsigned long result = 0;

  byte data[6];
  byte length;

  if (rfid.available()) {
    rfid.getData(data, length);
    result =
      ((unsigned long int)data[1] << 24) +
      ((unsigned long int)data[2] << 16) +
      ((unsigned long int)data[3] << 8) +
      data[4];
  }

  return result;

}

bool Authorized(unsigned long card)
{
  for (int i = 0; i < currentStatus.cardNum; i++)
  {
    if (currentStatus.whiteList[i] == card)
      return true;
  }
  return false;
}

unsigned long dummy_card;


bool checkSensor()
{
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(SENSOR_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SENSOR_TRIG_PIN, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(SENSOR_ECH_PIN, HIGH);

  // Calculating the distance
  long distance = duration * 0.034 / 2;

  return (distance < TRIGGER_MAX_THRESHOLD &&  distance > TRIGGER_MIN_THRESHOLD);

}



void setup()
{
  rfid.begin();


  pinMode(LOCK_PIN, OUTPUT);
  //pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, HIGH);
  pinMode(SENSOR_TRIG_PIN, OUTPUT);
  pinMode(SENSOR_ECH_PIN, INPUT);

  pinMode(SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(SENSOR_PWR_PIN, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);



/*

DEFINE HERE your whitelisted cards
  currentStatus.whiteList[0] = 123123;
  currentStatus.whiteList[1] = 234234;
  currentStatus.whiteList[2] = 345345;

*/


  currentStatus.cardTime = millis();
  currentStatus.sensorTime = millis();
  currentStatus.locked = false;
  currentStatus.state = idle;
}


void loop()
{
  dummy_card = readCard();


  switch (currentStatus.state)
  {
    case idle:
      if (dummy_card != 0)
      {
        currentStatus.card = dummy_card;
        currentStatus.cardTime = millis();
        currentStatus.state = read;
      }

      if ( TIME_ELLAPSED > SENSOR_CHECK)
      {
        if (checkSensor())
        {
          currentStatus.state = triggered;
        }

        currentStatus.sensorTime = millis();
      }

      break;
    case read:

      if (Authorized(currentStatus.card))
      {
        currentStatus.state = accepted;
      }
      else
      {
        currentStatus.state = denied;
      }
      break;
    case accepted:
      if (currentStatus.locked == false)
      {
        digitalWrite(LOCK_PIN, LOW);

        currentStatus.locked = true;
        currentStatus.cardTime = millis();
      }
      else if ( TIME_ELLAPSED > OPEN_TIME)
      {
        digitalWrite(LOCK_PIN, HIGH);

        currentStatus.locked = false;
        currentStatus.state = idle;
      }
      break;
    case denied:
      if (currentStatus.locked == false)
      {
        currentStatus.locked = true;
        currentStatus.cardTime = millis();
      }
      else if ( TIME_ELLAPSED > COOLDOWN_TIME)
      {
        currentStatus.locked = false;
        currentStatus.state = idle;
      }
      break;
    case triggered:
      if (currentStatus.locked == false)
      {
        digitalWrite(LOCK_PIN, LOW);

        currentStatus.locked = true;
        currentStatus.sensorTime = millis();
      }
      else if ( TIME_ELLAPSED > OPEN_TIME)
      {
        digitalWrite(LOCK_PIN, HIGH);

        currentStatus.locked = false;
        currentStatus.state = idle;
      }
      break;
    case error:
      resetFunc(); // reset device
    break;

    default:
      currentStatus.state = error;
      break;
  }

}
