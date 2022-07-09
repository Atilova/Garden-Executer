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
        public:
          const char* name;
          uint8_t size;
          char** jsonLevels;

          JsonFieldLevel(const char* name, uint8_t size)
            {
              this->name = name;
              this->size = size;
              this->jsonLevels = new char* [size];
            };

          ~JsonFieldLevel() {};
      };

    struct MultipleAbstractType
      {
        private:
          JsonFieldLevel** jsonFields = nullptr;
          uint8_t fillIndex = 0;

          void addField(const String* levelConfig)
            {
              char* name = new char[levelConfig[1].length()];
              strcpy(name, levelConfig[1].c_str());

              JsonFieldLevel* newField = new JsonFieldLevel (
                name,
                std::count(levelConfig[0].begin(), levelConfig[0].end(), '.')+1
              );

              char levelCharArray[levelConfig[0].length()];
              strcpy(levelCharArray, levelConfig[0].c_str());

              char* token = strtok(levelCharArray, ".");
              uint8_t index = 0;

              while(token)
                {
                  newField->jsonLevels[index] = new char[strlen(token)+1];
                  strcpy(newField->jsonLevels[index], token);
                  token = strtok(NULL, ".");
                  index++;
                };
              jsonFields[fillIndex] = newField;
              fillIndex++;
            };

        public:
          MultipleAbstractType() {};
          ~MultipleAbstractType() {};

          template <class Field> void add(const Field& field)
            {
              if(jsonFields == nullptr)
                jsonFields = new JsonFieldLevel* [1];

              addField(field);
            };

          // Should be called at least ones
          template<class FirstField, class ...Fields> void add(const FirstField& firstField, const Fields& ...args)
            {                            
              jsonFields = new JsonFieldLevel* [sizeof...(args)+1];
              addField(firstField);
              add(args...);
            };

          int setValue(JsonDocument& doc, const char* section, auto value, const char* name=nullptr)
            {              
              if(jsonFields == nullptr)
                return -1;  // Not configured

              JsonFieldLevel* searchedField = nullptr;
              if(name == nullptr)
                searchedField = jsonFields[0];
              else
                for(uint8_t index = 0; index < fillIndex; index++)
                  {                    
                    Serial.println(jsonFields[index]->name);
                    if(!strcmp(jsonFields[index]->name, name))
                      searchedField = jsonFields[index];                      
                  };

              if(searchedField == nullptr)
                return -2;  // Unable to search

              auto docSection = doc[section];
              char** jsonLevels = searchedField->jsonLevels;
              //Todo: needs a better solution
              switch(searchedField->size)
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
                    return -3;  // Not supported size
                };
              return true;
            };
      };
#endif