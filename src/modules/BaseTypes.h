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

    struct JsonFieldLevel
      {
        const char* name;
        const uint8_t index;
      };

    struct MultipleAbstractType
      {
        private:
          uint8_t* jsonFieldsSize;  // [{"name": "w", "index": 0}, {"name": "q", "index": 1}]
          char*** jsonFieldsArray;  // [["w", "w", "w"], ["q", "q"]]
          uint8_t fillIndex = 0;

          template <class Field> void add(const Field& field)
            {
              addField(field);
            };

          void addField(const String* levelConfig)
            {
              char** jsonLevels = new char* [std::count(levelConfig[0].begin(), levelConfig[0].end(), '.')];
              char levelCharArray[levelConfig[0].length()];
              strcpy(levelCharArray, levelConfig[0].c_str());

              char* token = strtok(levelCharArray, ".");
              uint8_t index = 0;

              while(token)
                {
                  jsonLevels[index] = new char[strlen(token)+1];
                  strcpy(jsonLevels[index], token);
                  token = strtok(NULL, ".");
                  index++;
                };

              jsonFieldsArray[fillIndex] = jsonLevels;
              jsonFieldsSize[fillIndex] = index;
              fillIndex++;
            };

        public:
          // Should be called at least ones
          template<class FirstField, class ...Fields> void add(const FirstField& firstField, const Fields& ...args)
            {
              uint8_t argsSize = sizeof...(args) + 1;
              jsonFieldsSize = new uint8_t[argsSize];
              jsonFieldsArray = new char** [argsSize];

              addField(firstField);
              add(args...);
            };

          ~MultipleAbstractType() {};

          int setValue(JsonDocument& doc, const char* section, auto value, uint8_t levelIndex=0)
            {
              auto docSection = doc[section];
              char** jsonLevels = jsonFieldsArray[levelIndex];

              //Todo: needs a better solution              
              switch(jsonFieldsSize[levelIndex])
                {
                  case(1):
                    docSection[jsonLevels[0]] = value;
                    break;
                  case(2):
                    docSection[jsonLevels[0]][jsonLevels[1]] = value;
                    break;
                  case(3):
                    docSection[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]] = value;
                    break;
                  case(4):
                    docSection[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]][jsonLevels[3]] = value;
                    break;
                  default:
                    return -1;
                };
              return true;
            };

      };
#endif