//Web Server
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

//Bot Tele
#define USE_CLIENTSSL true
#include <AsyncTelegram2.h>
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

//Bot Tele
BearSSL::WiFiClientSecure client;
BearSSL::Session session;
BearSSL::X509List certificate(telegram_cert);
AsyncTelegram2 LampuDIFF_Bot(client);

//Setup Wifi Modul
const char* APssid = "changethis";
const char* APpassword = "changethis";
const char* ssid = "changethis";
const char* password = "changethis";
const char* token = "changethis";

int relayPin = 15;
int tombolPin = 4;

int hasilldr;
bool relayStatus = LOW;

int lastStatusTombolHard = 1;
int pilihanjalan = 0;

ReplyKeyboard replyKBTele;
bool statusKB;

void setup(void){
  delay(2000);
  Serial.begin(9600);
  Serial.println("--------SETUP--------");
  Serial.println("------Langkah 1------");
  Serial.println("Pengaturan Access Point.");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);
  Serial.println("Access Point Terbuka");
  
  Serial.println("------Langkah 2------");
  Serial.println("Menghubungkan dengan WiFi.");
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Terkoneksi : ");
  Serial.println(ssid);
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.println("Webserver akan dibuka pada IP "+WiFi.localIP().toString());
  server.on("/", halamanAwal);
  server.on("/LEDOn", halamanOn);
  server.on("/LEDOff", halamanOff);
  server.on("/Manual", halamanManual);
  server.on("/Otomatis", halamanOtomatis);
  server.begin();
  Serial.println("Webserver sudah dibuka!");

  Serial.println("------Langkah 3------");
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp");
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
  LampuDIFF_Bot.setUpdateTime(200);
  LampuDIFF_Bot.setTelegramToken(token);
  Serial.println("Mencoba Koneksi Telegram.");
  LampuDIFF_Bot.begin()?Serial.println("GOOD") : Serial.println("BAD");

  statusKB = false;
  replyKBTele.addButton("LED On");
  replyKBTele.addButton("LED Off");
  replyKBTele.addButton("LED Status");
  replyKBTele.addRow();
  replyKBTele.addButton("Hide");
  replyKBTele.enableResize();
  
  pinMode(relayPin, OUTPUT);
  pinMode(tombolPin, INPUT_PULLUP);
  digitalWrite(relayPin, relayStatus);
  
  Serial.println("-------SELESAI-------");
}

void loop(void){
  delay(50);
  hasilldr = analogRead(A0);
  server.handleClient();

  //Tombol Lampu
  int currentStatusTombolHard = digitalRead(tombolPin);
  if(currentStatusTombolHard!=lastStatusTombolHard){
    Serial.println("Tombol Ditekan");
    if(currentStatusTombolHard==0){
      relayStatus = !relayStatus;
      digitalWrite(relayPin, relayStatus);
    }
    lastStatusTombolHard=currentStatusTombolHard;
    delay(50);
  }

  //Telegram Lampu
  TBMessage msg;
  if(LampuDIFF_Bot.getNewMessage(msg)){
    MessageType msgType = msg.messageType;
    String msgText = msg.text;
    switch(msgType){
      case MessageText :
        Serial.print("Menerima pesan dari @");
        Serial.print(msg.sender.username);
        Serial.print(" dengan ID : ");
        Serial.print(int(msg.sender.id));
        Serial.println("");

        if(msgText.equalsIgnoreCase("/action")){
          LampuDIFF_Bot.sendMessage(msg, "Pilih Tindakan :", replyKBTele);
          statusKB = true;
        }else if (statusKB){
          if(msgText.equalsIgnoreCase("/action")){
            LampuDIFF_Bot.removeReplyKeyboard(msg, "Menutup Reply Keyboard.");
            statusKB = false;
          }else{
            if(msgText.equalsIgnoreCase("LED On")){
              relayStatus = true;
              digitalWrite(relayPin,HIGH);
              LampuDIFF_Bot.sendMessage(msg, relayStatus?"LED Menyala":"LED Padam");
              Serial.println("-------Lampu Menyala (Tele)-------");
            }else if(msgText.equalsIgnoreCase("LED Off")){
              relayStatus = false;
              digitalWrite(relayPin,LOW);
              LampuDIFF_Bot.sendMessage(msg, relayStatus?"LED Menyala":"LED Padam");
              Serial.println("-------Lampu Padam (Tele)-------");
            }else if(msgText.equalsIgnoreCase("LED Status")){
              LampuDIFF_Bot.sendMessage(msg, relayStatus?"LED Menyala":"LED Padam");
            }
          }
        }else{
          LampuDIFF_Bot.sendMessage(msg, "Gunakan '/action'.");
        }
        break;
      default:
        break;
    }
  }
}

void halamanAwal(){
  String statusRelay = "PADAM";
  String tombol = "<button onclick=\"location.href = '/LEDOn'\">Turn On</button>";
  String tombol2 = "<button onclick=\"location.href = '/Otomatis'\">Otomatis (Via LDR)</button>";
  String tombol3 = "<button onclick=\"location.href = '/Manual'\">Manual (Tombol On/OFF)</button>";
  
  if(relayStatus==1){
    statusRelay = "MENYALA";
    tombol = "<button onclick=\"location.href = '/LEDOff'\">Turn Off</button>";
  }
  if(pilihanjalan==1){
    tombol = "";
    tombol2 = "";
  }else{
    tombol3 = "";
  }

  if(pilihanjalan==1){
    if(hasilldr>=50){
      digitalWrite(relayPin,LOW);
      relayStatus = 0;
      Serial.println("-------Lampu Padam (LDR)-------");
    }else if(hasilldr<45){
      digitalWrite(relayPin,HIGH);
      relayStatus = 1;
      Serial.println("-------Lampu Menyala (LDR)-------");
    }
  }

  String clientIP = server.client().remoteIP().toString();
  String page = R"===(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <meta name="viewport" content="widht=device-width, initial-scale=1.0">
        <style>
          h1{text-align: center;}
          h4{text-align: center;}
          button {
            background-color: #4CAF50;
            color: white;
            padding: 15px 32px;
            text-align: center;
            display: inline-block;
            font-size: 16px;
            margin: 4px 2px;
            cursor: pointer;
           }
        </style>
        <script>
          function refresh(refreshPeriod) 
          {
            setTimeout("location.reload(true);", refreshPeriod);
          } 
          window.onload = refresh(2000);
        </script>
        <title>IoT Sistem Lampu</title>
      </head>
      <body>
        <h1>Sistem Lampu Webserver</h1>
        <h4>Lampu sedang )==="+ statusRelay + R"===(</h4>
        <h4>Tingkat Kecerahan Cahaya = )==="+ hasilldr + R"===(</h4>
        <p>)==="+ tombol + R"===(</p>
        <p>)==="+ tombol2 + R"===(</p>
        <p>)==="+ tombol3 + R"===(</p>
       </body>)===";
   server.send(200, "text/html", page);
}

void halamanOn(){
  relayStatus = 1;
  digitalWrite(relayPin, relayStatus);
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
  Serial.println("-------Lampu Menyala (MANUAL)-------");
}

void halamanOff(){
  relayStatus = 0;
  digitalWrite(relayPin, relayStatus);
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
  Serial.println("-------Lampu Padam (MANUAL)-------");
}

void halamanManual(){
  pilihanjalan=0;
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}

void halamanOtomatis(){
  pilihanjalan=1;
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}
