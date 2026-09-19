// просыпаемся по аппаратному прерыванию из sleep
#include <GyverPower.h>
#include <GyverBME280.h>
GyverBME280 bme;

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128          //ширина дисплея (пиксели)
#define SCREEN_HEIGHT 64          //высота дисплея (пиксели)
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C       //адрес дисплея
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void printDisplay(float temp_in, float humidity, float pressur){
  //функция выводит на дисплей температуру и влажность
  
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("TempIN " + String(temp_in));
   display.println("HumIN " + String(humidity));
   //display.setTextSize(1);
   display.print("Press ");
   //display.setTextSize(2);
   display.println(" " + String(pressur));
   display.println("Voltage: " + String(float(readVcc())/1000) + "V");
   display.display();
   /*delay(5000);
   display.clearDisplay();
   display.display();
   */
}

//измеряет напряжение в милливольтах 
long readVcc() {
  //Настраиваем АЦП на измерение внутреннего 1.1В относительно Vcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  
  int result = ADCL | (ADCH << 8);
  return 1125300L / result;
}


void setup() {
  Serial.begin(9600);
  bme.setStandbyTime(STANDBY_1000MS);
  bme.begin();

  //инициализация дисплея
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);       //без выбора цвета текста ничего не выводит
  display.clearDisplay();
  

  //кнопка подключена к GND и D3
  pinMode(3, INPUT_PULLUP);

  //подключаем прерывание на пин D3 (Arduino NANO)
  //attachInterrupt(1, isr, FALLING);

  // глубокий сон
  power.setSleepMode(POWERDOWN_SLEEP);
}

// обработчик аппаратного прерывания
void isr() {
  //printDisplay(bme.readTemperature(), bme.readHumidity(), pressureToMmHg(bme.readPressure()));
  //в отличие от sleepDelay, ничего вызывать не нужно!
}

void loop() {
  Serial.print(F("Temperature: "));
  Serial.print(bme.readTemperature());
  Serial.println(F(" *C"));
  
  Serial.print("Humidity: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");
  
  Serial.print(F("Pressure: "));
  Serial.print(pressureToMmHg(bme.readPressure())); //давление в мм рт. столба
  Serial.println(F(" mm Hg"));
  Serial.println("");

  //printDisplay(bme.readTemperature(), bme.readHumidity(), pressureToMmHg(bme.readPressure()));
  
  Serial.println(F("go sleep"));
  delay(300);
  
  
  if(readVcc() >= 3200){
    printDisplay(bme.readTemperature(), bme.readHumidity(), pressureToMmHg(bme.readPressure()));
    delay(5000);
    display.clearDisplay();
    display.display();
  }
  else{
    while(readVcc() < 3300){
      display.setCursor(0, 0);
      display.println(F("Battery level is LOW!"));
      display.print("Voltage: " + String(float(readVcc())/1000));
      display.display();
      delay(2000);
      display.clearDisplay();
      display.display();
      
      attachInterrupt(1, isr, FALLING);
      power.sleep(SLEEP_FOREVER);
      detachInterrupt(1);
    }
  }

  //спим ~8 секунд, но можем проснуться по кнопке
  attachInterrupt(1, isr, FALLING);
  power.sleep(SLEEP_FOREVER);
  detachInterrupt(1);
  

  Serial.println(F("wake up!"));
  delay(300);
}
