#include <BluetoothSerial.h>
#include <EEPROM.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
//#include <HardwareSerial.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define EEPROM_SIZE 4096 //100 each, ssid, passw, key
#define POS_SSID 0
#define POS_PASS 50
#define POS_KEY 100
#define POS_MODE 150
#define POS_APN 200

#define BUZZ 13   //active high
#define BUTT 12   //active low

// DHT SENSOR
#define DHTPIN 14
#define DHTTYPE DHT22


// RFID SENSOR
#define SS_PIN    21
#define RST_PIN   22

// IR OBS SENSOR
#define IRPIN 27

// RELAY
#define RELAY 26

// Serial UART for GSM
#define RXD 16
#define TXD 17

// Initial Config
String ssid       = "IPB";
String pass       = "vok451ip8";
String blPasskey  = "myiot";
String host       = "103.129.223.216";
String postUri    = "/algae/monitoring/iot/";
byte httpPort     = 80;
String response   = "";

// GET parameters
String appKey     = "";
String idn        = WiFi.macAddress();
String sensorType = "";
String payload    = "";

BluetoothSerial SerialBT;
WiFiClient client;
StaticJsonDocument<500> json;
LiquidCrystal_I2C  lcd(0x27,4,20);

String inputText[2];

//mesi
String turbidity   = "0";
String timestamp   = "0";
String water_lv    = "0";

//ilham
String Ph          = "0";
String mpx         = "0";
String water_flow  = "0";

//faiq
String lux         = "0";
String pzem        = "0";
String suhu_air    = "0";

// time management (milisecond)
int webTimeout        = 4000;
int callInterval      = 300000;
unsigned int curTime  = millis();

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);

  lcd.begin(); 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("D0 : Sx-Data Out");
  lcd.setCursor(0,1);
  lcd.print("S1 : Serial 1 In");
  lcd.setCursor(0,2);
  lcd.print("S2 : Serial 2 In");
  lcd.setCursor(0,3);
  lcd.print("S3 : Serial 3 In");

  delay(5000);
  lcd.clear();

  if(!SD.begin()){
        lcd.print("Card Mount Failed!");
        return;
    }

  WiFi.begin(ssid.c_str(),pass.c_str());
  while (WiFi.status() != WL_CONNECTED) 
  { Serial.print(".");
    delay(400);
  }

//  konek1.begin(115200, SWSERIAL_8N1, 4, 2, true, 256);
//  konek1.begin(115200);

  writeFile(SD, "/hello.txt", "");
    
}

void loop() {
  String S1 = "";
  String S2 = "";
  String S3 = "";

  while (Serial.available() > 0) {
    char c = Serial.read();
    if(c!=13 && c!=10) S1 += String(c);
  }
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if(c!=13 && c!=10) S2 += String(c);
  }
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if(c!=13 && c!=10) S3 += String(c);
  }

  // data ilham
  //  lcd.setCursor(0,1);
  //  lcd.print("S1 : "+S1);
  stringToArray(inputText,' ',S1);
  if(inputText[0]=="ph")  Ph = inputText[1];
  if(inputText[0]=="mpx") mpx = inputText[1];
  if(inputText[0]=="wfl") water_flow = inputText[1];
  if(inputText[0]=="ph_min") Serial.println("6.85");
  
  // data faiq
  //  lcd.setCursor(0,2);
  //  lcd.print("S2 : "+S2);
  stringToArray(inputText,' ',S2);
  if(inputText[0]=="lux") lux = inputText[1];
  if(inputText[0]=="va") pzem = inputText[1];
  if(inputText[0]=="tmp") suhu_air = inputText[1];
  if(inputText[0]=="wf_min") Serial1.println("7.00");

  // data mesi
  //  lcd.setCursor(0,3);
  //  lcd.print("S3 : "+S3);
  stringToArray(inputText,' ',S3);
  if(inputText[0]=="tb") turbidity = inputText[1];
  if(inputText[0]=="wl") water_lv = inputText[1];
  if(inputText[0]=="time_now") Serial2.println("23:59");

  lcd.setCursor(0,0);
  lcd.print("PH: "+Ph+"  Lux:"+lux);
//  lcd.print(String(S3));
  lcd.setCursor(0,1);
  lcd.print("Tb: "+turbidity+"  Wlv:"+water_lv);
//lcd.print(String(S2));
  lcd.setCursor(0,2);
  lcd.print("VA: "+pzem+"  Tmp:"+suhu_air);
//lcd.print(String(S1));
  lcd.setCursor(0,3);
  lcd.print("Fl: "+water_flow+"  Mpx:"+mpx);

  //Serial1.println("{'data':{'message':'data sent successfully'}}");
//  wifiGet();
  delay(500);
  lcd.clear();
  
  appendFile(SD, "/hey.txt", "World!\n");
}

void clearRow(int x)
{ lcd.setCursor(0,x);
  lcd.print("                    ");
}

void wifiGet()
{ 
  Serial.print("Host: ");
  Serial.println(host);
  
  if (!client.connect(host.c_str(), httpPort)) {
    Serial.println("connection failed");
    return;
  }

  client.println("GET "+postUri+"?order_by=tanggal&sort=DESC&limit=1 HTTP/1.1");
  client.println("Host: "+host);
  client.println("Connection: close");
  client.println();
  
  unsigned long timenow = millis();
  while (client.available() == 0) 
  { if(millis() - timenow > webTimeout) 
    {  client.stop();
       return;
    }
  }
  
  while(client.available()){
    String line = client.readStringUntil('\r');
//    Serial.println(line);
    if(line.indexOf('{')>0){
       response = line;
       Serial.println(line);
    }
  }
}

void stringToArray(String hasil[], char delimiter, String input)
{ int index = 0;
  while(input.indexOf(delimiter)>=0)
  { hasil[index] = input.substring(0,input.indexOf(delimiter));
    index++;
    input = input.substring(input.indexOf(delimiter)+1);
  }
  hasil[index] = input;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void wifiSend()
{   
  if (!client.connect(host.c_str(), httpPort)) {
    return;
  }

  client.println("GET "+postUri+"ph="+Ph+"&kekeruhan="+turbidity+"&lux="+lux+"&air_flow="+water_flow+"&udara_flow="+mpx+"&suhu_air="+suhu_air+"&ampere="+pzem+"&volt="+pzem+"&water_lv="+water_lv+" HTTP/1.1");
  client.println("Host: "+host);
  client.println("Connection: close");
  client.println();
  
  unsigned long timenow = millis();
  while (client.available() == 0) 
  { if(millis() - timenow > webTimeout) 
    {  client.stop();
       return;
    }
  }
  
  while(client.available()){
    String line = client.readStringUntil('\r');
    if(line.indexOf('{')>0){
       response = line;
    }
  }
}
