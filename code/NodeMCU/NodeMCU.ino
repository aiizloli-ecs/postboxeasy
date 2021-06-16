#include <ESP8266WiFi.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <FirebaseESP8266.h>
#include <TimeLib.h>
#include <time.h>

// check status of system
bool STATE = true;

// communication setup
int RX = D5;
int TX = D6;
SoftwareSerial NodeSerial(RX, TX);
SoftwareSerial soft(D3, D4);

//sms setup
byte endMsg = 0x1A;

// lcd setup
#define LCDADDR 0x27
LiquidCrystal_I2C lcd(LCDADDR, 16, 2);

//Firebase token
#define FIREBASE_HOST "locker-dc169-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "6shxHuJR1ZCyyvdWCXj5RQ6Vrh3HvMyWbTHCW18Z"

// Config connect WiFi
#define WIFI_SSID "OnePlus 7 Pro"
#define WIFI_PASSWORD "Boy13411341"

// Config time
int timezone = 7;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";
int dst = 0;

// Declare the Firebase Data object in the global scope
FirebaseData firebaseData;

void printResult(FirebaseData &data);


// keypad setup
#define KPDADDR 0x23
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {7, 6, 5, 4};
byte colPins[4] = {3, 2, 1, 0};
Keypad_I2C kpd(makeKeymap(keys), rowPins, colPins, 4, 4, KPDADDR);

// menu
void mainMenu();

// fn
void depositFn();
void firebaseFn(bool, char, String, String);
void KeycardfirebaseFn(char, String, String);
void openLocker(char);
void closeLocker(char);
void sendsms(char, String, String);

void packageDeposit(char);
char lockerFn();

String cardScanner();
String phoneFn();
String OTPGenerator();
String OTPfirebase(char);
String phonefirebase(char);

void receiveFn();
String getOTP();
bool checkOTP(char);
void packageReceive(char);
void adminFn();

void keypadEvent(KeypadEvent);
void printFn(String, String);

void setup() {
  // lcd
  lcd.begin();
  printFn("LCD SETUP", "");
  delay(1000);

  // communication
  printFn("SETTING", "COMMUNICATION");
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  NodeSerial.begin(57600);
  soft.begin(9600);
  Serial.begin(115200);
  delay(1000);

  //wifi
  printFn("WIFI CONNECTING", "");
  connectWifi();
  timeCall();
  delay(1000);

  // firebase
  printFn("FIREBASE SETTING", "");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  delay(1000);

  // keypad
  printFn("KEYPAD SETTING", "");
  kpd.begin();
  kpd.addEventListener(keypadEvent);
  kpd.setHoldTime(3000);
  delay(1000);


  // wait for setup
  Serial.println("");
  Serial.println("SETUP COMPLETE");
  printFn("SETUP COMPLETE", "");
  delay(1000);

  // menu
  Serial.println(now());
  printFn("A.Deposit", "B.Receive");
}

void loop() {
  char key = kpd.getKey();
  if (key != NO_KEY) {
    Serial.println(key);
  }
  delay(100);
}

void keypadEvent(KeypadEvent key) {
  if (STATE) {
    switch (kpd.getState()) {
      case PRESSED:
        switch (key) {
          case 'A':
            depositFn();
            break;
          case 'B':
            receiveFn();
            break;
        }
        break;
      case HOLD:
        switch (key) {
          case 'D':
            adminFn();
            break;
        }
        break;
    }
  }
}

void depositFn() {
  STATE = false;

  char numLocker = '0';
  char lockerStatus = '0';
  printFn("SELECT LOCKER", "1 OR 2");
  time_t t_start = now();
  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();
    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');

    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        lockerStatus = 'O';
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        lockerStatus = 'C';
        delay(1000);
      }
      break;
    }

    if ( (key == '1') || (key == '2') ) {
      Serial.print("KEY: ");
      Serial.println(key);
      lockerStatus = lockerFn(key);
      Serial.print("lockerStatus: ");
      Serial.println(lockerStatus);
      if (lockerStatus == 'E') {
        printFn("Locker is Empty", "");
        Serial.println("Locker is Empty");
        numLocker = key;
        delay(1000);
        break;
      }
      else if (lockerStatus == 'F') {
        printFn("Locker is Full", "");
        Serial.println("Locker is Full");
        delay(1000);
        printFn("SELECT LOCKER", "1 OR 2");
        t_start = now();
      }
    }
    delay(100);
  }

  if (lockerStatus == 'E') {
    String cardID = cardScanner();
    Serial.print("cardID.length() : ");
    Serial.println(cardID.length());
    if ( (cardID.length() == 11) && (cardID == "03 89 20 44") ) {
      String phoneNumber = phoneFn();
      if (phoneNumber.length() == 10) {
        Serial.println("OPEN");
        openLocker(numLocker);

        Serial.println("PACKAGE");
        packageDeposit(numLocker);

        Serial.println("CLOSE");
        closeLocker(numLocker);

        Serial.println("OTP GENERATE");
        String OTP = OTPGenerator();

        Serial.println("SEND SMS");
        // sms code
        sendsms(numLocker, phoneNumber, OTP);
        Serial.println("SEND DATA TO FIREBASE");
        firebaseFn(true, numLocker, phoneNumber, OTP);
        KeycardfirebaseFn(numLocker, phoneNumber, cardID);
      }
      else {
        printFn("WRONG CARD", "");
        delay(1000);
      }
    }
  }
  printFn("A.Deposit", "B.Receive");
  STATE = true;
}

char lockerFn(char lockerNumber) {
  Serial.println("lockerFn()");
  char lockerStatus;
  Serial.println("SENDING DATA");
  NodeSerial.print(lockerNumber);
  NodeSerial.print('\n');

  while (true) {
    int numData = NodeSerial.available();
    Serial.print("numData : ");
    Serial.println(numData);
    if (numData > 0) {
      lockerStatus = NodeSerial.read();
      if ( (lockerStatus == 'E') || (lockerStatus == 'F') ) {
        break;
      }
    }
  }
  clearData();
  return lockerStatus;
}

String cardScanner() {
  Serial.println("cardScanner()");
  time_t t_start = now();
  String cardID = "";

  printFn("Card Scanning", "");
  Serial.println("SEND DATA TO ARDUINO");
  NodeSerial.print("3\n");

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        cardID = "O";
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        cardID = "C";
        delay(1000);
      }
      break;
    }

    int numData = (NodeSerial.available());
    Serial.print("numData:");
    Serial.println(numData);
    if (numData > 0) {
      cardID = NodeSerial.readString();
      cardID.trim();
      if ( (cardID != "\n")  && (cardID != "") ) {
        Serial.print("cardID : ");
        Serial.println(cardID);
        printFn("cardID", cardID);
        delay(1000);
        break;
      }
    }
    // for avoid watch dog timer
    delay(100);
  }
  clearData();
  Serial.println("END CARD SCANNER");
  return cardID;
}

String phoneFn() {
  time_t t_start = now();
  int index = 1;
  String phoneNumber = "0";
  printFn("Phone Number", phoneNumber);

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        phoneNumber = "O";
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        phoneNumber = "C";
        delay(1000);
      }
      break;
    }

    if ( (key != NO_KEY)) {
      if ((key >= 48) && (key <= 57) && (index < 10)) {
        lcd.print(key);
        phoneNumber += key;
        index++;
      }
      // A
      else if (key == 'A') {
        if (index > 1) {
          index--;
          phoneNumber = phoneNumber.substring(0, index);
          lcd.setCursor(index, 1);
          lcd.print(" ");
          lcd.setCursor(index, 1);
        }
      }

      // delete
      else if (key == 'B') {
        index = 1;
        phoneNumber = "0";
        printFn("Phone Number", phoneNumber);
      }

      // reset
      else if (key == 'D') {
        if (index == 10) {
          break;
        }
      }
      t_start = now();
    }
    delay(100);
  }
  return phoneNumber;
}
// command = true -> open the door
void openLocker(char numLocker) {
  Serial.println("OPEN LOCKER");
  printFn("LOCKER OPENNING", "");
  if (numLocker == '1') {
    NodeSerial.print("4\n");
  }
  else if (numLocker == '2') {
    NodeSerial.print("6\n");
  }
  delay(1000);
}
void closeLocker(char numLocker) {
  Serial.println("CLOSE LOCKER");
  printFn("LOCKER CLOSE", "");
  if (numLocker == '1') {
    NodeSerial.print("5\n");
  }
  else if (numLocker == '2') {
    NodeSerial.print("7\n");
  }
  delay(1000);
}

void packageDeposit(char numLocker) {
  printFn("Place Package", "");
  time_t t_start = now();

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
      }
      else {
        printFn("CANCLE", "");
      }
      delay(1000);
      break;
    }

    // REQUEST LOCKER STATUS FROM ARDUINO
    if (NodeSerial.available() == 0) {
      NodeSerial.print(numLocker);
      NodeSerial.print('\n');
    }

    if (NodeSerial.available() > 0) {
      t_start = now();
      char lockerStatus = NodeSerial.read();
      Serial.println(lockerStatus);
      if (lockerStatus == 'F') {
        printFn("Close Door", "& Press 'D'");
        if (key == 'D') {
          printFn("DONE", "");
          delay(1000);
          break;
        }
      }
      else if (lockerStatus == 'E') {
        printFn("Place Package", "");
      }
    }
    delay(100);
  }
  clearData();
}

String OTPGenerator() {
  Serial.println("OTP GENERATOR");
  printFn("OTP GENERATOR", "");
  String Otp = "";
  for (int i = 0; i < 8; i++) {
    Otp += String(random(0, 10));
  }
  delay(1000);
  return Otp ;
}

void sendsms(char numLocker, String phoneNumber, String OTP ) {
  printFn("SENDING SMS", "");
  String strNumLocker = String(numLocker);
  Serial.print("/*****");
  Serial.print(" CM IoT : GPRS/GSM module with Motion sensor ");
  Serial.print(" - Checking status, Please wait");

  while (ATcheck() != true) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Module is Ready");
  Serial.print(" - Set Baudrate 9600");

  while (ATBaudrate("9600") != true) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Baudrate is set");
  Serial.print(" - SMS Format");

  while (AT_SMS_format() != true) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Text mode is set");
  Serial.print(" - SMS Sending test");
  while (AT_SMS_sending(phoneNumber) != true) {
    //change to your cell phone number
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("Sending.....");
  soft.println("your OTP is : " + OTP);
  soft.println("Locker number : " + strNumLocker);
  soft.print("Thank you for using our service");
  delay(1000);
  soft.write(endMsg);
  Serial.print("Done");
}

void firebaseFn(bool op, char numLocker, String phoneNumber, String otp) {
  Serial.print("char num");
  Serial.println(numLocker);
  int NumLocker = int(numLocker) - 48;
  Serial.print("int num");
  Serial.println(NumLocker);
  FirebaseJson locker;
  locker.set("Time", NowString());
  locker.set("Date", NowString2());
  locker.set("phone", phoneNumber);
  locker.set("No_Lock", NumLocker);
  if (op) {
    locker.set("State_Lock", "Deposit");

    Firebase.pushJSON(firebaseData, "/locker", locker);
    if (numLocker == '1') {
      Firebase.setString(firebaseData, "/phone/phone1", phoneNumber);
      Firebase.setString(firebaseData, "/OTP/Locker1", otp);
    }
    else if (numLocker == '2') {
      Firebase.setString(firebaseData, "/phone/phone2", phoneNumber);
      Firebase.setString(firebaseData, "/OTP/Locker2", otp);
    }
  }
  else {
    locker.set("State_Lock", "Receive");
    Firebase.pushJSON(firebaseData, "/locker", locker);
  }
}

void KeycardfirebaseFn(char numLocker, String phoneNumber, String cardID) {
  Serial.print("char num");
  Serial.println(numLocker);
  int NumLocker = int(numLocker) - 48;
  Serial.print("int num");
  Serial.println(NumLocker);
  FirebaseJson keycard;
  keycard.set("Time", NowString());
  keycard.set("Date", NowString2());
  keycard.set("phone", phoneNumber);
  keycard.set("ID_card", cardID );
  Firebase.pushJSON(firebaseData, "/keycard", keycard);
}

String OTPfirebase(char numLocker) {
  String OTP = "";
  if (numLocker == '1') {
    Firebase.getString(firebaseData, "/OTP/Locker1");
    OTP = firebaseData.stringData();;
  }
  else if (numLocker == '2') {
    Firebase.getString(firebaseData, "/OTP/Locker2");
    OTP = firebaseData.stringData();
  }
  return OTP;
}

String phonefirebase(char numLocker) {
  String phoneNumber =  "";
  if (numLocker == '1') {
    Firebase.getString(firebaseData, "/phone/phone1");
    phoneNumber = firebaseData.stringData();
  }
  else if (numLocker == '2') {
    Firebase.getString(firebaseData, "/phone/phone2");
    phoneNumber = firebaseData.stringData();
  }
  return phoneNumber;
}

void receiveFn() {
  STATE = false;

  char numLocker = '0';
  char lockerStatus = '0';
  printFn("SELECT LOCKER", "1 OR 2");
  time_t t_start = now();
  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();
    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');

    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        lockerStatus = 'O';
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        lockerStatus = 'C';
        delay(1000);
      }
      break;
    }

    if ( (key == '1') || (key == '2') ) {
      Serial.print("KEY: ");
      Serial.println(key);
      lockerStatus = lockerFn(key);
      Serial.print("lockerStatus: ");
      Serial.println(lockerStatus);
      if (lockerStatus == 'F') {
       // printFn("Locker is Full", "");
        Serial.println("Locker is Full");
        numLocker = key;
        //delay(1000);
        break;
      }
      else if (lockerStatus == 'E') {
        printFn("Locker is Empty", "");
        Serial.println("Locker is Empty");
        delay(1000);
        printFn("SELECT LOCKER", "1 OR 2");
        t_start = now();
      }
    }
    delay(100);
  }
  if (lockerStatus == 'F') {
    // OTP = ...;  get OTP from Firebase
    String OTP = OTPfirebase(numLocker);
    String phoneNumber = phonefirebase(numLocker);
    String myOTP = getOTP();
    if (myOTP == OTP) {
      openLocker(numLocker);
      packageReceive(numLocker);
      closeLocker(numLocker);
      firebaseFn(false, numLocker, phoneNumber, "");
      printFn("DONE", "");
      delay(1000);
    }
  }
  printFn("A.Deposit", "B.Receive");
  STATE = true;
}

String getOTP() {
  time_t t_start = now();
  int index = 0;
  String myOTP = "";
  printFn("myOTP", myOTP);

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        myOTP = "O";
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        myOTP = "C";
        delay(1000);
      }
      break;
    }

    if ( (key != NO_KEY)) {
      if ((key >= 48) && (key <= 57) && (index < 8)) {
        lcd.print(key);
        myOTP += key;
        index++;
      }
      // A
      else if (key == 'A') {
        if (index > 0) {
          index--;
          myOTP = myOTP.substring(0, index);
          lcd.setCursor(index, 1);
          lcd.print(" ");
          lcd.setCursor(index, 1);
        }
      }

      // delete
      else if (key == 'B') {
        index = 0;
        myOTP = "";
        printFn("myOTP", myOTP);
      }

      // reset
      else if (key == 'D') {
        if (index == 8) {
          break;
        }
      }
      t_start = now();
    }
    delay(100);
  }
  return myOTP;
}

void packageReceive(char numLocker) {
  printFn("Get Item", "");
  time_t t_start = now();

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
      }
      else {
        printFn("CANCLE", "");
      }
      delay(1000);
      break;
    }

    // REQUEST LOCKER STATUS FROM ARDUINO
    if (NodeSerial.available() == 0) {
      NodeSerial.print(numLocker);
      NodeSerial.print('\n');
    }

    if (NodeSerial.available() > 0) {
      t_start = now();
      char lockerStatus = NodeSerial.read();
      Serial.println(lockerStatus);
      if (lockerStatus == 'E') {
        printFn("Close Door", "& Press 'D'");
        if (key == 'D') {
          printFn("DONE", "");
          delay(1000);
          break;
        }
      }
      else if (lockerStatus == 'F') {
        printFn("Get Item", "");
      }
    }
    delay(100);
  }
  clearData();
}


void adminFn() {
  STATE = false;

  String password = "55555";
  String myPassword = getPassword();
  char numLocker = '0';

  if (myPassword == password) {
    char lockerStatus = '0';
    printFn("SELECT LOCKER", "1 OR 2");
    time_t t_start = now();
    while (true) {
      time_t duration = now() - t_start;
      char key = kpd.getKey();
      bool moreFiveS = (duration >= 5);
      bool isCancle = (key == 'C');

      if (moreFiveS || isCancle) {
        //NodeSerial.print("S\n");
        if (moreFiveS) {
          printFn("TIME OUT", "");
          lockerStatus = 'O';
          delay(1000);
        }
        else {
          printFn("CANCLE", "");
          lockerStatus = 'C';
          delay(1000);
        }
        break;
      }

      if ( (key == '1') || (key == '2') ) {
        numLocker = key;
        break;
      }
      delay(100);
    }
  }

  if ( (numLocker == '1') || (numLocker == '2') ) {
    adminOperation(numLocker);
  }
  printFn("A.Deposit", "B.Receive");
  STATE = true;
}

void adminOperation(char numLocker) {
  printFn("1.OPEN DOOR", "2.CLOSE DOOR");
  time_t t_start = now();

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();
    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        delay(1000);
      }
      break;
    }

    if ( (key == '1') || (key == '2') ) {
      if (key == '1') {
        printFn("OPEN LOCKER", "");
        openLocker(numLocker);
        delay(1000);
        break;
      }
      else if (key == '2') {
        printFn("CLOSE LOCKER", "");
        closeLocker(numLocker);
        delay(1000);
        break;
      }
    }
    delay(100);
  }
  clearData();
}


String getPassword() {
  time_t t_start = now();
  int index = 0;
  String PASSWORD = "";
  printFn("PASSWORD", "");

  while (true) {
    time_t duration = now() - t_start;
    char key = kpd.getKey();

    bool moreFiveS = (duration >= 5);
    bool isCancle = (key == 'C');
    if (moreFiveS || isCancle) {
      //NodeSerial.print("S\n");
      if (moreFiveS) {
        printFn("TIME OUT", "");
        PASSWORD = "O";
        delay(1000);
      }
      else {
        printFn("CANCLE", "");
        PASSWORD = "C";
        delay(1000);
      }
      break;
    }

    if ( (key != NO_KEY)) {
      if ((key >= 48) && (key <= 57) && (index < 8)) {
        lcd.print("*");
        PASSWORD += key;
        index++;
      }
      // A
      else if (key == 'A') {
        if (index > 0) {
          index--;
          PASSWORD = PASSWORD.substring(0, index);
          lcd.setCursor(index, 1);
          lcd.print(" ");
          lcd.setCursor(index, 1);
        }
      }

      // delete
      else if (key == 'B') {
        index = 0;
        PASSWORD = "";
        printFn("PASSWORD", "");
      }

      // reset
      else if (key == 'D') {
        if (index == 5) {
          break;
        }
      }
      t_start = now();
    }
    delay(100);
  }
  return PASSWORD;
}

void printFn(String sent1 = "", String sent2 = "") {
  lcd.clear();
  lcd.print(sent1);
  lcd.setCursor(0, 1);
  lcd.print(sent2);
  lcd.noCursor();
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  //try to connect with wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());
}

void timeCall() {
  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Now: " + NowString());
}

String NowString() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);

  String tmpNow = "";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  tmpNow += ":";
  tmpNow += String(newtime->tm_sec);
  return tmpNow;
}

String NowString2() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);

  String tmpNow = "";
  tmpNow += String(newtime->tm_year + 1900);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mon + 1);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mday);
  return tmpNow;
}

void clearData() {
  char tempData;
  int numData;
  do {
    Serial.println("CLEAR DATA");
    numData = NodeSerial.available();
    Serial.print("numData : ");
    Serial.println(numData);
    tempData = NodeSerial.read();
    Serial.print("tempData : ");
    Serial.println(tempData);
    delay(100);
  } while (numData != 0);
}

boolean ATcheck() {
  String incoming;
  soft.println("AT");
  delay(1000);

  while (soft.available()) {
    byte in = soft.read();
    incoming += (char)in;
    if (incoming.indexOf("OK") != -1) {
      incoming = "";
      return true;
    }
  }
  incoming = "";
  return false;
}



boolean ATBaudrate(String baud) {
  String incoming;
  String command = "AT+IPR=" + baud;
  soft.println(command);
  delay(1000);
  while (soft.available()) {
    byte in = soft.read();
    incoming += (char)in;

    if (incoming.indexOf("OK") != -1) {
      incoming = "";
      return true;
    }
  }

  incoming = "";
  return false;
}



boolean AT_SMS_format() {
  String incoming;
  String command = "AT+CMGF=1";
  soft.println(command);
  delay(1000);



  while (soft.available()) {
    byte in = soft.read();
    incoming += (char)in;

    if (incoming.indexOf("OK") != -1) {
      incoming = "";
      return true;

    }

  }

  incoming = "";
  return false;

}

boolean AT_SMS_sending(String cellNumber) {
  String incoming;
  String command = "AT+CMGS=" + cellNumber;
  soft.flush();
  soft.println(command);
  delay(5000);



  while (soft.available()) {
    byte in = soft.read();
    incoming += (char)in;



    if (incoming.indexOf(">") != -1) {
      incoming = "";
      return true;

    }

  }
  incoming = "";
  return false;

}
