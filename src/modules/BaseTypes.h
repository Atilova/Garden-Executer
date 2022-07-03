#ifndef _BASE_TYPES_H_
  #define _BASE_TYPES_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <ArduinoJson.h>


  class BaseController
    {
      public:
        uint8_t listSize;
        const char* moduleNamespace;
        JsonDocument* doc;

      public:
        BaseController(void) {};
        ~BaseController(void) {};

        void configure(uint8_t size, const char* moduleNamespace, JsonDocument& doc)
          {
            this->listSize = size;
            this->moduleNamespace = moduleNamespace;
            this->doc = &doc;
          };
      };


    struct AbstractType
      {
        public:
          char** jsonNestedLevels;
          uint8_t jsonNestedLevelsSize = 1;

          AbstractType(const String& levels)
            {
              this->jsonNestedLevelsSize += std::count(levels.begin(), levels.end(), '.');
              this->jsonNestedLevels = new char* [jsonNestedLevelsSize];

              char levelCharArray[levels.length()];
              strcpy(levelCharArray, levels.c_str());

              char* token = strtok(levelCharArray, ".");
              uint8_t index = 0;

              while(token)
                {
                  jsonNestedLevels[index] = new char[strlen(token)+1];
                  strcpy(jsonNestedLevels[index], token);
                  token = strtok(NULL, ".");
                  index++;
                };
            };
          ~AbstractType() {};

          int setValue(JsonDocument& doc, const char* section, auto value)
            {
              //Todo: needs a better solution
              auto docSection = doc[section];
              switch(jsonNestedLevelsSize)
                {
                  case(1):
                    docSection[jsonNestedLevels[0]] = value;
                    break;
                  case(2):
                    docSection[jsonNestedLevels[0]][jsonNestedLevels[1]] = value;
                    break;
                  case(3):
                    docSection[jsonNestedLevels[0]][jsonNestedLevels[1]][jsonNestedLevels[2]] = value;
                    break;
                  case(4):
                    docSection[jsonNestedLevels[0]][jsonNestedLevels[1]][jsonNestedLevels[2]][jsonNestedLevels[3]] = value;
                    break;
                  default:
                    return -1;
                };
              return true;
            };
      };
#endif