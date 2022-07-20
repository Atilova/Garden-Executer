#ifndef _LIGHTNING_H_
  #define _LIGHTNING_H_

  #include <EEPROM.h>
  #include <FunctionalInterrupt.h>
  #include <ArduinoJson.h>
  #include <SparkFun_AS3935.h>

  #define EEPROM_CONFIG_ADDRESS 0  // Адрес в eeprom, с которого хранится структура


  struct SparkConfig
    {
      boolean valid = false;
      boolean indoor = true;  // == true - датчик расположен внутри дома, иначе -снаружи
      uint8_t noiseLevel = 7;  // По умолчанию 2, минимальный уровень шума сравнивается с известным эталонным напряжением. Если этот уровень превышен, микросхема выдает прерывание на вывод IRQ, сообщая, что не может работать должным образом из-за шума (INT_NH). .
      boolean showDisturber = true;  // == true - инфо о помехах вкл, false- не выводим инфо о наличие помех
      uint8_t watchdogThreshold = 6;  // Threshold значения 1-10. по умолчанию=2. чем больше значение, тем больше загрубляем и больше сопротивляемость помехам
      uint8_t spike = 6;  // Значение 1-11, значение по умолчанию =2, настройка, как и порог сторожевого таймера, может помочь определить между ложными событиями и действительной молнией.
                      // Форма пика анализируется во время процедуры проверки сигнала.
                      // УВЕЛИЧЕНИЕ ЭТОГО ЗНАЧЕНИЯ УВЕЛИЧИВАЕТ НАДЕЖНОСТЬ за счет чувствительности к отдаленным событиям.
                      // Это значение помогает различать события и реально молнию, анализируя форму всплеска во время процедуры проверки сигнала.
                      // чем больше значение, тем выше эффективность определения молнии(больше сопротивляемость помехам), но меньше дистанция определения молнии
                      // Всплеска импульса молнии должен быть больше этого порогового значения. !!! импульсы со значение ниже порога отбрасываются

      uint8_t lightningsCount = 1;  //  1,5,9, or 16.  Количество событий молнии перед IRQ установлено высоким. 15-ти минутное окно - окно времени до сброса количества
                                // Обнаруженных событий молнии. Количество ударов молнии может быть установлено на 1,5,9 или 16.
    };

  class SparkSensorController
    {
      private:
        SparkConfig config ;
        SparkFun_AS3935* sensor;
        uint8_t capacitor,
                interrupPin;
        volatile boolean sparkHasDetected = true;

      protected:
        void IRAM_ATTR sparkUpdateISR(void)
          {
            sparkHasDetected = true;
          };

      public:
        SparkSensorController(SparkFun_AS3935& sparkSensor, uint8_t capacitor, uint8_t interrupPin)
          {
            sensor = &sparkSensor;
            this->capacitor = capacitor;
            this->interrupPin = interrupPin;
          };

        ~SparkSensorController() {};

        void init(void)
          {
            pinMode(interrupPin, INPUT_PULLUP);
            if(!sensor->begin())
              {
                Serial.println("модуль Детектора молнии AS3935 не обнаружен!!!");
                return;
              };
            sensor->tuneCap(capacitor);
            if(!sensor->calibrateOsc())
              {
                Serial.println("Not Successfully Calibrated!");
                return;
              };

            loadEEPROM();  // true, если нужно очистить eeprom, иначе пусто
            setupConfig();
            showConfig();

            attachInterrupt(interrupPin, std::bind(&SparkSensorController::sparkUpdateISR, this), HIGH);
          };

      private:
        void eraseEEPROM(void)
          {
            EEPROM.write(EEPROM_CONFIG_ADDRESS, 0xff);
            EEPROM.commit();
          };

        void loadEEPROM(boolean erase=false)
          {
            if(erase)
              eraseEEPROM();

            if(EEPROM.read(EEPROM_CONFIG_ADDRESS))
              updateEEPROM();
            else
              EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
          };

        void updateEEPROM()
          {
            EEPROM.put(EEPROM_CONFIG_ADDRESS, config);
            EEPROM.commit();
          };


        void setupConfig()
          {
            Serial.println("SETUP CONFIG");
            sensor->setIndoorOutdoor(config.indoor ? INDOOR : OUTDOOR);
            sensor->setNoiseLevel(config.noiseLevel);
            sensor->maskDisturber(!config.showDisturber);  // Инфо о наличие помех ВКЛ - false
            sensor->watchdogThreshold(config.watchdogThreshold);
            sensor->lightningThreshold(config.lightningsCount);  // Запись в модуль количества учитываемых молний за 15 мин
            sensor->spikeRejection(config.spike);
            delay(10);
            sensor->readInterruptReg();  // Requires first read, interrupt will not work otherwise
          };

        void showConfig(void)
          {
            // Cчитываем значение indoor or outdoor из модуля
            Serial.print("IndoorOutdoor setup: ");
            Serial.println(sensor->readIndoorOutdoor() == INDOOR ? "*Indoor*" : "*Outdoor*");

            // Cчитываем значение порогового значения шума из модуля
            Serial.print("Noise Level is set at: ");
            Serial.println(sensor->readNoiseLevel());

            // Cчитываем из модуля инфо выдвать ли сообщения при помехе
            Serial.print("Помехи при работе будем выводить? -> ");
            sensor->readMaskDisturber() ? Serial.println("нет") : Serial.println("да");

            // Считать значение сторожевого таймера
            Serial.print("Watchdog Threshold is set to: ");
            Serial.println(sensor->readWatchdogThreshold());

            // Cчитываем пороговое значение всплеска импульса
            Serial.print("Spike Rejection is set to: ");
            Serial.println(sensor->readSpikeRejection());

            Serial.print("Количество вспышек молнии до выдачи прерывания: ");
            Serial.println(sensor->readLightningThreshold());  // Default is 0
          };

      public:
        void loop(void)
          {
            if(!sparkHasDetected)
              return;

            sparkHasDetected = false;
            switch(sensor->readInterruptReg())
              {
                case NOISE_TO_HIGH:
                  {
                    Serial.println("Spark -> NOISE (Шум)");
                    break;
                  };
                case DISTURBER_DETECT:
                  {
                    Serial.println("Spark -> DISTURBER (Помеха)");
                    break;
                  };
                case LIGHTNING:
                  {
                    uint8_t lightningDistance = sensor->distanceToStorm();
                    Serial.println("Spark -> LIGHTNING (молния)");
                    Serial.println(lightningDistance);
                    break;
                  };
                default:
                  {
                    Serial.println("Spark -> UNTRACKED: ");
                    break;
                  };
              }
          };

        SparkConfig* getConfig(void)
          {
            return &config;
          };

        int updateConfig(JsonDocument& doc)
          {
            SparkConfig temporaryConfig = config;

            if(doc.containsKey("indoor") && doc["indoor"].is<boolean>())
              temporaryConfig.indoor = doc["indoor"];

            if(doc.containsKey("noiseLevel") && doc["noiseLevel"].is<uint8_t>())
              {
                uint8_t anySetting = doc["noiseLevel"];
                if(anySetting < 0 || anySetting > 7)
                  return -1;
                temporaryConfig.noiseLevel = anySetting;
              };

            if(doc.containsKey("showDisturber") && doc["showDisturber"].is<boolean>())
              temporaryConfig.showDisturber = doc["showDisturber"];

            if(doc.containsKey("watchdogThreshold") && doc["watchdogThreshold"].is<uint8_t>())
              {
                uint8_t anySetting = doc["watchdogThreshold"];
                if(anySetting < 1 || anySetting > 10)
                  return -2;
                temporaryConfig.watchdogThreshold = anySetting;
              };

            if(doc.containsKey("spike") && doc["spike"].is<uint8_t>())
              {
                uint8_t anySetting = doc["spike"];
                if(anySetting < 1 || anySetting > 11)
                  return -3;
                temporaryConfig.spike = anySetting;
              };

            if(doc.containsKey("lightningsCount") && doc["lightningsCount"].is<uint8_t>())
              {
                uint8_t anySetting = doc["lightningsCount"];
                if((anySetting != 1 && anySetting != 5) && (anySetting != 9 && anySetting != 16))
                  return -4;
                temporaryConfig.lightningsCount = anySetting;
              };

            config = temporaryConfig;
            updateEEPROM();
            setupConfig();
            return true;
          };
      };
#endif