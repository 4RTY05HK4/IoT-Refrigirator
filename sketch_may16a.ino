#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>

RTC_DS3231 rtc;
OneWire oneWire(27);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DeviceAddress Device1Address;
int lod = 13, zamr = 14, buzz = 25;
bool flagLOD = false, backup_flagLOD = false, flagZAMR = false, backup_flagZAMR = false, DataSent = true;
int hC = 1000, mC = 1000, sC = 1000, hO = 1000, mO = 1000, sO = 1000, LoC = 0, ZoC = 0, mCtmp = 0, DeviceIDtmp = 13;
float DOtemperatureC, DCtemperatureC;
String timeOD, timeDC, dataNow;

void setup() {
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_13,1);
  esp_sleep_enable_ext1_wakeup(0x6000,ESP_EXT1_WAKEUP_ANY_HIGH);
  if(digitalRead(lod) == 1 && flagLOD == false && backup_flagLOD == false)
  {
    backup_flagLOD = true;  
  }
  if(digitalRead(zamr) == 1 && flagZAMR == false && backup_flagZAMR == false)
  {
    backup_flagZAMR = true;  
  }

  Wire.begin(32, 33);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, World!");
  pinMode(lod, INPUT_PULLUP);
  pinMode(zamr, INPUT_PULLUP);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);                              
  WiFi.begin("TP-Link_24", "pasz12port");               
  
  while (WiFi.status() != WL_CONNECTED){
    //Serial.println("Brak polaczenia");
    lcd.clear();
    lcd.print("Brak polaczenia");
    delay(500);
  }
  lcd.clear();
  lcd.print("Polaczono z WiFi");
  //Serial.print("Nawiazano polaczenie, addres IP : ");
  //Serial.println(WiFi.localIP());
  
  while(!Ping.ping("192.168.0.169", 1)){
      lcd.clear();
      lcd.print("Serwer nie");
      lcd.setCursor(0, 1);
      lcd.print("odpowiada");
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  
  delay(500);
  

  pinMode(buzz, OUTPUT);
  
  if (!rtc.begin()) {
    AlertsHandler(4);
    while(1);
  }
  
  //rtc.adjust(DateTime(2021, 5, 20, 17, 56, 0));
  sensors.begin();
  
  DateTime nowTmp = rtc.now();
  mCtmp = nowTmp.minute();
  if(mCtmp > 58) mCtmp += 1 - 60;
  else mCtmp += 1;
  sC = nowTmp.second();
  lcd.clear();
  lcd.noBacklight();
}

void loop() {

  if(digitalRead(lod) == 1 && digitalRead(zamr) == 0){
    DeviceIDtmp = 13;
  }else if(digitalRead(zamr) == 1 && digitalRead(lod) == 0){
    DeviceIDtmp = 14;
  }
  DeviceHandler(DeviceIDtmp);


  
  //delay(1000);
  
}

void DeviceHandler(int DeviceID){
    DateTime now = rtc.now();
    if((digitalRead(DeviceID) == 1 && returnflagValue(DeviceID) == false) || returnBackupFlagValue(DeviceID) == true){
      lcd.clear();
      lcd.backlight();
      //flag = true;
      setFlag(DeviceID, true);
      //backup_flag = false;
      setBackupFlag(DeviceID, false);
      incrementCounter(DeviceID);
      hO = now.hour();
      mO = now.minute();
      sO = now.second();
      timeOD = (String)hO + ":" + (String)mO + ":" + (String)sO;
      //Serial.println(timeOD);
      lcd.print(timeOD);
  
      DOtemperatureC = getTempFromDeviceID(DeviceID);
      lcd.setCursor(10, 0);
      lcd.print((String)DOtemperatureC);
      lcd.print("C");
      //Serial.print(DOtemperatureC);
      //Serial.println("ºC");
      delay(1000);
  }
  else if(digitalRead(DeviceID) == 1 && returnflagValue(DeviceID) == true){
      if((mO < now.minute() && sO <= now.second())){
        AlertsHandler(2);
      }else if(returnCounterValue(DeviceID) > 3){
        AlertsHandler(1);
      }else if(mO < now.minute() && sO <= now.second() && returnCounterValue(DeviceID) > 3){
        AlertsHandler(6);
      }
      //Serial.println("Door opened!");
  }
  else if(digitalRead(DeviceID) == 0 && returnflagValue(DeviceID) == true){
      if(returnCounterValue(DeviceID) > 3){
        lcd.clear();
      }
      //flag = false;
      setFlag(DeviceID, false);
      hC = now.hour();
      mC = now.minute();
      if(mC > 56) mCtmp = mC + 3 - 60;
      else mCtmp = mC + 3;
      sC = now.second();
      timeDC = (String)hC + ":" + (String)mC+ ":" + (String)sC;
      //Serial.println(timeDC);
      lcd.setCursor(0, 1);
      lcd.print(timeDC);
  
      DCtemperatureC = getTempFromDeviceID(DeviceID);
      //Serial.print(DCtemperatureC);
      //Serial.println("ºC");
      lcd.setCursor(10, 1);
      lcd.print((String)DCtemperatureC);
      lcd.print("C");
      
      String dane = "DeviceID=" + (String)DeviceID + "&DataSent=" + (String)DataSent + "&DCtemperatureC=" + (String)DCtemperatureC + "&DOtemperatureC=" + (String)DOtemperatureC + "&timeOD=" + timeOD + "&timeDC=" + timeDC;
      
      HTTPClient http;
      http.begin("http://192.168.0.169/index/index.php");
      http.setConnectTimeout(1000);
      int httpCode = http.GET();
      if(httpCode < 0)
      {
        if(httpCode == HTTPC_ERROR_CONNECTION_REFUSED) 
          AlertsHandler(3);
      }else if(httpCode >= 0){
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.POST(dane);
        //Serial.println(dane);
        http.end();
      }
  }
  else if(digitalRead(DeviceID) == 0 && returnflagValue(DeviceID) == false){
      if(mCtmp == now.minute() && sC <= now.second()){
        //Serial.println("going to sleep");
        lcd.clear();
        lcd.backlight();
        lcd.print("Ide spac");
        delay(3000);
        lcd.clear();
        lcd.noBacklight();
        esp_deep_sleep_start();
      }
      HTTPClient http;
      http.begin("http://192.168.0.169/index/index.php");
      http.setConnectTimeout(1000);
      int httpCode = http.GET();
      if(httpCode < 0)
      {

        if(httpCode == HTTPC_ERROR_CONNECTION_REFUSED){
            lcd.backlight(); 
            AlertsHandler(3);
          }
      }else{
        //Serial.println("Door closed");
        lcd.clear();
        lcd.noBacklight();
      }
    }
}

int getTempFromDeviceID(int DeviceID){
  int sensorID;
  if(DeviceID == 13){
    sensorID = 0;
  }else if(DeviceID == 14){
    sensorID = 1;
  }
  sensors.requestTemperatures();
  sensors.getAddress(Device1Address, sensorID); 
  return sensors.getTempC(Device1Address);
}

void incrementCounter(int DeviceID){
  if(DeviceID == 13){
    LoC++;
  }else if(DeviceID == 14){
    ZoC++;
  }
}

int returnCounterValue(int DeviceID){
  if(DeviceID == 13){
    return LoC;
  }else if(DeviceID == 14){
    return ZoC;
  }
}

void setFlag(int DeviceID, bool value){
  if(DeviceID == 13){
    flagLOD = value;
  }else if(DeviceID == 14){
    flagZAMR = value;
  }
}

void setBackupFlag(int DeviceID, bool value){
  if(DeviceID == 13){
    backup_flagLOD = value;
  }else if(DeviceID == 14){
    backup_flagZAMR = value;
  }
}

bool returnflagValue(int DeviceID){
  if(DeviceID == 13){
    return flagLOD;
  }else if(DeviceID == 14){
    return flagZAMR;
  }
}

bool returnBackupFlagValue(int DeviceID){
  if(DeviceID == 13){
    return backup_flagLOD;
  }else if(DeviceID == 14){
    return backup_flagZAMR;
  }
}

void AlertsHandler(int errCode){
  switch (errCode) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ZBYT CZESTE ");
      lcd.setCursor(0, 1);
      lcd.print("OTWIERANIE DRZWI");
      digitalWrite(buzz, HIGH);
      delay(1000);
      digitalWrite(buzz, LOW); 
      lcd.clear();
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ZBYT DLUGO ");
      lcd.setCursor(0, 1);
      lcd.print("OTWARTE DRZWI");
      digitalWrite(buzz, HIGH);
      delay(1000);
      digitalWrite(buzz, LOW); 
      lcd.clear();
      break;
    case 3:
      lcd.clear();
      lcd.print("Serwer nie");
      lcd.setCursor(0, 1);
      lcd.print("odpowiada");
    break;
    case 4:
      lcd.clear();
      lcd.print("RTC nie");
      lcd.setCursor(0, 1);
      lcd.print("odpowiada");
    break;
    case 5:
      lcd.clear();
      lcd.print("Czujnik nie");
      lcd.setCursor(0, 1);
      lcd.print("odpowiada");
    break;
    case 6:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ZA DLUGO/CZESTO ");
      lcd.setCursor(0, 1);
      lcd.print("OTWARTE DRZWI");
      digitalWrite(buzz, HIGH);
      delay(1000);
      digitalWrite(buzz, LOW); 
      lcd.clear();
    break;
    default:
    
    break;
  }
}
