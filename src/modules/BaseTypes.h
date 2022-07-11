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

    #define FIELD_NOT_CHANGED 0
    #define FIELD_NESTED_SIZE_TOO_BIG -1
    #define FIELD_SET 1

    typedef std::function<boolean(void*, boolean, boolean)> CheckDifferCallback;

    template<typename ValType> ValType* useType(ValType defaultValue)
      {
        ValType* typePointer = &defaultValue;
        return typePointer;
      };

    template<typename ValType> struct MeasuringExtension
      {
        private:
          ValType lastValue;
          int isError = -1;  // First start

        public:
          MeasuringExtension(ValType initValue)
            {
              lastValue = initValue;
            };

          ~MeasuringExtension() {};

          boolean checkDiffer(void* valuePointer, boolean diff, boolean deviceError=false)
            {
              ValType* currentValue = static_cast<ValType*>(valuePointer);

              if(!diff || isError == -1)  // if error -1 (first update) allow to set value
                {
                  isError = deviceError;  // Lock if error occurred
                  if(!deviceError)  // If was an error (void* valuePointer=nullptr), prevent to set lastValue as nullptr
                    lastValue = *currentValue;
                  return true;  // Allow to set value/error
                }

              if(deviceError)
                {
                  if(isError)
                    return false;  // Already locked, diff=false (still error on device)
                  else
                    {
                      isError = true;  // Error occurred, lock. Allow to set error
                      return true;
                    };
                }
                else if(isError || lastValue != *currentValue)  // If previously was error, unlock it. Or if value changed
                  {
                    isError = false;
                    lastValue = *currentValue;
                    return true;
                  };
              return false;
            };
      };


    struct JsonFieldLevel
      {
        public:
          constexpr static const char* ERROR = "error";  // Error phrase
          const char* name;
          uint8_t size;
          char** jsonLevels;
          CheckDifferCallback differCallback = nullptr;

          JsonFieldLevel() {};

          template<typename T> JsonFieldLevel(T* typePointer, const char* name, const String& strLevels) :
            JsonFieldLevel(name, strLevels)
            {
              differCallback = std::bind(
                &MeasuringExtension<T>::checkDiffer,
                new MeasuringExtension<T> {*typePointer},
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
              );
            };

          JsonFieldLevel(const char* name, const String& strLevels)
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

          struct SearchField
            {
              public:
                boolean isFound;
                JsonFieldLevel* field;

                SearchField(const char* name, MultipleAbstractType* _this)
                  {
                    if(name == nullptr)
                      {
                        this->field = _this->jsonFields[0];
                        this->isFound = true;
                        return;
                      }
                      else
                        for(uint8_t index = 0; index < _this->fillIndex; index++)
                          {
                            if(!strcmp(_this->jsonFields[index]->name, name))
                              {
                                this->field = _this->jsonFields[index];
                                this->isFound = true;
                                return;
                              };
                          };
                    this->isFound = false;
                  };

              ~SearchField() {};
            };

          int setJson(JsonDocument& doc, const char* section, JsonFieldLevel* jsonField, auto& value)
            {
              if(section == nullptr)
                return setJson(&doc, jsonField, value);              
              auto docSection = doc[section];
              return setJson(&docSection, jsonField, value);
            };

          int setJson(auto* doc, JsonFieldLevel* jsonField, auto& value)
            {
              char** jsonLevels = jsonField->jsonLevels;
              switch(jsonField->size)
                {
                  case(1):
                    (*doc)[jsonLevels[0]] = value;
                    break;
                  case(2):
                    (*doc)[jsonLevels[0]][jsonLevels[1]] = value;
                    break;
                  case(3):
                    (*doc)[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]] = value;
                    break;
                  case(4):
                    (*doc)[jsonLevels[0]][jsonLevels[1]][jsonLevels[2]][jsonLevels[3]] = value;
                    break;
                  default:
                    return FIELD_NESTED_SIZE_TOO_BIG;  // Not supported size
                };
              return FIELD_SET;
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

          // Should be called at least ones for one device (jsonFields[SIZE] won't be able to store more)
          template<class FirstField, class ...Fields> void add(const FirstField& firstField, const Fields& ...args)
            {
              if(jsonFields == nullptr)
                jsonFields = new JsonFieldLevel* [sizeof...(args)+1];

              addField(firstField);
              add(args...);
            };

          int setValue(JsonDocument& doc, const char* section, auto value, boolean diff, const char* name=nullptr)
            {
              SearchField result(name, this);
              if(!result.isFound)
                return -1;

              CheckDifferCallback callback = result.field->differCallback;
              return (callback == nullptr || callback(static_cast<void*>(&value), diff, false))
                ? setJson(doc, section, result.field, value)
                : FIELD_NOT_CHANGED;
              
            };

          int setError(JsonDocument& doc, const char* section, boolean diff, const char* name=nullptr, const char* error=JsonFieldLevel::ERROR)
            {
              SearchField result(name, this);
              if(!result.isFound)
                return -1;

              CheckDifferCallback callback = result.field->differCallback;
              return (callback == nullptr || callback(static_cast<void*>(nullptr), diff, true))
                ? setJson(doc, section, result.field, error)
                : FIELD_NOT_CHANGED;
            };
      };
#endif