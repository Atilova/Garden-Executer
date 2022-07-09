#ifndef _BASE_TYPES_H_
  #define _BASE_TYPES_H_

  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <ArduinoJson.h>
  #include <iostream>
  #include <functional>

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


//* new version

    typedef std::function<void(void)> DifferType;

    template<typename T> T* useType(T defaultValue) 
      {        
        T* typePointer = &defaultValue;
        return typePointer;
      };

    template<typename ValType> struct LastMeasuring
      {
        ValType lastValue;

        LastMeasuring(ValType initValue) 
          {
            lastValue = initValue;
          };

        ~LastMeasuring() {};

        void getDiffer(void)
          {
            Serial.print("Here -> ");
            Serial.println(lastValue);
          };    

      };


    struct JsonFieldLevel
      {
        public:
          void* lastValue;
          const char* name;
          uint8_t size;
          char** jsonLevels;
          DifferType myPtr;

          JsonFieldLevel() {};

          template<typename T> JsonFieldLevel(T* typePointer, const char* name, const String& strLevels)
            {
              this->name = name;
              this->size = std::count(strLevels.begin(), strLevels.end(), '.') + 1;
              this->jsonLevels = new char* [size];

              char levelCharArray[strLevels.length()];
              strcpy(levelCharArray, strLevels.c_str());

              char* token = strtok(levelCharArray, ".");
              uint8_t index = 0;

              while(token)
                {
                  this->jsonLevels[index] = new char[strlen(token)+1];
                  strcpy(this->jsonLevels[index], token);
                  token = strtok(NULL, ".");
                  index++;
                };

              myPtr = std::bind(&LastMeasuring<T>::getDiffer, new LastMeasuring<T> {*typePointer});              
            };

          ~JsonFieldLevel() {};
      };


    struct MultipleAbstractType
      {
        private:
          JsonFieldLevel** jsonFields = nullptr;
          uint8_t fillIndex = 0;

          void addField(JsonFieldLevel* field)
            {
              jsonFields[fillIndex] = field;
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

          void show(void)
            {
              for(uint8_t index = 0; index < fillIndex; index++)
                {
                  Serial.print("Name -> ");              
                  Serial.println(jsonFields[index]->name);
                  jsonFields[index]->myPtr();
                };
            };

          // int setValue(JsonDocument& doc, const char* section, auto value, const char* name=nullptr)
          //   {
          //     if(jsonFields == nullptr)
          //       return -1;  // Not configured

          //     JsonFieldLevel* searchedField = nullptr;
          //     if(name == nullptr)
          //       searchedField = jsonFields[0];
          //     else
          //       for(uint8_t index = 0; index < fillIndex; index++)
          //         {
          //           Serial.println(jsonFields[index]->name);
          //           if(!strcmp(jsonFields[index]->name, name))
          //             searchedField = jsonFields[index];
          //         };

          //     if(searchedField == nullptr)
          //       return -2;  // Unable to search

          //     auto docSection = doc[section];
          //     char** jsonLevels = searchedField->jsonLevels;
          //     //Todo: needs a better solution
          //     switch(searchedField->size)
          //       {
          //         case(1):
          //           docSection[jsonLevels[0]] = value;
          //           break;
          //         case(2):
          //           docSection[jsonLevels[0]][jsonLevels[1]] = value;
          //           break;
          //         case(3):
          //           docSection[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]] = value;
          //           break;
          //         case(4):
          //           docSection[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]][jsonLevels[3]] = value;
          //           break;
          //         default:
          //           return -3;  // Not supported size
          //       };
          //     return true;
          //   };
      };
#endif



