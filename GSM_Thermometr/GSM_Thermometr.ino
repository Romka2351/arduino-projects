#include <SoftwareSerial.h>
#include <EEPROM.h>
#define SIM_PIN 0000

SoftwareSerial sim800l(2, 4); //RX, TX

bool sms_status = 0;
String NUM = "";

struct Settings{
  char phone[12];
  int Period;
  int HTemp;
  int LTemp;
  
};

struct Data{
  float Temp;
  float Humid;
  float Press;
};

Settings settings;
Data data = {23.6, 42.7, 731.55};

void setup() {
  Serial.begin(9600);
  sim800l.begin(9600);
  pinMode(6, INPUT_PULLUP);
  delay(1000);
  if(!digitalRead(6)){
    Serial.println("Wipe data, default settings!");
    strcpy(settings.phone, "9302211318");
    settings.Period = 15;
    saveSettings();
  }
  pinMode(6, INPUT);

  
  EEPROM.get(0, settings);
  if(settings.phone[0] == 0xFF or settings.phone[0] == 0){
    Serial.println("First start, default settings");
    strcpy(settings.phone, "9302211318");
    settings.Period = 15;
    saveSettings();
  }
  NUM = settings.phone;

  
  

  
  sim800l.println("AT");
  String line = sim800l.readStringUntil('\n');
  while(line.indexOf("OK") == -1){
    sim800l.println("AT");
    line = sim800l.readStringUntil('\n');
    delay(50);
  }
  //while(line.indexOf("CPIN: ") == -1) line = sim800l.readStringUntil("\n");
  int i = 0;
  while(line.indexOf("CPIN:") == -1 and i < 50){
    line = sim800l.readStringUntil('\n');
    i++;
    delay(100);
  }
  sim800l.println("AT+CPIN?");
  delay(50);
  line = sim800l.readStringUntil('\n');
  if(line.indexOf("OK") == -1)sim800l.println("AT+CPIN=\"" + String(SIM_PIN) + "\"");
  
  
  while(sim800l.available()) Serial.println(sim800l.readStringUntil("\n"));
  

}


bool CMGF = 0;

void loop() {
  String line; //= sim800l.readStringUntil('\n');

  //если модуль что-то отправил
  while(sim800l.available()){
    line = sim800l.readStringUntil('\n');
    Serial.println(line);
    if(line.indexOf("SMS Ready") != -1) sms_status = 1;
    if(line.indexOf("+CMTI") != -1){
      Serial.print("SMS num: " + String(atoi(line.substring(line.indexOf("\"SM\",") +5, line.indexOf("\r")).c_str())));
      Serial.println(" in line: " + line);
      //ParceSMS(atoi(line[line.indexOf("\"SM\",") +5]));
      ParceSMS(atoi(line.substring(line.indexOf("\"SM\",") +5, line.indexOf("\r")).c_str()));
    }
    if(line.indexOf("RING") != -1){
      sim800l.println("ATH");
    }
    
  }

  //автоматическое удаление смс после её обработки
  if(sms_status == 1 and CMGF == 0){
    CMGF = 1;
    sim800l.println("AT+CMGF=1");
    delay(50);
    sim800l.println("AT+CSCLK=2");
    delay(50);
    sendSMS("device online", NUM);
  }
  if(Serial.available()) sim800l.println(Serial.readStringUntil('\n'));
  

}

void Delay(uint32_t t){
  t += millis();
  while(t > millis());
}


//обработка принятой смс
void ParceSMS(int sms_index){
  sim800l.println("AT");
  delay(100);
  sim800l.print("AT+CMGR=");
  sim800l.println(sms_index);
  String str = sim800l.readStringUntil("\r");
  str.trim();
  while(str.indexOf("+CMGR") == -1){
    str = sim800l.readStringUntil("\r");
    str.trim();
  }
  sim800l.println("AT+CMGD=1,4");
  delay(100);
  //if(sim800l.available()) Serial.println(sim800l.readStringUntil("\n"));
  
  String number;
  if(str.indexOf("+CMGR") != -1){
    int num_ind = str.indexOf("\",\"+7")+5;
    for(int i = 0; i < 10; i++) number += str[num_ind + i];
  }
  bool reg_num = (number == NUM);
  Serial.println("SMS from number: " + number);

  
  String param[] = {"HTemp=", "LTemp=", "period="};
  int param_set[3] = {255, 255, 255};

  
  if(str.indexOf("OK") != -1){
    //Serial.println("String: " + str);
    if(reg_num){
      if(str.indexOf("period") != -1){
        int t1 = str.indexOf("period")+7;
        t1 = atoi(str.substring(t1, t1+2).c_str());
        settings.Period = t1;
        param_set[2] = t1;
      }
      if(str.indexOf("HTemp") != -1){
        int t1 = str.indexOf("HTemp")+6;
        t1 = atoi(str.substring(t1, t1+2).c_str());
        settings.HTemp = t1;
        param_set[0] = t1;
      }
      if(str.indexOf("LTemp") != -1){
        int t1 = str.indexOf("LTemp")+6;
        t1 = atoi(str.substring(t1, t1+2).c_str());
        settings.LTemp = t1;
        param_set[1] = t1;
      }
      if(str.indexOf("getSettings") != -1){
        param_set[0] = settings.HTemp;
        param_set[1] = settings.LTemp;
        param_set[2] = settings.Period;
      }
      if(str.indexOf("getData") != -1) sendData();
      saveSettings();
    }

    
    String answer = "";
    for(int i = 0; i < sizeof(param_set)/sizeof(int); i++){
      if(param_set[i] == 255) continue;
      if(answer != "") answer += "\r\n";
      answer += param[i] + String(param_set[i]);
    }
    if(answer != "") sendSMS(answer, NUM);

    
    if(!reg_num){
      if(str.indexOf("regPin=2413")){
        strcpy(settings.phone, number.c_str());
        saveSettings();
        Serial.println("New number saved!");
        sendSMS("Number is changed!", number);
      }else Serial.println("Unknown number!");
    }
  }
  Serial.println("End SMS");
}

void sendData(){
  String answer = "Temp: " + String(data.Temp) + "\r\n" +
                  "Humidity: " + String(data.Humid) + "\r\n" +
                  "Pressure: " + String(data.Press);
  sendSMS(answer, NUM);
}

void saveSettings(){
  EEPROM.put(0, settings);
  Serial.println("Settings saved!");
}


void sendSMS(String message, String number){
  sim800l.println("AT");
  delay(100);
  sim800l.println("AT+CMGS=\"+7" + number +"\"");
  delay(50);
  sim800l.print(message);
  delay(50);
  sim800l.write(26);
  delay(2000);
}
