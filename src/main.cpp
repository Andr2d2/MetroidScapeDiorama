#include <Arduino.h>
#include <Thread.h>
#include <ThreadController.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define BASE_LEDS_PIN 3
#define CAPSULE_PIN 6
#define MIST_MACHINE_PIN 7
#define BOTTOM_TUBE_PIN 2
#define CABLE_PIN 8

// ThreadController that will controll all threads
ThreadController controll = ThreadController();
SoftwareSerial mySerial(10, 11); // TX, RX
DFRobotDFPlayerMini soundPlayer;
bool isPlaying = false;

Thread playSoundThread = Thread();
Thread pulseBaseLedsThread = Thread();
Thread blinkLedsThread = Thread();
Thread capsuleLedThread = Thread();
Thread mistMachineThread = Thread();
Thread cableThread = Thread();

void printLogMessage(uint8_t type, int value)
{
  switch (type)
  {
  case TimeOut:
    Serial.println(F("LOG - Time Out!"));
    break;
  case WrongStack:
    Serial.println(F("LOG - Stack Wrong!"));
    break;
  case DFPlayerCardInserted:
    Serial.println(F("LOG - Card Inserted!"));
    break;
  case DFPlayerCardRemoved:
    Serial.println(F("LOG - Card Removed!"));
    break;
  case DFPlayerCardOnline:
    Serial.println(F("LOG - Card Online!"));
    break;
  case DFPlayerUSBInserted:
    Serial.println("LOG - USB Inserted!");
    break;
  case DFPlayerUSBRemoved:
    Serial.println("LOG - USB Removed!");
    break;
  case DFPlayerPlayFinished:
    Serial.print(F("LOG - Number:"));
    Serial.print(value);
    Serial.println(F(" Play Finished!"));
    break;
  case DFPlayerError:
    Serial.print(F("LOG - DFPlayerError:"));
    switch (value)
    {
    case Busy:
      Serial.println(F("LOG - Card not found"));
      break;
    case Sleeping:
      Serial.println(F("LOG - Sleeping"));
      break;
    case SerialWrongStack:
      Serial.println(F("LOG - Get Wrong Stack"));
      break;
    case CheckSumNotMatch:
      Serial.println(F("LOG - Check Sum Not Match"));
      break;
    case FileIndexOut:
      Serial.println(F("LOG - File Index Out of Bound"));
      break;
    case FileMismatch:
      Serial.println(F("LOG - Cannot Find File"));
      break;
    case Advertise:
      Serial.println(F("LOG - In Advertise"));
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

void pulseLedFx(int pinOut, int delayInterval = 10)
{
  for (int brightness = 0; brightness <= 255; brightness++)
  {
    analogWrite(pinOut, brightness);
    delay(delayInterval);
  }

  for (int brightness = 255; brightness >= 0; brightness--)
  {
    analogWrite(pinOut, brightness);
    delay(delayInterval);
  }
}

void pulseBaseLeds()
{
  Serial.print("[ PULSE_LEDS ] I'm running on: ");
  Serial.println(millis());

  pulseLedFx(BASE_LEDS_PIN);
}

void blinkLeds()
{
  Serial.print("[ BLINK_LEDS ] I'm running on: ");
  Serial.println(millis());

  digitalWrite(BASE_LEDS_PIN, HIGH);
  delay(10);
  digitalWrite(BASE_LEDS_PIN, LOW);
}

void blinkCableLed()
{
  Serial.print("[ BLINK_CABLE_LEDS ] I'm running on: ");
  Serial.println(millis());

  digitalWrite(CABLE_PIN, HIGH);
  delay(10);
  digitalWrite(CABLE_PIN, LOW);
  delay(10);
  digitalWrite(CABLE_PIN, HIGH);
  delay(10);
  digitalWrite(CABLE_PIN, LOW);
}

void mistMachine()
{
  Serial.print("[ MIST_MACHINE ] I'm running on: ");
  Serial.println(millis());

  digitalWrite(MIST_MACHINE_PIN, HIGH);
  // delay(1000);
  // digitalWrite(MIST_MACHINE_PIN, LOW);
  // delay(1000);
}

void capsuleLeds()
{
  Serial.print("[ CAPSULE_LEDS ] I'm running on: ");
  Serial.println(millis());
  digitalWrite(CAPSULE_PIN, HIGH);
  digitalWrite(BOTTOM_TUBE_PIN, HIGH);
}

void playSound()
{
  Serial.print("[ PLAY_SOUND ] I'm running on: ");
  Serial.println(millis());

  if (!isPlaying)
  {
    soundPlayer.play(1);
    isPlaying = true;
  }

  if (soundPlayer.available())
  {
    printLogMessage(soundPlayer.readType(), soundPlayer.read());

    if (soundPlayer.readType() == DFPlayerPlayFinished)
    {
      int finishedFile = soundPlayer.read();
      Serial.print(F("============ File finished: "));
      Serial.println(finishedFile);

      if (finishedFile == 1)
      {
        blinkLedsThread.enabled = false;
        pulseBaseLedsThread.enabled = true;
        capsuleLedThread.enabled = true;
        mistMachineThread.enabled = true;
        soundPlayer.next();
      }

      if (finishedFile == 2)
      {
        pulseBaseLedsThread.enabled = false;
        blinkLedsThread.enabled = true;
        soundPlayer.next();
      }

      if (finishedFile == 3)
      {
        soundPlayer.play(3);
        soundPlayer.loop(3);
        Serial.print(F("On looping..."));
      }
    }
  }
}

void setup()
{
  mySerial.begin(9600);
  Serial.begin(9600);

  pinMode(BASE_LEDS_PIN, OUTPUT);
  pinMode(MIST_MACHINE_PIN, OUTPUT);
  pinMode(CAPSULE_PIN, OUTPUT);
  pinMode(BOTTOM_TUBE_PIN, OUTPUT);
  pinMode(CABLE_PIN, OUTPUT);

  if (!soundPlayer.begin(mySerial))
  {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true)
      ;
  }

  Serial.println(F("DFPlayer Mini online."));
  Serial.print(F("Amount of files in car :"));
  Serial.println(soundPlayer.readFileCounts());

  soundPlayer.volume(18);

  playSoundThread.onRun(playSound);
  playSoundThread.setInterval(1000);

  pulseBaseLedsThread.onRun(pulseBaseLeds);
  pulseBaseLedsThread.setInterval(1000);

  blinkLedsThread.onRun(blinkLeds);
  blinkLedsThread.setInterval(250);

  capsuleLedThread.onRun(capsuleLeds);
  capsuleLedThread.setInterval(250);

  mistMachineThread.onRun(mistMachine);
  mistMachineThread.setInterval(500);

  cableThread.onRun(blinkCableLed);
  cableThread.setInterval(1000);

  pulseBaseLedsThread.enabled = false;
  blinkLedsThread.enabled = false;
  capsuleLedThread.enabled = false;
  mistMachineThread.enabled = false;
  cableThread.enabled = true;

  // Adds both threads to the controller
  controll.add(&playSoundThread);
  controll.add(&pulseBaseLedsThread);
  controll.add(&blinkLedsThread);
  controll.add(&capsuleLedThread);
  controll.add(&cableThread);
  controll.add(&mistMachineThread); // & to pass the pointer to it

  delay(2000);
  playSoundThread.run();
}

void loop()
{
  // Serial.println(F("debug."));
  // digitalWrite(BASE_LEDS_PIN, HIGH);
  // digitalWrite(CAPSULE_PIN, HIGH);
  // delay(2000);

  // run ThreadController
  // this will check every thread inside ThreadController,
  // if it should run. If yes, he will run it;
  controll.run();

  // Rest of code
  // Serial.println(F("Passei aqui."));
}