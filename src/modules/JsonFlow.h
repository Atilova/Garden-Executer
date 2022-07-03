#ifndef _JSON_FLOW_H_
  #define _JSON_FLOW_H_

  #include <ArduinoJson.h>


  class JsonWorkflow
    {
      public:
        static int setValue(JsonDocument& doc, const char* section, char** levels, uint8_t nestedLevelSize, auto value)
          {
            auto docSection = doc[section];
            switch(nestedLevelSize)
              {
                case(1):
                  docSection[levels[0]] = value;
                  break;

                case(2):
                  docSection[levels[0]][levels[1]] = value;
                  break;

                case(3):
                  docSection[levels[0]][levels[1]][levels[2]] = value;
                  break;

                case(4):
                  docSection[levels[0]][levels[1]][levels[2]][levels[3]] = value;
                  break;

                default:
                  return -1;
              };
            return true;
          };
    };
#endif