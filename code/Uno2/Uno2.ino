 #include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>

#define ledPin1         A0
#define ledPin2         A1
#define relay1          A3
#define relay2          A4
#define triggerPin1     7
#define echoPin1        6
#define triggerPin2     5
#define echoPin2        4
#define RST_PIN         9
#define SS_PIN          10
#define RX              3
#define TX              8

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
void activateRec(MFRC522 mfrc522);
void clearInt(MFRC522 mfrc522);

String strId;
SoftwareSerial UnoSerial(RX, TX); // RX | TX
char UltraSonic(int, int, int);

void setup()
{
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(triggerPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(triggerPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  UnoSerial.begin(57600);
  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);      // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();          // Init SPI bus

  mfrc522.PCD_Init();   // Initiate MFRC522

  UltraSonic(triggerPin1, echoPin1, ledPin1);
  UltraSonic(triggerPin2, echoPin2, ledPin2);
  Serial.println("End setup");
  delay(1000);
}

void loop()
{
  //Serial.println("WAITING DATA FROM ESP");
  if (UnoSerial.available() > 0)
  {
    int i_data = UnoSerial.parseInt();
    Serial.println(i_data);
    if (UnoSerial.read() == '\n')
    {
      if (i_data == 1) {
        char boxStatus1 = UltraSonic(triggerPin1, echoPin1, ledPin1);
        Serial.println("ultra1 Req");
        Serial.print("ultra status1:");
        Serial.println(boxStatus1);
        UnoSerial.print(boxStatus1);
        UnoSerial.print('\n');
        delay(100);
      }
      else if (i_data == 2) {
        char boxStatus2 = UltraSonic(triggerPin2, echoPin2, ledPin2);
        Serial.println("ultra2 Req");
        Serial.print("ultra status2:");
        Serial.println(boxStatus2);
        UnoSerial.print(boxStatus2);
        UnoSerial.print('\n');
        delay(100);
      }
      else if (i_data == 3) {
        Serial.println("Card Scanning");
        String cardID = "";
        while (true) {
          Serial.println("CARD");
          if (UnoSerial.available() > 0) {
            char STATUS = UnoSerial.read();
            if (STATUS == 'S') {
              Serial.print("STATUS : ");
              Serial.println(STATUS);
              break;
            }
          }

          mfrc522.PICC_IsNewCardPresent();
          if (mfrc522.PICC_ReadCardSerial()) {
            // String content = "";
            for (byte i = 0; i < mfrc522.uid.size; i++)
            {
              if (i == 0) {
                cardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
                cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
              }
              else {
                cardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
                cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
              }
              //Serial.println(content);
            }
            cardID.toUpperCase();
            Serial.print("cardID : ");
            Serial.println(cardID);
            UnoSerial.print(cardID);
            UnoSerial.print('\n');
            break;
          }
          delay(100);
        }
      }
      
      else if (i_data == 4) {
        Serial.println("OPEN LOCKER1");
        digitalWrite(relay1, LOW);
      }

      else if (i_data == 5) {
        Serial.println("CLOSE LOCKER1");
        digitalWrite(relay1, HIGH);
      }
      
      else if (i_data == 6) {
        Serial.println("OPEN LOCKER2");
        digitalWrite(relay2, LOW);
      }

      else if (i_data == 7) {
        Serial.println("CLOSE LOCKER2");
        digitalWrite(relay2, HIGH);
      }
    }
    i_data = 0;
  }
  delay(100);
}

char UltraSonic(int triggerPin, int echoPin, int ledPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  int duration = pulseIn(echoPin, HIGH);
  int distCM = duration / 58;
  char boxStatus;
  if (distCM <= 15) {
    digitalWrite(ledPin, LOW);
    boxStatus = 'F';
  }
  else {
    digitalWrite(ledPin, HIGH);
    boxStatus = 'E';
  }
  return boxStatus;
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String cardID = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (i == 0) {
      cardID.concat(String(buffer[i] < 0x10 ? "0" : ""));
      cardID.concat(String(buffer[i], HEX));
    }
    else {
      cardID.concat(String(buffer[i] < 0x10 ? " 0" : " "));
      cardID.concat(String(buffer[i], HEX));
    }
    cardID.toUpperCase();
  }
  return cardID;
}
