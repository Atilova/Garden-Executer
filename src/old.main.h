#include <Arduino.h>
#include "SparkFun_AS3935.h"



//-------гроза----------------------------------------------------------------------------

// 0x03 is default, but the address can also be 0x02, 0x01.
// Adjust the address jumpers on the underside of the product. 
#define AS3935_ADDR 0x03 
#define INDOOR 0x12 
#define OUTDOOR 0xE

// коды ответа модуля, что он обнаружил - шум, помеху или удар молнии, так определены в SparkFun_As3935
// #define  NOISE_TO_HIGH     0x01 // шум
// #define  DISTURBER_DETECT  0x04 // помеха
// #define  LIGHTNING         0x08  // молния



SparkFun_AS3935 lightning(AS3935_ADDR);
/* описание выводов платы as3935
VCC/VDD	Positive supply voltage
GND	Ground
SCL	I²C clock bus or SPI clock bus (according to SI setting)
MOSI	I²C data bus or SPI data input bus (according to SI setting)
MISO	SPI data output bus
CS	Chip Select (active low)
SI	Select Interface (GND → SPI or VDD → I²C)
IRQ	Interrupt (out)

*/
// Interrupt pin for lightning detection 
const int pinInterrupLightning = 5; //порт с прерыванием ESP или Меги


// эти значения настроек можно изменяеть только при подключении по шине I2C. видимо для spi нет функций!!!! 
// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 

uint8_t int_groza_answer=0; // возвращаемое значение модуля, что он определил
uint8_t noise = 7; // Value between 1-7 было 2
uint8_t disturber = 6; // Value between 1-10 ,default -2 
uint8_t spike = 6;  // def=2
uint8_t lightningThresh = 1;  //def=1, количество учитываемых вспышек молний в 15-ми минутном интервале до подачи прерывания IRq
 
volatile bool flag_groza;
//-------гроза----------------------------------------------------------------------------

void myISR_GROZA () 
{ //в это прерывание попадем, если модуль поставит 1 на входе 3 меги,
   
   flag_groza=1; // модуль что то обнаружил, раз попали в прерывание
   //а потом читаем состояние регистров AS3935 и сбрасываем flag_groza_AS3935 в ноль
   // Serial.println("Зашли в прерывание"); 
}


void setup()
  {
    // When lightning is detected the interrupt pin goes HIGH.
  pinMode(pinInterrupLightning, INPUT_PULLUP); 
  
  Serial.begin(115200); 
  Serial.println("AS3935 Franklin Lightning Detector"); 

  Wire.begin(); // Begin Wire before lightning sensor. 

  if( !lightning.begin() )
     { // Включение сенсора. 
      Serial.println ("модуль  Детектора молнии AS3935 не обнаружен!!!"); 
     }
    else
      Serial.println("Детектор молнии AS3935 обнаружен в системе");
    delay (100);
  
  byte divVal = lightning.readDivRatio(); 
  Serial.print("Division Ratio is set to: "); 
  Serial.println(divVal); 

   lightning.tuneCap(48); //установка значения конденсатора для нашей платы
  // считываем значение внутреннего конденсатора pF.
  int tuneVal = lightning.readTuneCap();
  Serial.print("Internal Capacitor is set to: "); 
  Serial.println(tuneVal);    
  
  
  //Serial.println("\n----Displaying oscillator on INT pin.----\n"); 
  //lightning.displayOscillator(true, 3); ///тест частоты на выходе irq as3935, частота*16

  
   
   if(lightning.calibrateOsc())
      Serial.println(" - Successfully!");
    else
      Serial.println("Not Successfully Calibrated!");
      

  //-- установка значения INDOOR 0x12 -default, или OUTDOOR OUTDOOR - 0xE
 //lightning.setIndoorOutdoor(OUTDOOR); 
   lightning.setIndoorOutdoor(INDOOR);

  //-- cчитываем значение indoor or outdoor из модуля 
  int enviVal = lightning.readIndoorOutdoor();
  
  Serial.print("IndoorOutdoor setup: ");  
  if( enviVal == INDOOR )
    Serial.println("*Indoor*");  
  else 
   if( enviVal == OUTDOOR )
     Serial.println("*Outdoor*");  
     else 
    Serial.println(enviVal, BIN);
    

 //22222222222222222222222222222222222222222222222222222222222222222222
  //----- устанавливаем уровень порога 1-7 для шума,2=default 
  lightning.setNoiseLevel(noise);  

  //--считываем значение порогового значения шума из модуля
  int noiseVal = lightning.readNoiseLevel();
  Serial.print("Noise Level is set at: ");
  Serial.println(noiseVal);

  //___________________________________________________________________________________________________помехи__________
  // при большом количестве сработок от  - "Disturbers", можно запретить выдачу  IRQ
   //=0, разрешено давать перывание IRQ при помехе; =1- нет сообщений при помехе 

  //----lightning.maskDisturber(true); //инфо о помехах выкл
  lightning.maskDisturber(false);  //инфо о наличие помех ВКЛ
  
  //считываем из модуля инфо выдвать ли сообщения при помехе 
  int maskVal = lightning.readMaskDisturber();
  Serial.print("Помехи при работе будем выводить? -> "); 
  if (maskVal == 1)
    Serial.println("нет"); 
  else if (maskVal == 0)
    Serial.println("да"); 

    

  //----- установить значение сторожевойго таймера. Разрешенные значения 1-10. Значение по умолчанию=2. Самое маленькое значение=1 
  //чем больше значение, тем больше загрубляем и больше сопротивляемость помехам
  lightning.watchdogThreshold(disturber); 

  //считать значение сторожевого таймера
  int watchVal = lightning.readWatchdogThreshold();
  Serial.print("Watchdog Threshold is set to: ");
  Serial.println(watchVal);

  
//____________________________________________
//Эта настройка, как и порог сторожевого таймера, может помочь определить между ложными событиями и действительной молнией.
 //Форма пика анализируется во время процедуры проверки сигнала.    
 //УВЕЛИЧЕНИЕ ЭТОГО ЗНАЧЕНИЯ УВЕЛИЧИВАЕТ НАДЕЖНОСТЬ за счет чувствительности к отдаленным событиям.
 //значение 1-11, значение по умолчанию =2
 // Это значение помогает различать события и реально молнию, анализируя форму всплеска во время процедуры проверки сигнала чипа.
 // чем больше значение, тем выше эффективность определения молнии(больше сопротивляемость Distarders), но меньше дистанция detection
 //всплеска импульса молнии должен быть больше этого порогового значения. !!! импульсы со значение ниже порога отбрасываются 
 //------ lightning.spikeRejection(spike); //считываем

  //считываем пороговое значение всплеска импульса
  int spikeVal = lightning.readSpikeRejection();
  Serial.print("Spike Rejection is set to: ");
  Serial.println(spikeVal);
  
  //______________________________________________________ Количество вспышек молнии до выдачи прерывания:____________________
  //Количество событий молнии перед IRQ установлено высоким. 15-ти минутное окно - окно времени до сброса количества 
  //обнаруженных событий молнии. Количество ударов молнии может быть установлено на 1,5,9 или 16.
  //default: 0 (single lightning strike)????
  
  //-----
  lightning.lightningThreshold(lightningThresh); //запись в модуль количества учитываемых молний за 15 мин 

  uint8_t lightVal = lightning.readLightningThreshold(); //Default is 1???
  Serial.print("Количество вспышек молнии до выдачи прерывания: "); 
  Serial.println(lightVal); 

 //___________________________________________________________ resetSettings()__________________________________________
 // Когда оценивается расстояние до шторма, оно учитывает другую молнию, которая была обнаружена
  //за последние 15 минут. Если вы хотите сбросить время, то вы можете вызвать эту функцию.
  //lightning.clearStatistics(); //сброс прдыдущей статистики за последние 15 мин 

  
  // сброс настроек в значения по умолчанию
 // lightning.resetSettings();

  attachInterrupt(pinInterrupLightning, myISR_GROZA, HIGH);
  // attachInterrupt(1, myISR_GROZA, RISING); это для МЕГи
  flag_groza=1;
 
  
  };




void loop()
  {
if( flag_groza)
   {
    //Serial.println("Что то есть");
    // модуль что то обнаружил и выставил нам прерывание. считываем значение регистра прерывания состояния и определяем - что это.
    int_groza_answer = lightning.readInterruptReg();
    switch (int_groza_answer)
       { case NOISE_TO_HIGH: 
                  { 
                    Serial.println("Шум"); 
                    break;
                  }
  
          case DISTURBER_DETECT:       
                  {
                    Serial.println("Помеха"); 
                    break;
                  }
                  
          case LIGHTNING:
                  {
                    Serial.println("Удар молнии Detected!"); 
                    // Lightning! Now how far away is it? Distance estimation takes into
                    // account previously seen events. 
                    byte distance = lightning.distanceToStorm(); 
                    Serial.print("Примерное расстояние до нее : "); 
                    Serial.print(distance); 
                    Serial.println("km !"); 
              
                    // "Lightning Energy" and I do place into quotes intentionally, is a pure
                    // number that does not have any physical meaning. 
                    long lightEnergy = lightning.lightningEnergy(); 
                    Serial.print("Lightning Energy: "); 
                    Serial.println(lightEnergy); 
                    break;
                }
            default:
                  {
                    break;
                  }
       } 
     flag_groza=0; //сбрасываем флаг прерывания
     delay(100); // Slow it down.
   }
}

