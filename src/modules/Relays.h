#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include <Adafruit_MCP23017.h>
#include "BaseTypes.h"


namespace systemRelays
  {    
    struct Relay : public AbstractType
      {
        public:
          Adafruit_MCP23017* board;  // плата i2c
          uint8_t pinOutput;  // pin на плате i2с, который соответствует нужному реле
          const char* name;  // имя, через которое будем общаться с масяней
          boolean level;  // уровень сигнала управления платой реле (LOW, HIGH)

          Relay(const char* name, const String& jsonLevels, Adafruit_MCP23017& board, uint8_t pinOutput, boolean level=LOW) :
            AbstractType(jsonLevels)
            {
              this->name = name;
              this->board = &board;
              this->pinOutput = pinOutput;
              this->level = level;
            };
      };

    struct SearchResponse
      {
        public:
          Relay* relay;
          boolean code;

          SearchResponse(Relay& relay) 
            {
              this->relay = &relay;
              this->code = true;
            };

          SearchResponse() 
            {
              this->code = false;
            };
          ~SearchResponse() {};
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

                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++)
                  {
                    Relay currentRelay = list[relayIndex];
                    currentRelay.board->pinMode(currentRelay.pinOutput, OUTPUT);
                  };
              };
            
            int setState(const char* relayName, boolean state)
              {
                SearchResponse result = getRelay(relayName);
                if(!result.code)
                  return -1;

                setState(result.relay, state);                
                return getState(result.relay);
              };

            int getState(const char* relayName)
              {
                SearchResponse result = getRelay(relayName);
                if(!result.code)
                  return -1;
                return getState(result.relay);
              };
            
            void readAll(void)
              {
                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++)
                  {
                    Relay relay = list[relayIndex];                    
                    relay.setValue(*doc, moduleNamespace, getState(&relay));
                  };                  
              };

          private:
            void setState(Relay* relay, boolean state) 
              {
                relay->board->digitalWrite(relay->pinOutput, state != relay->level ? LOW : HIGH);                
              };

            boolean getState(Relay* relay) 
              {
                return relay->board->digitalRead(relay->pinOutput) == relay->level ? true : false;
              };

            SearchResponse getRelay(const char* name) 
              {
                for(uint8_t relayIndex=0; relayIndex<listSize; relayIndex++)
                  {
                    Relay currentRelay = list[relayIndex];
                    if(currentRelay.name == name)
                      return SearchResponse(currentRelay);
                  };
                  return SearchResponse();
              };
        };
  };
#endif