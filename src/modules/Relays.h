#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include "BaseTypes.h"


namespace systemRelays
  {
    struct Relay : public AbstractType
      {        
        Adafruit_MCP23017* board; //плата i2c
        uint8_t pinOutput;       //pin на плате i2с, который соответствует нужному реле
        const char* name;  // имя, через которое будем общаться с масяней
        boolean level; // уровень сигнала управления платой реле (LOW, HIGH)


        Relay(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, uint8_t pinOutput, boolean level=LOW) :
          AbstractType(jsonLevels)
          {
            this->name = name;
            this->board = &board;
            this->pinOutput = pinOutput;
            this->level = level;
          };
      };


      class RelayController : BaseController
        {
          private:
            Relay* list;

          public:
            void configure(Relay list[], uint8_t size, const char* moduleNamespace, JsonDocument& doc)              
              {
                BaseController::configure(size, moduleNamespace, doc);
                this->list = list;

                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++) {
                  Relay currentRelay = list[relayIndex];
                  currentRelay.board->pinMode(currentRelay.pinOutput, OUTPUT);
                };
              };

            int setState(const char* relayName, boolean state)
              {
                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++)
                  {
                    Relay currentRelay = list[relayIndex];
                    if(currentRelay.name == relayName)
                      return controlRelay(currentRelay, state);
                  };
                return -1;
              };

            int getState(const char* relayName)
              {
                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++)
                  {
                    Relay currentRelay = list[relayIndex];
                    if(currentRelay.name == relayName)
                      {
                        bool x = currentRelay.board->digitalRead(currentRelay.pinOutput) == currentRelay.level ? HIGH : LOW;
                        Serial.println(x ? "ON " : "OFF ");
                        return x;
                      };
                  };
                return -1;
              };

          private:
            boolean controlRelay(Relay relay, boolean state) {
              relay.board->digitalWrite(relay.pinOutput, state != relay.level ? LOW : HIGH);
              Serial.println(relay.board->digitalRead(relay.pinOutput) == relay.level ? "ON " : "OFF " );
              int t = relay.board->digitalRead(relay.pinOutput);
              Serial.println(t);
              return true;
            };
        };
  };
#endif